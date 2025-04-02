#include "pch.h"
#include "json.hpp"
#include <fstream>
NAMEPACE_BEG(Scintilla)

class PlatformConf
{
public:
    // 基础功能字段
    std::string _baseUrl;
    std::string _apiSkey;
    std::string _modelName;
    std::string _generateEndpoint;
    std::string _chatEndpoint;

    // 从json字符串读取配置内容
    bool Load(const std::string& sdat);
};

// 端点配置结构体
struct EndpointConfig 
{
    std::string method = "post";
    std::string api;
    std::string prompt;

    EndpointConfig() = default;
    explicit EndpointConfig(const nlohmann::json& j) 
    {
        method = j.value("method", "post");
        api = j.value("api", "");
        prompt = j["prompt"].is_string() ? j["prompt"] : "";
    }
};

// 平台配置结构体
struct PlatformConfig {
    std::string base_url;
    std::string api_key;
    std::string model_name;
    EndpointConfig generate_endpoint;
    EndpointConfig chat_endpoint;
    EndpointConfig models_endpoint;
    std::vector<std::string> models;

    PlatformConfig() = default;
    explicit PlatformConfig(const nlohmann::json& j) 
    {
        base_url = j.value("base_url", "");
        api_key = j["api_key"].is_string() ? j["api_key"] : "";
        model_name = j.value("model_name", "");

        if (j.contains("generate_endpoint")) 
        {
            generate_endpoint = EndpointConfig(j["generate_endpoint"]);
        }
        if (j.contains("chat_endpoint")) 
        {
            chat_endpoint = EndpointConfig(j["chat_endpoint"]);
        }
        if (j.contains("models_endpoint")) 
        {
            models_endpoint = EndpointConfig(j["models_endpoint"]);
        }
        models = j.value("models", std::vector<std::string>());
    }
};

// 主配置类
class PluginConfig 
{
public:
    // 公共配置项
    std::string platform;
    int timeout = 180;
    std::map<std::string, PlatformConfig> platforms;

    const PlatformConfig& Platform()
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
    void Load(const std::string& filename) 
    {
        try 
        {
            std::ifstream file(filename);
            nlohmann::json j;
            file >> j;

            // 解析基础配置
            platform = j.value("platform", "");
            timeout = j.value("timeout", 180);

            // 解析平台配置
            platforms.clear();
            if (j.contains("platforms") && j["platforms"].is_object()) 
            {
                for (auto& [key, value] : j["platforms"].items()) 
                {
                    platforms.emplace(key, PlatformConfig(value));
                }
            }
        }
        catch (const std::exception& e) 
        {
            throw std::runtime_error("Load config failed: " + std::string(e.what()));
        }
    }

    // 保存配置
    void Save(const std::string& filename) 
    {
        nlohmann::json j;

        // 基础配置
        j["platform"] = platform;
        j["timeout"] = timeout;

        // 平台配置
        nlohmann::json platforms_json;
        for (auto& [name, config] : platforms) 
        {
            nlohmann::json platform_json;
            platform_json["base_url"] = config.base_url;
            platform_json["api_key"] = config.api_key.empty() ? nullptr : config.api_key;
            platform_json["model_name"] = config.model_name;

            platform_json["generate_endpoint"]["method"] = config.generate_endpoint.method;
            platform_json["generate_endpoint"]["api"] = config.generate_endpoint.api;
            platform_json["generate_endpoint"]["prompt"] = config.generate_endpoint.prompt.empty() ? nullptr : config.generate_endpoint.prompt;

            platform_json["chat_endpoint"]["method"] = config.chat_endpoint.method;
            platform_json["chat_endpoint"]["api"] = config.chat_endpoint.api;
            platform_json["chat_endpoint"]["prompt"] = config.chat_endpoint.prompt.empty() ? nullptr : config.chat_endpoint.prompt;

            platform_json["models_endpoint"]["method"] = config.models_endpoint.method;
            platform_json["models_endpoint"]["api"] = config.models_endpoint.api;
            platform_json["models_endpoint"]["prompt"] = config.models_endpoint.prompt.empty() ? nullptr : config.models_endpoint.prompt;

            platform_json["models"] = config.models;

            platforms_json[name] = platform_json;
        }
        j["platforms"] = platforms_json;

        // 写入文件
        std::ofstream file(filename);
        file << j.dump(4);
    }
};

NAMEPACE_END