#include "pch.h"
#include "PluginConf.h"

template<typename T>
bool JsonGet(const nlohmann::json& jdat, const std::string& name, T& val)
{
    // ʹ��contains���������ԣ������쳣��[[3]]
    if (!jdat.contains(name))
    {
        return false;
    }

    try {
        // ͨ��at()��ȫ���ʲ�ǿ������ת��[[5]][[6]]
        val = jdat.at(name).get<T>();
        return true;
    }
    catch (const nlohmann::json::type_error&) {
        // �������Ͳ�ƥ���쳣[[5]]
        return false;
    }
    catch (const nlohmann::json::exception&) {
        // ��������JSON�쳣[[5]]
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