#include "pch.h"
#include "Utils.h"
#include <windows.h>
#include <stdexcept>
#include <memory>
#include "json.hpp"

using namespace Scintilla;

std::string Scintilla::String::ConvEncoding(const char* pSrc, size_t nLen, unsigned int dwScp, unsigned int dwDcp)
{
    std::string text;
    if (pSrc && nLen == 0) nLen = strlen(pSrc);
    if (!pSrc || nLen == 0) return text;

    // תUnicode
    int wlen = MultiByteToWideChar(dwScp, 0, pSrc, (int)nLen, NULL, 0);
    if(wlen <= 0) return text;

    std::shared_ptr<wchar_t[]> pUnicode(new wchar_t[(size_t)wlen + 1]);
    MultiByteToWideChar(dwScp, 0, pSrc, (int)nLen, pUnicode.get(), wlen);

    // UnicodeתĿ�����
    int dlen = WideCharToMultiByte(dwDcp, 0, pUnicode.get(), wlen, NULL, 0, NULL, NULL);
    if (dlen <= 0) return text;

    std::shared_ptr<char[]> pOut(new char[(size_t)dlen + 1]);
    WideCharToMultiByte(dwDcp, 0, pUnicode.get(), wlen, pOut.get(), dlen, NULL, NULL);

    // ���ش�����
    text.assign(pOut.get(), dlen);
    return text;
}


std::string Scintilla::String::GBKToUTF8(const char* pSrc, size_t nLen/* = 0*/)
{
    return ConvEncoding(pSrc, nLen, CP_ACP, CP_UTF8);
}



std::string Scintilla::String::UTF8ToGBK(const char* pSrc, size_t nLen/* = 0*/)
{
    return ConvEncoding(pSrc, nLen, CP_UTF8, CP_ACP);
}

std::string Scintilla::String::Trim(const std::string& sin, const std::string& chars/* = " \r\n\t"*/, bool left/* = true*/, bool right/* = true*/)
{
    if (sin.empty() || chars.empty()) return sin;
    std::string sout = sin;
    if (left)
    {
        // ���ҵ�һ����ȥ���ַ���λ��
        auto pos = sout.find_first_not_of(chars);
        if (pos != std::string::npos)
        {
            sout.erase(0, pos);
        }
        else
        {
            sout.clear();
            return sout;
        }
    }

    if (right)
    {
        // �������һ����ȥ���ַ���λ��
        auto pos = sout.find_last_not_of(chars);
        if (pos != std::string::npos)
        {
            sout.erase(pos + 1);
        }
        else
        {
            sout.clear();
            return sout;
        }
    }

    return sout;
}

bool ParseResult::ExtractContent(AiRespType arType, std::string& resp, std::string& content, bool& finished)
{
    bool bRet = false;

    switch (arType)
    {
    case AiRespType::OPENAI_TOTAL_RESP:
        bRet = ParseTotalResponse(resp, content);
        finished = true;
        break;
    case AiRespType::OPENAI_STREAM_RESP:
        bRet = ParseStreamResponse(resp, content, finished);
        break;
    default:
        break;
    }
    return bRet;
}

bool Scintilla::ParseResult::ParseTotalResponse(const std::string& jsonStr, std::string& content)
{
    bool bRet = false;
    try
    {
        auto j = nlohmann::json::parse(jsonStr);
        if (j.contains("choices") && !j["choices"].empty())
        {
            content = j["choices"][0]["message"]["content"].get<std::string>();
            bRet = true;
        }
    }
    catch (const nlohmann::json::exception&)
    {

    }
    return "";
}

bool Scintilla::ParseResult::ParseStreamResponse(std::string& resp, std::string& content, bool& finished)
{
    bool bRet = false;
    const std::string origin = resp;
    const std::string span = "\n\n";
    size_t newlinePos = resp.find(span);
    std::string chunk = resp;
    const bool bFirst = (newlinePos != std::string::npos);
    if (bFirst)
    {
        chunk = String::Trim(resp.substr(0, newlinePos));
        resp = resp.substr(newlinePos + span.size());
    }
    else
    {
        resp = "";
    }

    try
    {
        finished = false;
        content = "";
        // ȥ��"data: "ǰ׺���������ݸ�ʽΪ"data: {...}"��
        if (chunk.rfind("data: ", 0) == 0)
        {
            chunk = chunk.substr(6);
        }

        auto j = nlohmann::json::parse(chunk);
        if (j.contains("choices"))
        {
            // ��������choices��ƴ��content
            for (const auto& choice : j["choices"])
            {
                if (choice.contains("delta") && choice["delta"].contains("content"))
                {
                    content += choice["delta"]["content"].get<std::string>();
                }

                if (choice.contains("finish_reason") && !choice["finish_reason"].is_null() && choice["finish_reason"].get<std::string>() == "stop")
                {
                    finished = true;
                    break;
                }
            }

        }
        bRet = true;
    }
    catch (const nlohmann::json::exception&)
    {
        if (!bFirst)
        {
            resp = origin;
        }
    }
    return bRet;
}

bool File::ReadFile(const std::string& filename, std::string& content)
{
    // �Զ�����ģʽ���ļ������⻻�з�ת����
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    // ʹ������������ȡ�����ļ�����
    content.assign(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );

    // ����Ƿ�ɹ���ȡ���ļ�ĩβ
    return file.eof();
}

std::shared_ptr<nlohmann::json> Util::JsonLoadFile(const std::string& path)
{
    try
    {
        std::shared_ptr<nlohmann::json> pJson(new nlohmann::json());
        std::ifstream file(path);
        file >> *pJson;
        return pJson;
    }
    catch (...)
    {

    }
    return nullptr;
}

bool Util::JsonLoad(const std::string& sdat, nlohmann::json& jdat)
{
    try
    {
        jdat = nlohmann::json::parse(sdat);
        return true;
    }
    catch(...)
    {

    }
    return false;
}

void Typewriter::Run()
{
    ParseResult pr;
    std::string text;
    std::string last;
    std::string content;
    bool finished = false;
    for (int nRet = 0; (nRet = m_getter(text)) != 0; )
    {
        if (text.empty())
        {
            continue;
        }
        text = last + text;
        last = "";
        while (!text.empty() && !finished)
        {
            if (!pr.ExtractContent(AiRespType::OPENAI_STREAM_RESP, text, content, finished))
            {
                last = text;
                break;
            }
            m_writer(content);
            Sleep(50);
        }
    }
}