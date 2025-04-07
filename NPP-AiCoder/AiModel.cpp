#include "pch.h"
#include "AiModel.h"
#include "SimpleHttp.h"
#include "Utils.h"
#include "json.hpp"
#include "NppImp.h"
#include "PluginDefinition.h"

using namespace Scintilla;

std::string AiModel::CreateJsonRequest(bool stream, const std::string& model, const std::string& content)
{
    nlohmann::json req;
    req["stream"] = stream;
    req["model"] = model;

    // ������Ϣ�壨�Զ����������ַ�ת�壩[[4]]
    req["messages"] = nlohmann::json::array({
        {
            {"role", "user"},
            {"content", content}
        }
        });
    return req.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

// ͳһִ��AI����ĺ��ķ���
void AiModel::ExecuteAiTask(const std::string& promptTemplate,
    const std::string& content,
    const Scintilla::EndpointConfig& endpoint)
{
    auto& plat = _conf.Platform();
    // ����HTTP�ͻ���
    SimpleHttp cli(plat.base_url, plat.enable_ssl);

    // ��������ͷ
    cli.SetHeaders({
        {"Content-Type", "application/json; charset=UTF-8"},
        {"Authorization", plat.api_key}
    });

    // ����������ʾ�ʣ������ݲ���ģ�壩
    std::string fullPrompt = FormatPrompt(promptTemplate, content);

    // ����JSON����
    std::string requestJson = CreateJsonRequest(
        true,  // ��ʽ
        plat.model_name,
        Scintilla::String::GBKToUTF8(content.c_str())
    );

    std::string requestJsonGbk = CreateJsonRequest(
        true,  // ��ʽ
        plat.model_name,
        content
    );

    // ��������
    std::string resp;

    if (cli.Post(endpoint.api, requestJson, resp, true)) {
        MessageBox(_nppData._scintillaMainHandle,
            L"AI�������ʧ��",
            L"����",
            MB_OK);
        return;
    }

    // ������Ӧ����
    ProcessStreamResponse(cli);
}

// ��ʽ����ʾ��ģ��
std::string AiModel::FormatPrompt(const std::string& templateStr, const std::string& content)
{
    size_t pos = templateStr.find("{}");
    if (pos != std::string::npos)
    {
        return std::vformat(templateStr, std::make_format_args(content));
    }
    return templateStr + "\n\n" + content;
}

// ������ʽ��Ӧ
void AiModel::ProcessStreamResponse(SimpleHttp& cli) {
    Scintilla::ScintillaCall call;
    call.SetFnPtr((intptr_t)_nppData._scintillaMainHandle);

    // ��ʽ��ȡ�ص�
    auto fetchFn = std::bind(&SimpleHttp::TryFetchResp, &cli,
        std::placeholders::_1);

    // �ı�����ص�
    auto outputFn = [&](const std::string& text) {
        call.AddText(text.size(), text.c_str());
        call.ScrollCaret();
    };

    // �������ֻ�Ч��������
    Scintilla::Typewriter writer(fetchFn, outputFn);

    // ��ӳ�ʼ����
    outputFn("\r\n\r\n");

    // ������������
    writer.Run();

    // ��ӽ�������
    outputFn("\r\n\r\n");
}