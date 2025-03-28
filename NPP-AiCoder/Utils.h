#pragma once
#include "pch.h"
#include <Windows.h>
#include <string>
#include <fstream>
#include <functional>


NAMEPACE_BEG(Scintilla)

class File
{
public:
    static bool ReadFile(const std::string& filename, std::string& content) 
    {
        // 以二进制模式打开文件（避免换行符转换）
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) 
        {
            return false;
        }

        // 使用流迭代器读取整个文件内容
        content.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );

        // 检查是否成功读取到文件末尾
        return file.eof();
    }
};

class String 
{
public:
    static std::string ConvEncoding(const char* pSrc, size_t nLen, unsigned int dwScp, unsigned int dwDcp);
    static std::string GBKToUTF8(const char* pSrc, size_t nLen = 0);
    static std::string UTF8ToGBK(const char* pSrc, size_t nLen = 0);
    static std::string Trim(const std::string& sin, const std::string& chars = " \r\n\t", bool left = true, bool right = true);
};


enum class AiRespType
{
    OPENAI_TOTAL_RESP,
    OPENAI_STREAM_RESP,
};


class ParseResult
{
public:
    /// <summary>
    /// 解析出应答包的数据内容
    /// </summary>
    /// <param name="arType">Ai模型返回的数据包类型</param>
    /// <param name="resp">ai模型返回的数据包类型内容，可能有多个包，换行符分割</param>
    /// <param name="resp">首包内容</param>
    /// <param name="finished">是否结束</param>
    /// <returns></returns>
    bool ExtractContent(AiRespType arType, std::string& resp, std::string& content, bool& finished)
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

private:
    bool ParseTotalResponse(const std::string& jsonStr, std::string& content);
    bool ParseStreamResponse(std::string& resp, std::string& content, bool& finished);
};


class Typewriter
{
public:
    using FNRead = std::function<int(std::string&)>;
    using FNWrite = std::function<void(const std::string&)>;

    Typewriter(FNRead getter, FNWrite setter) : m_getter(getter), m_writer(setter) {}

    void Run()
    {
        ParseResult pr;
        std::string text;
        std::string last;
        std::string content;
        bool finished = false;
        for(int nRet = 0; (nRet = m_getter(text)) != 0; )
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

private:
    FNRead m_getter;
    FNWrite m_writer;
};

NAMEPACE_END