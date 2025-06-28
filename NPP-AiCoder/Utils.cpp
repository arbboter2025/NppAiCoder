#include "pch.h"
#include "Utils.h"
#include <windows.h>
#include <Wincrypt.h>
#include <stdexcept>
#include <memory>
#include <filesystem>
#include <chrono>
#include <format>
#include <string>
#include <system_error>
#include "json.hpp"

using namespace Scintilla;


#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "Version.lib")

std::string Util::GetBuildDate()
{
    char buffer[16] = { 0 };
    const char* months[] = { "Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec" };
    // 解析__DATE__的格式（示例："Apr 20 2025"）
    std::vector<std::string> items;
    // 匿名函数实现 Split 功能
    [](const std::string& str, const std::string& delim, std::vector<std::string>& items, bool ignore_empty) {
        items.clear();
        size_t pos = 0;
        while (pos < str.size()) {
            size_t next = str.find(delim, pos);
            if (next == std::string::npos) {
                std::string token = str.substr(pos);
                if (!(ignore_empty && token.empty())) {
                    items.push_back(token);
                }
                break;
            }
            std::string token = str.substr(pos, next - pos);
            if (!(ignore_empty && token.empty())) {
                items.push_back(token);
            }
            pos = next + delim.size();
        }
    }(__DATE__, " ", items, true);
    if (items.size() != 3)
    {
        memcpy(buffer, "20250520", 8);
        return buffer;
    }
    int day = atoi(items[1].c_str());
    int year = atoi(items[2].c_str());

    // 将月份缩写转换为数字
    int month = 0;
    for (int i = 0; i < 12; ++i)
    {
        if (strcmp(items[0].c_str(), months[i]) == 0)
        {
            month = i + 1;
            break;
        }
    }

    // 格式化为YYYYMMDD
    snprintf(buffer, sizeof(buffer), "%04d%02d%02d", year, month, day);
    return buffer;
}

std::string Util::GetCurrentDate(const char* fmt)
{
    // 获取当前系统时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    // 转换为本地时间（线程安全版本）
    struct tm tm_struct;
#if defined(_WIN32)
    localtime_s(&tm_struct, &now_time);    // Windows 版本
#else
    localtime_r(&now_time, &tm_struct);    // Linux/macOS 版本
#endif

// 设置默认格式或使用自定义格式
    const char* actual_fmt = (fmt != nullptr) ? fmt : "%Y%m%d";

    // 格式化日期字符串
    char buffer[128];
    std::strftime(buffer, sizeof(buffer), actual_fmt, &tm_struct);

    return std::string(buffer);
}
bool Scintilla::Util::FileExist(const char* file)
{
    if (file == nullptr || file[0] == '\0') return false;
    FILE* fp = nullptr;
    auto err = fopen_s(&fp, file, "r");
    if (err || fp == nullptr)
    {
        return false;
    }
    fclose(fp);
    return true;
}

std::string Scintilla::Util::GetVersionInfo(const std::string& name)
{
    std::string info;

    if (g_hModule == nullptr)
    {
        return info;
    }

    char path[MAX_PATH] = { 0 };
    GetModuleFileNameA(g_hModule, path, MAX_PATH);

    DWORD dummy;
    DWORD size = GetFileVersionInfoSizeA(path, &dummy);
    if (size == 0)
    {
        return info;
    }

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoA(path, 0, size, buffer.data()))
    {
        return info;
    }

    // 获取语言和代码页信息
    UINT translateLen = 0;
    LPVOID translatePtr = nullptr;
    if (!VerQueryValueA(buffer.data(), "\\VarFileInfo\\Translation", &translatePtr, &translateLen))
    {
        return info;
    }

    // 构造版本信息路径（使用第一个语言代码页）
    if (translateLen >= sizeof(DWORD))
    {
        DWORD langCodepage = *static_cast<DWORD*>(translatePtr);
        char subBlock[256] = { 0 };
        snprintf(
            subBlock,
            sizeof(subBlock),
            "\\StringFileInfo\\%04x%04x\\%s",
            LOWORD(langCodepage),
            HIWORD(langCodepage),
            name.c_str()
        );

        // 获取指定字段的版本信息
        LPVOID value = nullptr;
        UINT valueLen = 0;
        if (VerQueryValueA(buffer.data(), subBlock, &value, &valueLen) && valueLen > 0)
        {
            info = std::string(static_cast<const char*>(value), valueLen - 1); // 去除末尾空字符
        }
    }
    return info;
}

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
    const std::string span = "\n\n";
    size_t newlinePos = resp.find(span);
    std::string chunk = resp;
    const bool bFirst = (newlinePos != std::string::npos);
    if (bFirst)
    {
        chunk = resp.substr(0, newlinePos);
        resp = resp.substr(newlinePos + span.size());
    }

    try
    {
        finished = false;
        content = "";
        // 去除"data: "前缀
        const std::string prefix = "data: ";
        size_t pi = chunk.find(prefix);
        if (pi != std::string::npos)
        {
            chunk = chunk.substr(pi + prefix.size());
            if (!bFirst) resp = chunk;
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
        else
        {
            content = chunk;
            finished = true;
        }
        bRet = true;
        if (!bFirst)
        {
            resp = "";
        }
    }
    catch (const nlohmann::json::parse_error&)
    {

    }
    catch (const nlohmann::json::exception&)
    {

    }
    return bRet;
}

bool File::ReadFile(const std::string& filename, std::string& content)
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

std::string String::Base64Encode(const std::string& input)
{
    DWORD len = 0;
    if (!CryptBinaryToStringA(
        reinterpret_cast<const BYTE*>(input.c_str()),
        static_cast<DWORD>(input.size()),
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        nullptr,
        &len
    )) 
    {
        return "";
    }

    std::vector<char> buf(len);
    if (!CryptBinaryToStringA(
        reinterpret_cast<const BYTE*>(input.c_str()),
        static_cast<DWORD>(input.size()),
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        buf.data(),
        &len
    )) 
    {
        return "";
    }

    return std::string(buf.data(), len - 1);
}

bool Util::Mkdir(const std::string& filepath)
{
    namespace fs = std::filesystem;
    fs::path dir = fs::path(filepath).parent_path();

    if (dir.empty()) 
    {
        return true;
    }

    std::error_code ec;
    fs::create_directories(dir, ec);

    // 检查是否创建成功或目录已存在
    return !ec && fs::is_directory(dir, ec);
}

std::string String::Format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    int size = vsnprintf(nullptr, 0, fmt, args);
    va_end(args);

    if (size < 0) 
    {
        throw std::runtime_error("Format error: vsnprintf failed");
    }

    // 分配缓冲区（包含终止符'\0'）
    std::vector<char> buf(static_cast<size_t>(size) + 1);

    va_start(args, fmt);
    vsnprintf(buf.data(), buf.size(), fmt, args);
    va_end(args);

    return std::string(buf.data(), buf.data() + size);
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
    std::string pack;
    std::string text;
    std::string content;
    bool finished = false;
    size_t nToken = 0;
    bool bRet = false;
    std::stringstream ss;
    for (int nRet = 0; !finished && g_bRun.load(); )
    {
        if (m_getter(pack) <= 0 || pack.empty())
        {
            Sleep(50);
            continue;
        }

        text += pack;
        while (g_bRun.load() && !finished && !text.empty())
        {
            bRet = pr.ExtractContent(AiRespType::OPENAI_STREAM_RESP, text, content, finished);
            if (!bRet)
            {
                break;
            }
            if (content.empty() || (!nToken && Scintilla::String::Trim(content).empty()))
            {
                continue;
            }
            if (nToken++ == 0)
            {
                content = Scintilla::String::Trim(content);
            }
            ss << content;
            m_writer(content);
            Sleep(50);
        }
    }
    content = ss.str();
    spdlog::info(Scintilla::String::UTF8ToGBK(content.c_str(), content.size()));
}