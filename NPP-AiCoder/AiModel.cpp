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

    // 构建消息体（自动处理特殊字符转义）[[4]]
    req["messages"] = nlohmann::json::array({
        {
            {"role", "user"},
            {"content", content}
        }
        });
    return req.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
}

// 统一执行AI请求的核心方法
void AiModel::ExecuteAiTask(const std::string& promptTemplate,
    const std::string& content,
    const Scintilla::EndpointConfig& endpoint)
{
    auto& plat = _conf.Platform();
    // 创建HTTP客户端
    SimpleHttp cli(plat.base_url, plat.enable_ssl);

    // 设置请求头
    cli.SetHeaders({
        {"Content-Type", "application/json; charset=UTF-8"},
        {"Authorization", plat.api_key}
    });

    // 构建完整提示词（将内容插入模板）
    std::string fullPrompt = FormatPrompt(promptTemplate, content);

    // 创建JSON请求
    std::string requestJson = CreateJsonRequest(
        true,  // 流式
        plat.model_name,
        Scintilla::String::GBKToUTF8(content.c_str())
    );

    std::string requestJsonGbk = CreateJsonRequest(
        true,  // 流式
        plat.model_name,
        content
    );

    // 发送请求
    std::string resp;

    if (cli.Post(endpoint.api, requestJson, resp, true)) {
        MessageBox(_nppData._scintillaMainHandle,
            L"AI服务调用失败",
            L"错误",
            MB_OK);
        return;
    }

    // 设置响应处理
    ProcessStreamResponse(cli);
}

// 格式化提示词模板
std::string AiModel::FormatPrompt(const std::string& templateStr, const std::string& content)
{
    size_t pos = templateStr.find("{}");
    if (pos != std::string::npos)
    {
        return std::vformat(templateStr, std::make_format_args(content));
    }
    return templateStr + "\n\n" + content;
}

// 处理流式响应
void AiModel::ProcessStreamResponse(SimpleHttp& cli) {
    Scintilla::ScintillaCall call;
    call.SetFnPtr((intptr_t)_nppData._scintillaMainHandle);

    // 流式获取回调
    auto fetchFn = std::bind(&SimpleHttp::TryFetchResp, &cli,
        std::placeholders::_1);

    // 文本输出回调
    auto outputFn = [&](const std::string& text) {
        call.AddText(text.size(), text.c_str());
        call.ScrollCaret();
    };

    // 创建打字机效果处理器
    Scintilla::Typewriter writer(fetchFn, outputFn);

    // 添加初始换行
    outputFn("\r\n\r\n");

    // 启动处理流程
    writer.Run();

    // 添加结束换行
    outputFn("\r\n\r\n");
}