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
        // ���账����Ȩ��Ϣ
    case AuthorizationConf::AuthType::None:
        return true;
    case AuthorizationConf::AuthType::Basic: 
        {
            // Basic��֤��Ҫ"username:password"��ʽ
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
                // ��ʽҪ��"HeaderName: Value" ��ֱ�Ӵ���ֵ���Զ����X-API-Keyͷ��
                size_t colonPos = auth_data.find(':');
                std::string header;

                if (colonPos != std::string::npos) 
                {
                    // �û��Զ���ͷ��ʽ
                    hkey = String::Trim(auth_data.substr(0, colonPos));
                    hval = String::Trim(auth_data.substr(colonPos + 1));
                }
                else 
                {
                    // Ĭ��ʹ��X-API-Keyͷ
                    hkey = "X-API-Key";
                    hval = auth_data;
                }
                return true;
            }
            // �������ݷ�ʽ��Ҫ��URL����׶δ����˴��޷�֧��
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
void AiModel::ExecuteAiTask(const std::string& content, const Scintilla::EndpointConfig& endpoint)
{
    try
    {
        if (fnSetStatus) fnSetStatus(TASK_STATUS_BEGIN);
        auto& plat = _conf.Platform();
        // ����HTTP�ͻ���
        SimpleHttp cli(plat.base_url, plat.enable_ssl);

        // ��������ͷ
        std::unordered_map<std::string, std::string> headers = {
            {"Content-Type", "application/json; charset=UTF-8"}
        };
        std::string authName, authValue;
        if (GetAuthorizationHeader(plat.authorization, authName, authValue) && !authName.empty())
        {
            headers[authName] = authValue;
        }
        cli.SetHeaders(headers);

        // ����������ʾ��
        spdlog::info("request:{}{} model:{} message:{}",
            plat.base_url.c_str(),
            plat.chat_endpoint.api.c_str(),
            plat.model_name.c_str(),
            content.c_str()
        );
        std::string promt = Scintilla::String::GBKToUTF8(content.c_str());

        // ����JSON����
        std::string requestJson = CreateJsonRequest(
            true,  // ��ʽ
            plat.model_name,
            promt
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
    catch (const std::exception& e)
    {
        ShowMsgBox(String::Format("�ӿڵ����쳣:%s", e.what()).c_str());
    }
    if (fnOutputFinished) fnOutputFinished("\r\n");
    if (fnSetStatus) fnSetStatus(TASK_STATUS_FINISH);
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
        if (fnAppentOutput)
        {
            fnAppentOutput(text);
            return;
        }
        call.AddText(text.size(), text.c_str());
        call.ScrollCaret();
    };

    // �������ֻ�Ч��������
    Scintilla::Typewriter writer(fetchFn, outputFn);

    // ��ӳ�ʼ����
    if(fnAppentOutput == nullptr) outputFn("\r\n\r\n");

    // ������������
    writer.Run();

    // ��ӽ�������
    if (fnAppentOutput == nullptr)
    {
        outputFn("\r\n\r\n");
    }
    fnAppentOutput = nullptr;
}