//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "pch.h"
#include "PluginDefinition.h"
#include "menuCmdID.h"

#include "ScintillaTypes.h"
#include "ScintillaStructures.h"
#include "ScintillaMessages.h"
#include "ScintillaCall.h"
#include "Utils.h"

#include "SimpleHttp.h"
#include "json.hpp"
#include "PluginConf.h"

#include <shlwapi.h>
#include <string>



#pragma comment(lib, "Shlwapi.lib")

// 宏
#define CARRAY_LEN(a) (sizeof(a) / sizeof(a[0]) - 1)

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;
Scintilla::PlatformConf g_PlatformConf;
//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    if (hModule != NULL)
    {
        char szModulePath[MAX_PATH] = { 0 };
        ::GetModuleFileNameA((HMODULE)hModule, szModulePath, CARRAY_LEN(szModulePath));

        char szModuleDir[MAX_PATH] = { 0 };
        strcpy_s(szModuleDir, szModulePath);
        PathRemoveFileSpecA(szModuleDir);

        std::string strConfigFile(szModuleDir);
        strConfigFile += "\\config.json";

        // 读取配置
        std::string strConf;
        Scintilla::File::ReadFile(strConfigFile, strConf);
        g_PlatformConf.Load(strConf);
    }
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );

    // 初始化快捷键
    auto pShortcutKeys = new ShortcutKey[nbFunc];
    memset(pShortcutKeys, 0, sizeof(ShortcutKey) * nbFunc);
    ShortcutKey& key = pShortcutKeys[0];


    setCommand(0, L"About AiCoder", HelloAiCoder, NULL, false);

    key = pShortcutKeys[1];
    key._isCtrl = true;
    key._isAlt = true;
    key._isShift = false;
    key._key = VK_F2;
    setCommand(1, L"选中内容问AI", AskBySelectedText, &key, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, const TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}


std::string CreateJsonRequest(bool stream, const std::string& model, const std::string& content) 
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

void AiRequest(const std::string& question)
{
    // 创建HTTPS客户端
    SimpleHttp cli(g_PlatformConf._baseUrl, true);


    // 设置默认头，字符编码使用UTF-8
    cli.SetHeaders({
        { "Content-Type", "application/json; charset=UTF-8" },
        { "Authorization", g_PlatformConf._apiSkey }
        }
    );

    // 请求参数
    auto prompt = CreateJsonRequest(true, g_PlatformConf._modelName, Scintilla::String::GBKToUTF8(question.c_str()));

    std::string resp;
     // 发送POST请求
    if (cli.Post(g_PlatformConf._chatEndpoint, prompt, resp, true))
    {
        MessageBox(nppData._scintillaMainHandle, L"调用大模型失败", L"提示", MB_OK);
        return;
    }

    Scintilla::ScintillaCall call;
    call.SetFnPtr((intptr_t)nppData._scintillaMainHandle);
    
    // 设置打字机参数，读和写
    auto fnGet = std::bind(&SimpleHttp::TryFetchResp, &cli, std::placeholders::_1);
    auto fnSet = [&](const std::string& text) {
        // 添加文本内容
        call.AddText(text.size(), text.c_str());
        // 光标强制可见，自动滚动效果
        call.ScrollCaret();
    };

    // 创建并启动打字机
    Scintilla::Typewriter writer(fnGet, fnSet);
    fnSet("\r\n\r\n");
    writer.Run();
    fnSet("\r\n\r\n");
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void HelloAiCoder()
{

}



void AskBySelectedText()
{
    // 获取当前文本内容
    Scintilla::ScintillaCall call;
    call.SetFnPtr((intptr_t)nppData._scintillaMainHandle);
    auto text = call.GetSelText();
    auto code_page = call.CodePage();
    if (code_page > 0 && code_page != CP_ACP)
    {
        text = Scintilla::String::ConvEncoding(text.c_str(), text.size(), code_page, CP_ACP);
    }

    // 设置输出位置，选中部分尾部新建一行
    auto pEnd = call.SelectionEnd();
    call.ClearSelections();
    call.GotoPos(pEnd + 1);

    // 创建并启动子线程
    std::shared_ptr<char[]> pText(new char[text.size()]);
    memcpy(pText.get(), text.c_str(), text.size());
    std::thread worker([pText]() {
        try
        {
            AiRequest(pText.get());
        }
        catch (const std::exception& e)
        {
            MessageBoxA(NULL, e.what(), "异常", MB_OK);
        }
        catch (...)
        {
            MessageBoxA(NULL, "未知异常", "异常", MB_OK);
        }
    });

    // 分离子线程，避免主线程阻塞
    worker.detach();
}
