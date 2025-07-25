﻿// pch.h: 这是预编译标头文件。
// 下方列出的文件仅编译一次，提高了将来生成的生成性能。
// 这还将影响 IntelliSense 性能，包括代码完成和许多代码浏览功能。
// 但是，如果此处列出的文件中的任何一个在生成之间有更新，它们全部都将被重新编译。
// 请勿在此处添加要频繁更新的文件，这将使得性能优势无效。

#ifndef PCH_H
#define PCH_H

// 添加要在此处预编译的标头
#include "framework.h"
#include <string>
#include <atomic>
#include <spdlog/spdlog.h>

#define NAMEPACE_BEG(x) namespace x {
#define NAMEPACE_END }

#define CHECK_OR_RETURN(A, ...) \
            do { \
                if (!(A)) { \
                    return (##__VA_ARGS__); \
                } \
            } while (0)
#define IDC_MENU_CONFIG_ENDPOINT 0
#define TASK_STATUS_BEGIN    0
#define TASK_STATUS_FINISH   1

extern std::atomic<bool> g_bRun;
extern HMODULE g_hModule;

enum class AiRespType
{
    OPENAI_TOTAL_RESP,
    OPENAI_STREAM_RESP,
};

int ShowMsgBox(const std::string& info, const std::string& title = "提示", UINT nFlag = MB_OK);

#endif //PCH_H
