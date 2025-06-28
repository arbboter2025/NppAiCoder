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
#include "AiModel.h"
#include "PluginConfigDlg.h"
#include "Exui.h"
#include "Utils.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

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
HMODULE g_hModule = nullptr;
NppData g_nppData;
static PluginConfigDlg* g_pConfigDlg = nullptr;
Scintilla::PluginConfig g_pluginConf;
AiAssistWnd* g_pAiWnd = nullptr;
AiModel* g_pAiModel = nullptr;
NppImp* g_pNppImp = nullptr;
ShortcutKey* g_pShortcutKeys = nullptr;
std::atomic<bool> g_bRun = false;

std::string GetAppDataPath() 
{
    // 获取所需缓冲区大小
    DWORD bufferSize = GetEnvironmentVariableA("APPDATA", nullptr, 0);
    if (bufferSize == 0) 
    {
        return "";
    }

    // 分配缓冲区并获取路径
    std::string appDataPath;
    appDataPath.resize(bufferSize);
    if (GetEnvironmentVariableA("APPDATA", appDataPath.data(), bufferSize) == 0)
    {
        return "";
    }

    // 移除末尾的终止符（如果有）
    appDataPath.resize(bufferSize - 1);
    return appDataPath;
}

void InitLogger()
{
    // 创建文件日志器
    auto log_file = GetAppDataPath() + "\\NppPlugin\\AiCoder\\logs\\AiCoder_" + Scintilla::Util::GetCurrentDate("%Y%m%d") + ".log";
    if (!Scintilla::Util::Mkdir(log_file))
    {
        return;
    }
    auto file_logger = spdlog::basic_logger_mt("file_logger", log_file);

    // 创建控制台日志器
    auto console_logger = spdlog::stdout_color_mt("console");

    // 设置全局日志器
    spdlog::set_default_logger(file_logger);

    // 设置日志格式
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    // 设置日志级别
    spdlog::set_level(spdlog::level::debug);

    // 自动保存到文件
    spdlog::flush_every(std::chrono::seconds(5));
}

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
    if (hModule != NULL)
    {
        g_hModule = (HMODULE)hModule;
        char szModulePath[MAX_PATH] = { 0 };
        ::GetModuleFileNameA((HMODULE)hModule, szModulePath, CARRAY_LEN(szModulePath));

        char szModuleDir[MAX_PATH] = { 0 };
        strcpy_s(szModuleDir, szModulePath);
        PathRemoveFileSpecA(szModuleDir);

        std::string strConfigFile(szModuleDir);
        strConfigFile += "\\config.json";

        // 读取配置
        g_pluginConf.Load(strConfigFile);
    }
    InitLogger();
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
    if (g_pAiModel) { delete g_pAiModel; g_pAiModel = nullptr; }
    if (g_pAiWnd) { delete g_pAiWnd; g_pAiWnd = nullptr; }
    if (g_pNppImp) { delete g_pNppImp; g_pNppImp = nullptr; }
    if (g_pShortcutKeys) { delete[] g_pShortcutKeys; g_pShortcutKeys = nullptr; }
}

BOOL InitRichEdit2()
{
    static HMODULE hRichEdit = LoadLibrary(TEXT("riched20.dll"));
    return hRichEdit != nullptr;
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
    g_pAiModel = new AiModel(g_pluginConf, g_nppData);

    // 初始化菜单
    ShortcutKey* pSck = new ShortcutKey[nbFunc];
    g_pShortcutKeys = pSck;
    size_t nCid = 0;
    pSck[nCid] = { false, true, false, 'B' };
    setCommand(nCid, L"参数配置", PluginConfig, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'K' };
    setCommand(nCid, L"显示窗口", OpenAiAssistWnd, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'J' };
    setCommand(nCid, L"解读代码", ReadCode, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'Y' };
    setCommand(nCid, L"优化代码", OptimizeCode, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'Z' };
    setCommand(nCid, L"代码注释", AddCodeComment, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'G' };
    setCommand(nCid, L"规范代码", FormatCode, pSck + nCid, false); ++nCid;
    pSck[nCid] = { false, true, false, 'A' };
    setCommand(nCid, L"选中即问", AskBySelectedText, pSck + nCid, false); ++nCid;
    pSck[nCid] = { true, true, false, 'C' };
    setCommand(nCid, L"取消任务", CancalTask, pSck + nCid, false); ++nCid;

    InitRichEdit2();
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


//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

int ShowMsgBox(const std::string& info, const std::string& title/* = "提示"*/, UINT nFlag/* = MB_OK*/)
{
    return MessageBoxA(g_nppData._nppHandle, info.c_str(), title.c_str(), nFlag);
}

// 参数设置
void PluginConfig()
{
    if (g_pConfigDlg == nullptr || !IsWindow(g_pConfigDlg->GetHandle()))
    {
        delete g_pConfigDlg;
        g_pConfigDlg = new PluginConfigDlg((HINSTANCE)g_hModule, g_pluginConf);
    }
    if (g_pAiWnd && g_pAiWnd->isVisible())
    {
        g_pConfigDlg->fnOnConfigChange = std::bind(&AiAssistWnd::ReloadConfig, g_pAiWnd);
    }
    else
    {
        g_pConfigDlg->fnOnConfigChange = nullptr;
    }
    g_pConfigDlg->Show();
}

// 打开Ai助手窗口
void OpenAiAssistWnd()
{
    // 初始化Dock窗口
    if (g_pAiWnd == nullptr && g_hModule != nullptr && g_nppData._nppHandle != nullptr)
    {
        g_pAiWnd = new AiAssistWnd((HINSTANCE)g_hModule, g_nppData, g_pluginConf);
        g_pAiWnd->init();
        auto& platform = g_pluginConf.Platform();
        auto it = std::find(platform.models.begin(), platform.models.end(), platform.model_name);
        g_pAiWnd->fnOnModelSelChange = [](const std::string& model) {
            g_pluginConf.platforms[g_pluginConf.platform].model_name = model;
        };
        g_pAiWnd->fnOnInputFinished = [](const std::string& text) {
            g_pAiModel->fnAppentOutput = [](const std::string& ans) {
                g_pAiWnd->appendAnswer(Scintilla::String::UTF8ToGBK(ans.c_str(), ans.size()), false); 
            };
            g_pAiModel->fnOutputFinished = std::bind(&AiAssistWnd::OnOutputFinished, g_pAiWnd, std::placeholders::_1);
            g_pAiWnd->appendAnswer("【答】");
            g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
                "",
                text);
        };
        g_pAiModel->fnSetStatus = std::bind(&AiAssistWnd::SetTaskStatus, g_pAiWnd, std::placeholders::_1);
    }
    if (g_pAiWnd)
    {
        g_pAiWnd->display(true);
    }
}

// 解读代码
void ReadCode()
{
    g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
        "read_code",
        g_pNppImp->GetSelText(true, true));
}

// 代码优化
void OptimizeCode()
{
    g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
        "optimize_code",
        g_pNppImp->GetSelText(true, true));
}

// 添加代码注释
void AddCodeComment()
{
    g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
        "add_comment",
        g_pNppImp->GetSelText(true, true));
}

// 规范代码
void FormatCode()
{
    g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
        "format_code",
        g_pNppImp->GetSelText(true, true));
}

// 直接提问
void AskBySelectedText()
{
    g_pNppImp->RunAiRequest(std::bind(&AiModel::Request, g_pAiModel, std::placeholders::_1, std::placeholders::_2),
        "",
        g_pNppImp->GetSelText(true, true));
}

void CancalTask()
{
    g_bRun = false;
    spdlog::info("用户主动取消任务");
}