#include "pch.h"
#include "json.hpp"
#include <fstream>
NAMEPACE_BEG(Scintilla)

class PlatformConf
{
public:
    // ���������ֶ�
    std::string _baseUrl;
    std::string _apiSkey;
    std::string _modelName;
    std::string _generateEndpoint;
    std::string _chatEndpoint;

    // ��json�ַ�����ȡ��������
    bool Load(const std::string& sdat);
};

// �˵����ýṹ��
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

// ƽ̨���ýṹ��
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

// ��������
class PluginConfig 
{
public:
    // ����������
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
            throw std::runtime_error("ƽ̨�б�Ϊ�գ������ò��ƽ̨");
        }
        return platforms.begin()->second;
    }

    // ��������
    void Load(const std::string& filename) 
    {
        try 
        {
            std::ifstream file(filename);
            nlohmann::json j;
            file >> j;

            // ������������
            platform = j.value("platform", "");
            timeout = j.value("timeout", 180);

            // ����ƽ̨����
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

    // ��������
    void Save(const std::string& filename) 
    {
        nlohmann::json j;

        // ��������
        j["platform"] = platform;
        j["timeout"] = timeout;

        // ƽ̨����
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

        // д���ļ�
        std::ofstream file(filename);
        file << j.dump(4);
    }
};

NAMEPACE_END