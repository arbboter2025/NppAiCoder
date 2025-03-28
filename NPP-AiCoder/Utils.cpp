#include "pch.h"
#include "Utils.h"
#include <windows.h>
#include <stdexcept>
#include <memory>
#include "json.hpp"

std::string Scintilla::String::ConvEncoding(const char* pSrc, size_t nLen, unsigned int dwScp, unsigned int dwDcp)
{
    std::string text;
    if (pSrc && nLen == 0) nLen = strlen(pSrc);
    if (!pSrc || nLen == 0) return text;

    // 转Unicode
    int wlen = MultiByteToWideChar(dwScp, 0, pSrc, (int)nLen, NULL, 0);
    if(wlen <= 0) return text;

    std::shared_ptr<wchar_t[]> pUnicode(new wchar_t[(size_t)wlen + 1]);
    MultiByteToWideChar(dwScp, 0, pSrc, (int)nLen, pUnicode.get(), wlen);

    // Unicode转目标编码
    int dlen = WideCharToMultiByte(dwDcp, 0, pUnicode.get(), wlen, NULL, 0, NULL, NULL);
    if (dlen <= 0) return text;

    std::shared_ptr<char[]> pOut(new char[(size_t)dlen + 1]);
    WideCharToMultiByte(dwDcp, 0, pUnicode.get(), wlen, pOut.get(), dlen, NULL, NULL);

    // 返回处理结果
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
        // 查找第一个非去除字符的位置
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
        // 查找最后一个非去除字符的位置
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
        // 去除"data: "前缀（假设数据格式为"data: {...}"）
        if (chunk.rfind("data: ", 0) == 0)
        {
            chunk = chunk.substr(6);
        }

        auto j = nlohmann::json::parse(chunk);
        if (j.contains("choices"))
        {
            // 遍历所有choices并拼接content
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