#pragma once
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <strsafe.h>

#pragma comment(lib, "winhttp.lib")


/// <summary>
/// �򵥵�HTTP�࣬�Զ�֧��http��https����
/// Ҳ֧����ʽ������ʱ����Ȼ��浽���У�����������TryFetchRespȥ��ѯ���
/// </summary>
class SimpleHttp
{
public:
    SimpleHttp(const std::string& home, bool https = false) :_strBaseUrl(home), _isHttps(https)
    {
        // ��ʼ��WinHTTP�Ự [[1]][[2]]
        _hSession = WinHttpOpen(L"SimpleHttp/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!_hSession)
        {
            throw std::runtime_error("Failed to initialize WinHTTP session: " + GetLastError());
        }

        // ����Ĭ�ϳ�ʱ��5�룩
        DWORD timeout = 60000;
        WinHttpSetTimeouts(_hSession, timeout, timeout, timeout, timeout);
    }
    ~SimpleHttp()
    {
        if (_worker.joinable())
        {
            _streamActive = false;
            _worker.join();
        }
        if (_hSession) WinHttpCloseHandle(_hSession);
    }

    /// <summary>
    /// ����GET����
    /// </summary>
    /// <param name="upath">�ӿ���</param>
    /// <param name="resp">����ʽ����ʱ����Ӧ������</param>
    /// <param name="stream">�Ƿ�Ϊ��ʽ����</param>
    /// <returns></returns>
    int Get(const std::string& upath, std::string& resp, bool stream = false)
    {
        return SendRequest(L"GET", upath, "", resp, stream);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="upath">�ӿ���</param>
    /// <param name="para">JSON�������</param>
    /// <param name="resp">����ʽ����ʱ����Ӧ������</param>
    /// <param name="stream">�Ƿ�Ϊ��ʽ����</param>
    /// <returns></returns>
    int Post(const std::string& upath, const std::string& para, std::string& resp, bool stream = false)
    {
        return SendRequest(L"POST", upath, para, resp, stream);
    }

    /// <summary>
    /// ��ȡ��ʽ���
    /// </summary>
    /// <param name="resp"></param>
    /// <returns>���ر������ݰ���С������0��ʾ���ݰ�������ɣ�-1��ʾû����</returns>
    int TryFetchResp(std::string& resp)
    {
        std::lock_guard<std::mutex> guard(_mtxDatas);
        if (_datas.empty())
        {
            return (_streamActive ? -1 : 0);
        }
        resp = _datas[0];
        _datas.erase(_datas.begin());
        return static_cast<int>(resp.size());
    }

    /// <summary>
    /// ��������ͷ
    /// </summary>
    /// <param name="headers"></param>
    void SetHeaders(const std::unordered_map<std::string, std::string>& headers)
    {
        for (auto& e : headers)
        {
            _headers[e.first] = e.second;
        }
    }

protected:
    /// <summary>
    /// д���������
    /// </summary>
    /// <param name="dat"></param>
    void Push(const std::string& dat)
    {
        FILE* fp = nullptr;
        fopen_s(&fp, "D:\\resp.dat", "ab+");
        if (fp != nullptr)
        {
            fwrite(dat.c_str(), dat.size(), 1, fp);
            fwrite("\n#\n", 3, 1, fp);
            fclose(fp);
        }
        std::lock_guard<std::mutex> guard(_mtxDatas);
        if (!dat.empty()) _datas.push_back(dat);
    }

private:
    int SendRequest(const std::wstring& method, const std::string& upath, const std::string& postData, std::string& resp, bool stream)
    {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        LPSTR pszOutBuffer;
        BOOL bResults = FALSE;
        HINTERNET hConnect = nullptr;
        HINTERNET hRequest = nullptr;

        try
        {
            // ��������
            hConnect = WinHttpConnect(_hSession,
                string2w(_strBaseUrl).c_str(),
                INTERNET_DEFAULT_PORT,
                0);
            if (!hConnect) throw std::runtime_error("Connection failed");

            // ��������
            hRequest = WinHttpOpenRequest(hConnect,
                method.c_str(),
                string2w(upath).c_str(),
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                (_isHttps ? (WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH) : 0));
            if (!hRequest) throw std::runtime_error("Request creation failed");

            // ���ð�ȫѡ�����֤�����
            if (_isHttps)
            {
                DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
            }

            // ��������ͷ
            SetHeaders(hRequest);

            // ��������
            bResults = WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                (LPVOID)postData.c_str(),
                (DWORD)postData.size(),
                (DWORD)postData.size(),
                0);
            if (!bResults) throw std::runtime_error("Send request failed");

            // ������Ӧ
            if (!WinHttpReceiveResponse(hRequest, nullptr))
            {
                throw std::runtime_error("Receive response failed");
            }

            // ��ȡ״̬�� [[10]]
            DWORD dwStatusCode = 0;
            dwSize = sizeof(dwStatusCode);
            WinHttpQueryHeaders(hRequest,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX,
                &dwStatusCode,
                &dwSize,
                WINHTTP_NO_HEADER_INDEX);

            if (dwStatusCode != 200)
            {
                throw std::runtime_error("HTTP error: " + std::to_string(dwStatusCode));
            }

            // ��ʽ���̴߳���
            if (stream)
            {
                _streamActive = true;
                _worker = std::thread(&SimpleHttp::StreamReceiver, this, hConnect, hRequest);
                return 0; // ��������
            }

            // ������Ӧ����
            std::string buffer;
            do
            {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize <= 0) break;

                size_t nRecv = dwSize;
                pszOutBuffer = new char[nRecv + 1];
                ZeroMemory(pszOutBuffer, nRecv + 1);

                if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))
                {
                    buffer.append(pszOutBuffer, dwDownloaded);
                }
                delete[] pszOutBuffer;
            } while (dwSize > 0);

            resp = buffer;
            return 0;
        }
        catch (const std::exception& e)
        {
            if (hRequest) WinHttpCloseHandle(hRequest);
            if (hConnect) WinHttpCloseHandle(hConnect);
            throw std::runtime_error("WinHTTP Error: " + std::string(e.what()) +
                " [ErrorCode: " + std::to_string(GetLastError()) + "]");
            return -1;
        }
    }

    /// <summary>
    /// ��ʽ����Ӧ��
    /// </summary>
    void StreamReceiver(HINTERNET hConnect, HINTERNET hRequest)
    {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        char* pszOutBuffer = nullptr;
        std::string strErr;

        try
        {
            while (_streamActive)
            {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize) || dwSize == 0) break;

                const size_t nRecv = dwSize;
                pszOutBuffer = new char[nRecv + 1];
                ZeroMemory(pszOutBuffer, nRecv + 1);

                if (WinHttpReadData(hRequest, pszOutBuffer, dwSize, &dwDownloaded))
                {
                    Push(std::string(pszOutBuffer, dwDownloaded));
                }
                delete[] pszOutBuffer;
            }
        }
        catch (...)
        {
            strErr = "stream recv data error";
        }

        // ���ͽ������
        _streamActive = false;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        if (!strErr.empty())
        {
            throw std::runtime_error(strErr.c_str());
        }
    }

    /// <summary>
    /// ��������ͷ
    /// </summary>
    void SetHeaders(HINTERNET hRequest)
    {
        // ���������������������ͷ
        for (const auto& h : _headers)
        {
            std::wstring header = string2w(h.first + ": " + h.second);
            if (!WinHttpAddRequestHeaders(hRequest,
                header.c_str(),
                static_cast<DWORD>(header.length()),
                WINHTTP_ADDREQ_FLAG_COALESCE))
            {
                //throw std::runtime_error("Failed to set header: " + h.first + " [Error: " + std::to_string(GetLastError()) + "]");
            }
        }
    }

    // std::wstring �� std::string (UTF-8)
    std::string wstring2s(const std::wstring& wstr) {
        if (wstr.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string result(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), (LPSTR)result.data(), len, nullptr, nullptr);
        return result;
    }

    // std::string (UTF-8) �� std::wstring
    std::wstring string2w(const std::string& str) {
        if (str.empty()) return {};
        int len = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring result(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), (LPWSTR)result.data(), len);
        return result;
    }

private:
    // �Ƿ�https
    bool _isHttps{ false };
    // ����ַ
    std::string _strBaseUrl;
    // Ĭ��headers
    std::unordered_map<std::string, std::string> _headers;
    // �첽����
    std::vector<std::string> _datas;
    std::mutex _mtxDatas;

    // WinHttp����
    HINTERNET _hSession = nullptr;

    // ��ʽ�������߳�
    std::atomic<bool> _streamActive{ false };
    std::thread _worker;
};


std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) return str; // ������ѭ��[[2]]

    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
    return str;
}

std::string ExtractContent(std::string& resp) {
    std::vector<std::string> results;
    std::string key = "\"content\":";
    size_t pos = 0;

    while ((pos = resp.find(key, pos)) != std::string::npos)
    {
        pos += key.length();
        // �����հ׺�ð��
        while (pos < resp.size() && (resp[pos] == ' ' || resp[pos] == ':')) pos++;

        if (resp[pos] != '"') continue; // ���ַ���ֵ����

        std::string content;

        // ����ת���ַ���˫����
        while (pos < resp.size())
        {
            char c = resp[pos++];
            if (c == '"') break;
            else if (c == '\\' && pos < resp.size())
            {
                switch (resp[pos++])
                {
                case 'n':
                    c = '\n';
                    break;
                case '\r':
                    c = '\r';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case '"':
                    c = '\"';
                    break;
                default:
                    pos--;
                    break;
                }
            }
            content += c;
        }
        resp = resp.substr(pos);
        return content;
    }
    resp = "";
    return resp;
}