#include "pch.h"
#include "AiModel.h"
#include "SimpleHttp.h"
#include "Utils.h"
#include "json.hpp"
#include "NppImp.h"
#include "PluginDefinition.h"

using namespace Scintilla;

void AiModel::Request(const std::string& promt_name, const std::string& promt_para)
{
    auto promt = _conf.promt.Format(promt_name, promt_para);
    ExecuteAiTask(promt, _conf.Platform().chat_endpoint);
}

bool AiModel::GetAuthorizationHeader(const AuthorizationConf& auth, std::string& hkey, std::string& hval)
{
    hkey = "";
    hval = "";
    const std::string& auth_data = auth.auth_data;
    switch (auth.eAuthType) 
    {
        // 无需处理授权信息
    case AuthorizationConf::AuthType::None:
        return true;
    case AuthorizationConf::AuthType::Basic: 
        {
            // Basic认证需要"username:password"格式
            size_t colonPos = auth_data.find(':');
            if (colonPos == std::string::npos) return false;

            std::string encoded = String::Base64Encode(auth_data);
            hkey = "Authorization";
            hval = "Basic " + encoded;
            return true;
        }
    case AuthorizationConf::AuthType::Bearer:
        {
            if (auth_data.empty()) return false;
            hkey = "Authorization";
            hval = "Bearer " + auth_data;
            return true;
        }
    case AuthorizationConf::AuthType::ApiKey: 
        {
            if (auth.eDeliveryType == AuthorizationConf::DeliveryType::Header) 
            {
                // 格式要求："HeaderName: Value" 或直接传递值（自动添加X-API-Key头）
                size_t colonPos = auth_data.find(':');
                std::string header;

                if (colonPos != std::string::npos) 
                {
                    // 用户自定义头格式
                    hkey = String::Trim(auth_data.substr(0, colonPos));
                    hval = String::Trim(auth_data.substr(colonPos + 1));
                }
                else 
                {
                    // 默认使用X-API-Key头
                    hkey = "X-API-Key";
                    hval = auth_data;
                }
                return true;
            }
            // 参数传递方式需要在URL构造阶段处理，此处无法支持
            return false;
        }
    default:
        return false;
    }
    return true;
}

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
void AiModel::ExecuteAiTask(const std::string& content, const Scintilla::EndpointConfig& endpoint)
{
    try
    {
        if (fnSetStatus) fnSetStatus(TASK_STATUS_BEGIN);
        auto& plat = _conf.Platform();
        // 创建HTTP客户端
        SimpleHttp cli(plat.base_url, plat.enable_ssl);

        // 设置请求头
        std::unordered_map<std::string, std::string> headers = {
            {"Content-Type", "application/json; charset=UTF-8"}
        };
        std::string authName, authValue;
        if (GetAuthorizationHeader(plat.authorization, authName, authValue) && !authName.empty())
        {
            headers[authName] = authValue;
        }
        cli.SetHeaders(headers);

        // 构建完整提示词
        spdlog::info("request:{}{} model:{} message:{}",
            plat.base_url.c_str(),
            plat.chat_endpoint.api.c_str(),
            plat.model_name.c_str(),
            content.c_str()
        );
        std::string promt = Scintilla::String::GBKToUTF8(content.c_str());

        // 创建JSON请求
        std::string requestJson = CreateJsonRequest(
            true,  // 流式
            plat.model_name,
            promt
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
    catch (const std::exception& e)
    {
        ShowMsgBox(String::Format("接口调用异常:%s", e.what()).c_str());
    }
    if (fnOutputFinished) fnOutputFinished("\r\n");
    if (fnSetStatus) fnSetStatus(TASK_STATUS_FINISH);
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
        if (fnAppentOutput)
        {
            fnAppentOutput(text);
            return;
        }
        call.AddText(text.size(), text.c_str());
        call.ScrollCaret();
    };

    // 创建打字机效果处理器
    Scintilla::Typewriter writer(fetchFn, outputFn);

    // 添加初始换行
    if(fnAppentOutput == nullptr) outputFn("\r\n\r\n");

    // 启动处理流程
    writer.Run();

    // 添加结束换行
    if (fnAppentOutput == nullptr)
    {
        outputFn("\r\n\r\n");
    }
    fnAppentOutput = nullptr;
}