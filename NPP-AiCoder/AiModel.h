#pragma once
#include "PluginConf.h"
#include "PluginInterface.h"
#include "SimpleHttp.h"

class AiModel
{
public:
    // ��ʼ������
    AiModel(const Scintilla::PluginConfig& conf, const NppData& nppData)
        : _conf(conf), _nppData(nppData) {}

    // �������
    void ReadCode(const std::string& content) 
    {
        ExecuteAiTask(_conf.promt.read_code,
            content,
            _conf.Platform().chat_endpoint);
    }

    // �����Ż�
    void OptimizeCode(const std::string& content)
    {
        ExecuteAiTask(_conf.promt.optimize_code,
            content,
            _conf.Platform().chat_endpoint);
    }

    // ���ע��
    void AddComment(const std::string& content) 
    {
        ExecuteAiTask(_conf.promt.add_comment,
            content,
            _conf.Platform().chat_endpoint);
    }

    // ֱ������
    void DirectRequest(const std::string& request)
    {
        ExecuteAiTask(request,
            "",  // �޸�������
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
    // ƽ̨����ָ��
    const Scintilla::PluginConfig& _conf;
    // Notepad++����
    const NppData& _nppData;
};

