#pragma once
#include "PluginConf.h"
#include "PluginInterface.h"
#include "SimpleHttp.h"

class AiModel
{
public:
    using FnAppentOutput = std::function<void(const std::string&)>;
    using FnSetStatus = std::function<void(int)>;

    // 初始化配置
    AiModel(const Scintilla::PluginConfig& conf, const NppData& nppData)
        : _conf(conf), _nppData(nppData) {}

    void Request(const std::string& promt_name, const std::string& promt_para);
    std::string CreateJsonRequest(bool stream, const std::string& model, const std::string& content);

public:
    FnAppentOutput fnAppentOutput = nullptr;
    FnAppentOutput fnOutputFinished = nullptr;
    FnSetStatus fnSetStatus = nullptr;

protected:
    void ExecuteAiTask(const std::string& content, const Scintilla::EndpointConfig& endpoint);
    void ProcessStreamResponse(SimpleHttp& cli);

    /// <summary>
    /// 获取webapi授权header
    /// </summary>
    /// <param name="auth"></param>
    /// <param name="hkey"></param>
    /// <param name="hval"></param>
    /// <returns></returns>
    bool GetAuthorizationHeader(const Scintilla::AuthorizationConf& auth, std::string& hkey, std::string& hval);

private:
    // 平台配置指针
    const Scintilla::PluginConfig& _conf;
    // Notepad++数据
    const NppData& _nppData;
};

