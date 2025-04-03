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

// 提示词
struct PromtConfig : public IConfig
{
    std::string read_code;
    std::string optimize_code;
    std::string add_comment;

    void SetDefault();

    virtual void from_json(const nlohmann::json& j) override
    {
        Util::JsonGet(j, "read_code", read_code);
        Util::JsonGet(j, "optimize_code", optimize_code);
        Util::JsonGet(j, "add_comment", add_comment);
    }
    virtual void to_json(nlohmann::json& j) override
    {

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
    std::string api_key;
    std::string model_name;
    EndpointConfig generate_endpoint;
    EndpointConfig chat_endpoint;
    EndpointConfig models_endpoint;
    std::vector<std::string> models;

    virtual void from_json(const nlohmann::json& j) override
    {
        Util::JsonGet(j, "enable_ssl", enable_ssl);
        Util::JsonGet(j, "base_url", base_url);
        Util::JsonGet(j, "api_key", api_key);
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
    void Load(const std::string& filename);

    // 保存配置
    void Save(const std::string& filename);
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