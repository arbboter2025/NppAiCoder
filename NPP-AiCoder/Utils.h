#pragma once
#include "pch.h"
#include "json.hpp"
#include <Windows.h>
#include <string>
#include <fstream>
#include <fstream>
#include <functional>
#include <algorithm>


NAMEPACE_BEG(Scintilla)


class Util
{
public:
    static std::string GetBuildDate();
    static std::string GetCurrentDate(const char* fmt = nullptr);
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

    static bool FileExist(const char* file);
    static std::string GetVersionInfo(const std::string& name);
    static bool Mkdir(const std::string& filepath);
};

class Json
{
public:
    bool LoadFile(const std::string& path)
    {
        _pJson = Util::JsonLoadFile(path);
        return _pJson != nullptr;
    }

    static bool IsValid(const std::string& data)
    {
        try 
        {
            auto j = nlohmann::json::parse(data);
            return true;
        } 
        catch (const nlohmann::json::parse_error&) 
        {
            return false;
        }
        catch (const nlohmann::json::exception&) 
        {
            return false;
        }
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

    static std::string Base64Encode(const std::string& input);

    // 去空
    static std::string TrimAll(const std::string& str) {
        const char* whitespace = " \t\n\r\f\v";
        size_t start = str.find_first_not_of(whitespace);
        if (start == std::string::npos) return "";

        size_t end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }

    // std::wstring → std::string (UTF-8)
    static std::string wstring2s(const std::wstring& wstr, bool bUtf8 = true) {
        if (wstr.empty()) return {};
        UINT nCodePage = bUtf8 ? CP_UTF8 : CP_ACP;
        int len = WideCharToMultiByte(nCodePage, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
        std::string result(len, '\0');
        WideCharToMultiByte(nCodePage, 0, wstr.data(), (int)wstr.size(), (LPSTR)result.data(), len, nullptr, nullptr);
        return result;
    }

    // std::string (UTF-8) → std::wstring
    static std::wstring string2w(const std::string& str, bool bUtf8 = true) {
        if (str.empty()) return {};
        UINT nCodePage = bUtf8 ? CP_UTF8 : CP_ACP;
        int len = MultiByteToWideChar(nCodePage, 0, str.data(), (int)str.size(), nullptr, 0);
        std::wstring result(len, L'\0');
        MultiByteToWideChar(nCodePage, 0, str.data(), (int)str.size(), (LPWSTR)result.data(), len);
        return result;
    }

    // 忽略大小写比较
    static int icasecompare(const std::string& str1, const std::string& str2) {
        // 拷贝临时对象，算法在临时对象上作更改
        std::string s1(str1);
        std::string s2(str2);
        std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
        std::transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
        return s1.compare(s2);
    }

    static std::string Format(const char* fmt, ...);
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