#pragma once
#include "pch.h"
#include "json.hpp"
#include <Windows.h>
#include <string>
#include <fstream>
#include <fstream>
#include <functional>


NAMEPACE_BEG(Scintilla)


class Util
{
public:
    static std::shared_ptr<nlohmann::json> JsonLoadFile(const std::string& path);
    static bool JsonLoad(const std::string& sdat, nlohmann::json& jdat);

    template<typename T>
    static bool JsonGet(const nlohmann::json& jdat, const std::string& name, T& val)
    {
        // 使用contains检查键存在性（避免异常）
        if (!jdat.contains(name))
        {
            return false;
        }
        try 
        {
            val = jdat[name].get<T>();
            return true;
        }
        catch (...) 
        {
            return false;
        }
    }

    template<typename T>
    static bool JsonGetByPath(const nlohmann::json& jdat, const std::vector<std::string>& paths, T& val)
    {
        for(size_t i=0; i < paths.size(); i++)
        {
            auto& k = paths[i];
            if (!jdat.contains(k) || jdat[k].is_null())
            {
                return false;
            }
            jdat = jdat[k];
        }
    }


};

class Json
{
public:
    bool LoadFile(const std::string& path)
    {
        _pJson = Util::JsonLoadFile(path);
        return _pJson != nullptr;
    }

    nlohmann::json& Inst() { return *_pJson; }

private:
    std::shared_ptr<nlohmann::json> _pJson = nullptr;
};

class File
{
public:
    static bool ReadFile(const std::string& filename, std::string& content);
};

class String 
{
public:
    static std::string ConvEncoding(const char* pSrc, size_t nLen, unsigned int dwScp, unsigned int dwDcp);
    static std::string GBKToUTF8(const char* pSrc, size_t nLen = 0);
    static std::string UTF8ToGBK(const char* pSrc, size_t nLen = 0);
    static std::string Trim(const std::string& sin, const std::string& chars = " \r\n\t", bool left = true, bool right = true);
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
    bool ExtractContent(AiRespType arType, std::string& resp, std::string& content, bool& finished);

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
    void Run();

private:
    FNRead m_getter;
    FNWrite m_writer;
};

NAMEPACE_END