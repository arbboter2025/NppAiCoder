#pragma once
#include "pch.h"
#include "Utils.h"
#include <fstream>
NAMEPACE_BEG(Scintilla)


class IConfig
{
public:
    virtual void from_json(const nlohmann::json& j) = 0;
    virtual void to_json(nlohmann::json& j) = 0;
};



class AuthorizationConf : public IConfig
{
public:
    // 授权类型
    enum class AuthType
    {
        // 无
        None,
        // 基础：用户名+密码
        Basic,
        // Bearer Token
        Bearer,
        // 使用api key方式，支持请求头和参数传递
        ApiKey
    };
    // 参数传递方式
    enum class DeliveryType
    {
        // 请求头
        Header,
        // 参数
        Para
    };

    AuthType eAuthType = AuthType::None;
    DeliveryType eDeliveryType = DeliveryType::Header;
    // 配置项数据，根据不同的授权类型设置不同的数据格式内容
    std::string auth_data;

    static AuthType GetAuthType(const std::string& authType)
    {
        AuthType eType = AuthType::None;
        if (!String::icasecompare(authType, "Basic"))
        {
            eType = AuthType::Basic;
        }
        if (!String::icasecompare(authType, "Bearer"))
        {
            eType = AuthType::Bearer;
        }
        if (!String::icasecompare(authType, "ApiKey"))
        {
            eType = AuthType::ApiKey;
        }
        return eType;
    }

    static std::string GetAuthType(AuthType eType)
    {
        std::string name = "";
        switch (eType)
        {
        case AuthType::None:
            name = "None";
            break;
        case AuthType::Basic:
            name = "Basic";
            break;
        case AuthType::Bearer:
            name = "Bearer";
            break;
        case AuthType::ApiKey:
            name = "ApiKey";
            break;
        default:
            break;
        }
        return name;
    }

    virtual void from_json(const nlohmann::json& j) override
    {
        std::string strVal;
        if (Util::JsonGet(j, "type", strVal))
        {
            eAuthType = GetAuthType(strVal);
        }
        Util::JsonGet(j, "data", auth_data);
    }

    virtual void to_json(nlohmann::json& j) override
    {

    }
};

// 提示词
struct PromtConfig : public IConfig
{
    std::string read_code;
    std::string optimize_code;
    std::string add_comment;
    std::string format_code;
    std::map<std::string, std::string> exts;

    void SetDefault();

    virtual void from_json(const nlohmann::json& j) override
    {
        Util::JsonGet(j, "read_code", read_code);
        Util::JsonGet(j, "optimize_code", optimize_code);
        Util::JsonGet(j, "add_comment", add_comment);
        Util::JsonGet(j, "format_code", format_code);
        if (j.contains("exts") && j["exts"].is_object())
        {
            for (auto& [key, value] : j["exts"].items())
            {
                if (key.empty() || !value.is_string()) continue;
                exts[key] = value.get<std::string>();
            }
        }
    }

    virtual void to_json(nlohmann::json& j) override
    {

    }

    std::string Format(const std::string& name, const std::string& content, bool utf8 = false) const
    {
        std::string promt;
        if (name == "read_code")
        {
            promt = read_code;
        }
        else if (name == "optimize_code")
        {
            promt = optimize_code;
        }
        if (name == "add_comment")
        {
            promt = add_comment;
        }
        if (name == "format_code")
        {
            promt = format_code;
        }
        if (promt.empty())
        {
            auto e = exts.find(name);
            if (e != exts.end())
            {
                promt = e->second;
            }
        }
        if (promt.empty())
        {
            return content;
        }
        if (!utf8) promt = String::UTF8ToGBK(promt.c_str());
        const std::string tagContent = "{0}";
        size_t pos = promt.find(tagContent);
        if (pos != std::string::npos)
        {
            return promt.substr(0, pos) + content + promt.substr(pos + tagContent.size());
        }
        return promt + "\n" + content;
    }
};

// 端点配置结构体
struct EndpointConfig : public IConfig
{
    std::string method = "post";
    std::string api;
    std::string prompt;

    virtual void from_json(const nlohmann::json& j) override
    {
        Util::JsonGet(j, "method", method);
        Util::JsonGet(j, "api", api);
        Util::JsonGet(j, "prompt", prompt);
    }
    virtual void to_json(nlohmann::json& j) override
    {

    }
};

// 平台配置结构体
struct PlatformConfig : public IConfig
{
    bool enable_ssl;
    std::string base_url;
    AuthorizationConf authorization;
    std::string model_name;
    EndpointConfig generate_endpoint;
    EndpointConfig chat_endpoint;
    EndpointConfig models_endpoint;
    std::vector<std::string> models;

    virtual void from_json(const nlohmann::json& j) override
    {
        Util::JsonGet(j, "enable_ssl", enable_ssl);
        Util::JsonGet(j, "base_url", base_url);
        Util::JsonGet(j, "model_name", model_name);
        if (j.contains("generate_endpoint") && j["generate_endpoint"].is_object())
        {
            generate_endpoint.from_json(j["generate_endpoint"]);
        }
        if (j.contains("chat_endpoint") && j["chat_endpoint"].is_object())
        {
            chat_endpoint.from_json(j["chat_endpoint"]);
        }
        if (j.contains("models_endpoint") && j["models_endpoint"].is_object())
        {
            models_endpoint.from_json(j["models_endpoint"]);
        }
        if (j.contains("authorization") && j["authorization"].is_object())
        {
            authorization.from_json(j["authorization"]);
        }
        Util::JsonGet(j, "models", models);
    }
    virtual void to_json(nlohmann::json& j) override
    {

    }
};

// 主配置类
class PluginConfig
{
public:
    // 公共配置项
    std::string platform;
    int timeout = 180;
    PromtConfig promt;
    std::map<std::string, PlatformConfig> platforms;

    const PlatformConfig& Platform() const
    {
        auto e = platforms.find(platform);
        if (e != platforms.end())
        {
            return e->second;
        }
        if (platforms.empty())
        {
            throw std::runtime_error("平台列表为空，请配置插件平台");
        }
        return platforms.begin()->second;
    }

    // 加载配置
    bool Load(const std::string& filename);

    // 保存配置
    bool Save(const std::string& filename = "");

private:
    std::string m_confFile;
};

NAMEPACE_END


namespace nlohmann
{
    template <>
    struct adl_serializer<Scintilla::PromtConfig>
    {
        static void from_json(const json& j, Scintilla::PromtConfig& data)
        {
            j.at("read_code").get_to(data.read_code);
            j.at("optimize_code").get_to(data.optimize_code);
            j.at("add_comment").get_to(data.add_comment);
        }
    };

    template <>
    struct adl_serializer<Scintilla::EndpointConfig>
    {
        static void from_json(const json& j, Scintilla::EndpointConfig& data)
        {
            j.at("method").get_to(data.method);
            j.at("api").get_to(data.api);
            j.at("prompt").get_to(data.prompt);
        }
    };
}