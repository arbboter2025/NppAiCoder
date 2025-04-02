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

#include "SimpleHttp.h"
#include "json.hpp"
#include "PluginConf.h"
#include "AiAssistWnd.h"
#include "NppImp.h"

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
HANDLE g_hModule = nullptr;
NppData g_nppData;
Scintilla::PluginConfig g_pluginConf;
AiAssistWnd* g_pAiWnd = nullptr;
NppImp* g_pNppImp = nullptr;
ShortcutKey* g_pShortcutKeys = nullptr;

Scintilla::PlatformConf g_PlatformConf;
//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    if (hModule != NULL)
    {
        g_hModule = hModule;
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
        g_pluginConf.Load(strConfigFile.c_str());
    }
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
    if (g_pAiWnd) { delete g_pAiWnd; g_pAiWnd = nullptr; }
    if (g_pNppImp) { delete g_pNppImp; g_pNppImp = nullptr; }
    if (g_pShortcutKeys) { delete[] g_pShortcutKeys; g_pShortcutKeys = nullptr; }
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

    // 初始化数据
    g_pNppImp = new NppImp(g_nppData);

    // 初始化菜单
    ShortcutKey* pSck = new ShortcutKey[nbFunc];
    g_pShortcutKeys = pSck;
    size_t nCid = 0;
    setCommand(nCid, L"参数配置", PluginConfig, NULL, false); ++nCid;
    pSck[nCid] = { false, true, false, 'K' };
    setCommand(nCid, L"显示窗口", OpenAiAssistWnd, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'J' };
    setCommand(nCid, L"解读代码", ReadCode, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'Y' };
    setCommand(nCid, L"优化代码", OptimizeCode, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'Z' };
    setCommand(nCid, L"代码注释", AddCodeComment, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'A' };
    setCommand(nCid, L"选中即问", AskBySelectedText, pSck + nCid, false); ++nCid;
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
        MessageBox(g_nppData._scintillaMainHandle, L"调用大模型失败", L"提示", MB_OK);
        return;
    }

    Scintilla::ScintillaCall call;
    call.SetFnPtr((intptr_t)g_nppData._scintillaMainHandle);
    
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
// 参数设置
void PluginConfig()
{

}

// 打开Ai助手窗口
void OpenAiAssistWnd()
{
    // 初始化Dock窗口
    if (g_pAiWnd == nullptr && g_hModule != nullptr && g_nppData._nppHandle != nullptr)
    {
        g_pAiWnd = new AiAssistWnd((HINSTANCE)g_hModule, g_nppData);
        g_pAiWnd->init();
        g_pAiWnd->updateModelList(g_pluginConf.Platform().models);
    }
    if (g_pAiWnd)
    {
        g_pAiWnd->display(true);
    }
}

// 解读代码
void ReadCode()
{

}

// 代码优化
void OptimizeCode()
{

}

// 添加代码注释
void AddCodeComment()
{

}

void AskBySelectedText()
{
    NppImp npp(g_nppData);
    auto text = npp.GetSelText();

    // 设置输出位置，选中部分尾部新建一行
    auto pCall = npp.SciCall();
    auto pEnd = pCall->SelectionEnd();
    pCall->ClearSelections();
    pCall->GotoPos(pEnd + 1);

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
