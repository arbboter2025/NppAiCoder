#include "pch.h"
#include "PluginConf.h"

template<typename T>
bool JsonGet(const nlohmann::json& jdat, const std::string& name, T& val)
{
    // 使用contains检查键存在性（避免异常）[[3]]
    if (!jdat.contains(name))
    {
        return false;
    }

    try {
        // 通过at()安全访问并强制类型转换[[5]][[6]]
        val = jdat.at(name).get<T>();
        return true;
    }
    catch (const nlohmann::json::type_error&) {
        // 处理类型不匹配异常[[5]]
        return false;
    }
    catch (const nlohmann::json::exception&) {
        // 处理其他JSON异常[[5]]
        return false;
    }
}

bool Scintilla::PlatformConf::Load(const std::string& sdat)
{
    try
    {
        auto j = nlohmann::json::parse(sdat);

        JsonGet(j, "base_url", _baseUrl);
        JsonGet(j, "api_key", _apiSkey);
        JsonGet(j, "model_name", _modelName);
        JsonGet(j, "generate_endpoint", _generateEndpoint);
        JsonGet(j, "chat_endpoint", _chatEndpoint);
        return true;
    }
    catch (const nlohmann::json::exception&)
    {
        return false;
    }
}