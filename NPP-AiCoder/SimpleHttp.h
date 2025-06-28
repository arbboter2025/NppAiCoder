#pragma once
#include "Utils.h"
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
/// 简单的HTTP类，自动支持http和https请求
/// 也支持流式处理，此时结果先缓存到队列，需主动调用TryFetchResp去轮询结果
/// </summary>
class SimpleHttp
{
public:
    SimpleHttp(const std::string& home, bool https = false) :_strBaseUrl(home), _isHttps(https)
    {
        // 初始化WinHTTP会话 [[1]][[2]]
        _hSession = WinHttpOpen(L"SimpleHttp/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!_hSession)
        {
            throw std::runtime_error("Failed to initialize WinHTTP session: " + GetLastError());
        }

        // 设置默认超时（5秒）
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
    /// 发起GET请求
    /// </summary>
    /// <param name="upath">接口名</param>
    /// <param name="resp">非流式请求时接收应答数据</param>
    /// <param name="stream">是否为流式请求</param>
    /// <returns></returns>
    int Get(const std::string& upath, std::string& resp, bool stream = false)
    {
        return SendRequest(L"GET", upath, "", resp, stream);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="upath">接口名</param>
    /// <param name="para">JSON请求参数</param>
    /// <param name="resp">非流式请求时接收应答数据</param>
    /// <param name="stream">是否为流式请求</param>
    /// <returns></returns>
    int Post(const std::string& upath, const std::string& para, std::string& resp, bool stream = false)
    {
        return SendRequest(L"POST", upath, para, resp, stream);
    }

    /// <summary>
    /// 获取流式结果
    /// </summary>
    /// <param name="resp"></param>
    /// <returns>返回本次数据包大小，等于0表示数据包接收完成，-1表示没数据</returns>
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
    /// 设置请求头
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
    /// 写入接收数据
    /// </summary>
    /// <param name="dat"></param>
    void Push(const std::string& dat)
    {
#ifdef _DEBUG
        FILE* fp = nullptr;
        fopen_s(&fp, "D:\\resp.dat", "ab+");
        if (fp != nullptr)
        {
            fwrite(dat.c_str(), dat.size(), 1, fp);
            fwrite("\n#\n", 3, 1, fp);
            fclose(fp);
        }
#endif
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
            // 创建连接
            hConnect = WinHttpConnect(_hSession,
                Scintilla::String::string2w(_strBaseUrl).c_str(),
                INTERNET_DEFAULT_PORT,
                0);
            if (!hConnect) throw std::runtime_error("Connection failed");

            // 创建请求
            hRequest = WinHttpOpenRequest(hConnect,
                method.c_str(),
                Scintilla::String::string2w(upath).c_str(),
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                (_isHttps ? (WINHTTP_FLAG_SECURE | WINHTTP_FLAG_REFRESH) : 0));
            if (!hRequest) throw std::runtime_error("Request creation failed");

            // 设置安全选项（忽略证书错误）
            if (_isHttps)
            {
                DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
            }

            // 设置请求头
            SetHeaders(hRequest);

            // 发送请求
            bResults = WinHttpSendRequest(hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                (LPVOID)postData.c_str(),
                (DWORD)postData.size(),
                (DWORD)postData.size(),
                0);
            if (!bResults) throw std::runtime_error("Send request failed");

            // 接收响应
            if (!WinHttpReceiveResponse(hRequest, nullptr))
            {
                throw std::runtime_error("Receive response failed");
            }

            // 获取状态码 [[10]]
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

            // 流式子线程处理
            if (stream)
            {
                _streamActive = true;
                _worker = std::thread(&SimpleHttp::StreamReceiver, this, hConnect, hRequest);
                return 0; // 立即返回
            }

            // 处理响应数据
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
    /// 流式接收应答
    /// </summary>
    void StreamReceiver(HINTERNET hConnect, HINTERNET hRequest)
    {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        char* pszOutBuffer = nullptr;
        std::string strErr;

        try
        {
            while (_streamActive || g_bRun.load())
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

        // 发送结束标记
        _streamActive = false;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        if (!strErr.empty())
        {
            throw std::runtime_error(strErr.c_str());
        }
    }

    /// <summary>
    /// 设置请求头
    /// </summary>
    void SetHeaders(HINTERNET hRequest)
    {
        // 创建请求后立即设置请求头
        for (const auto& h : _headers)
        {
            std::wstring header = Scintilla::String::string2w(h.first + ": " + h.second);
            if (!WinHttpAddRequestHeaders(hRequest,
                header.c_str(),
                static_cast<DWORD>(header.length()),
                WINHTTP_ADDREQ_FLAG_COALESCE))
            {
                throw std::runtime_error("Failed to set header: " + h.first + " [Error: " + std::to_string(GetLastError()) + "]");
            }
        }
    }

private:
    // 是否https
    bool _isHttps{ false };
    // 基地址
    std::string _strBaseUrl;
    // 默认headers
    std::unordered_map<std::string, std::string> _headers;
    // 异步数据
    std::vector<std::string> _datas;
    std::mutex _mtxDatas;

    // WinHttp对象
    HINTERNET _hSession = nullptr;

    // 流式操作子线程
    std::atomic<bool> _streamActive{ false };
    std::thread _worker;
};