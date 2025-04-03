#pragma once
#include "PluginConf.h"
#include "PluginInterface.h"
#include "SimpleHttp.h"

class AiModel
{
public:
    // 初始化配置
    AiModel(const Scintilla::PluginConfig& conf, const NppData& nppData)
        : _conf(conf), _nppData(nppData) {}

    // 解读代码
    void ReadCode(const std::string& content) 
    {
        ExecuteAiTask(_conf.promt.read_code,
            content,
            _conf.Platform().chat_endpoint);
    }

    // 代码优化
    void OptimizeCode(const std::string& content)
    {
        ExecuteAiTask(_conf.promt.optimize_code,
            content,
            _conf.Platform().chat_endpoint);
    }

    // 添加注释
    void AddComment(const std::string& content) 
    {
        ExecuteAiTask(_conf.promt.add_comment,
            content,
            _conf.Platform().chat_endpoint);
    }

    // 直接提问
    void DirectRequest(const std::string& request)
    {
        ExecuteAiTask(request,
            "",  // 无附加内容
            _conf.Platform().chat_endpoint);
    }

    std::string CreateJsonRequest(bool stream, const std::string& model, const std::string& content);

protected:
    void ExecuteAiTask(const std::string& promptTemplate,
        const std::string& content,
        const Scintilla::EndpointConfig& endpoint);

    std::string FormatPrompt(const std::string& templateStr, const std::string& content);
    void ProcessStreamResponse(SimpleHttp& cli);

private:
    // 平台配置指针
    const Scintilla::PluginConfig& _conf;
    // Notepad++数据
    const NppData& _nppData;
};

