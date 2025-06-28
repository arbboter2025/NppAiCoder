#include "pch.h"
#include "PluginConf.h"
#include "Utils.h"
#include <fstream>
using namespace Scintilla;

// 加载配置
bool PluginConfig::Load(const std::string& filename)
{
    m_confFile = filename;
    if (!Util::FileExist(filename.c_str()))
    {
        return false;
    }

    Scintilla::Json json;
    if (!json.LoadFile(filename))
    {
        ShowMsgBox("加载配置文件失败:" + filename);
        return false;
    }

    auto& j = json.Inst();
    // 解析基础配置
    Util::JsonGet(j, "platform", platform);
    Util::JsonGet(j, "timeout", timeout);

    // 解析提示词
    if (j.contains("promt") && j["promt"].is_object())
    {
        promt.from_json(j["promt"]);
    }
    promt.SetDefault();

    // 解析平台配置
    platforms.clear();
    if (j.contains("platforms") && j["platforms"].is_object())
    {
        for (auto& [key, value] : j["platforms"].items())
        {
            if (value.is_object())
            {
                PlatformConfig conf;
                conf.from_json(value);
                platforms[key] = conf;
            }
        }
    }
    return true;
}


// 保存配置
bool PluginConfig::Save(const std::string& filename/* = ""*/)
{
    nlohmann::json j;

    // 基础配置
    j["platform"] = platform;
    j["timeout"] = timeout;

    // 平台配置
    nlohmann::json platforms_json;
    for (auto& [name, config] : platforms)
    {
        nlohmann::json platform_json;
        platform_json["base_url"] = config.base_url;
        platform_json["authorization"]["type"] = AuthorizationConf::GetAuthType(config.authorization.eAuthType);
        platform_json["authorization"]["data"] = config.authorization.auth_data;
        platform_json["model_name"] = config.model_name;

        platform_json["generate_endpoint"]["method"] = config.generate_endpoint.method;
        platform_json["generate_endpoint"]["api"] = config.generate_endpoint.api;
        platform_json["generate_endpoint"]["prompt"] = config.generate_endpoint.prompt;

        platform_json["chat_endpoint"]["method"] = config.chat_endpoint.method;
        platform_json["chat_endpoint"]["api"] = config.chat_endpoint.api;
        platform_json["chat_endpoint"]["prompt"] = config.chat_endpoint.prompt;

        platform_json["models_endpoint"]["method"] = config.models_endpoint.method;
        platform_json["models_endpoint"]["api"] = config.models_endpoint.api;
        platform_json["models_endpoint"]["prompt"] = config.models_endpoint.prompt;

        platform_json["models"] = config.models;
        platforms_json[name] = platform_json;
    }
    j["platforms"] = platforms_json;

    // 提示词
    nlohmann::json promts;
    for (auto& [k, v] : promt.exts)
    {
        promts["exts"][k] = v;
    }
    j["promt"] = promts;

    // 写入文件
    std::string file = filename.empty() ? m_confFile : filename;
    // file = "D:\\config.json";
    std::ofstream fOut(file);
    if (!fOut.is_open())
    {
        ShowMsgBox("请尝试用管理员权限启动Notepad++，无法写入配置文件:" + file);
        return false;
    }
    fOut << j.dump(4);
    return true;
}

void Scintilla::PromtConfig::SetDefault()
{
    if (read_code.empty())
    {
        read_code = std::string(
            "[任务说明]                            \n"
            "请对用户提供的代码进行专业解读，要求：\n"
            "1. 分步骤分析代码结构和执行流程       \n"
            "2. 解释核心算法/逻辑的实现原理        \n"
            "3. 说明输入输出格式及数据处理方式     \n"
            "4. 标注关键代码段的作用               \n"
            "5. 指出可能存在的潜在风险             \n"
            "                                      \n"
            "[输入示例]                            \n"
            "def fibonacci(n):                     \n"
            "    a, b = 0, 1                       \n"
            "    result = []                       \n"
            "    while len(result) < n:            \n"
            "        result.append(a)              \n"
            "        a, b = b, a+b                 \n"
            "    return result                     \n"
            "                                      \n"
            "[输出要求]                            \n"
            "采用Markdown格式，包含：              \n"
            "- 函数功能概述                        \n"
            "- 执行流程图解（文字描述）            \n"
            "- 时间复杂度分析                      \n"
            "- 关键变量说明表                      \n"
            "- 异常处理建议                        \n"
        );
    }
    if (optimize_code.empty())
    {
        optimize_code = std::string(
            "[优化任务]                           \n"
            "请对以下代码进行专业级优化，要求：   \n"
            "1. 保持原有功能不变的前提下提升性能  \n"
            "2. 优化代码可读性和可维护性          \n"
            "3. 应用最新语言特性/设计模式         \n"
            "4. 添加必要的类型提示和错误处理      \n"
            "5. 给出优化前后的性能对比数据        \n"
            "                                     \n"
            "[优化原则]                           \n"
            "- 遵循PEP8/PEP20等规范               \n"
            "- 优先选择时间复杂度更优的算法       \n"
            "- 消除代码冗余和魔法数字             \n"
            "- 合理拆分复杂函数                   \n"
            "- 添加防御性编程措施                 \n"
            "                                     \n"
            "[输出格式]                           \n"
            "分两栏对比显示：                     \n"
            "| 原始代码 | 优化后代码 | 改进说明 | \n"
            "|----------|------------|----------| \n"
            "[示例优化项]                         \n"
            "- 循环结构优化                       \n"
            "- 内存使用优化                       \n"
            "- 并行计算改造                       \n"
            "- 缓存机制引入                       \n"
        );
    }
    if (add_comment.empty())
    {
        add_comment = std::string(
            "[注释规范]                                                   \n"
            "请为以下代码添加专业级注释，要求：                           \n"
            "1. 中英文双语注释（中文为主，英语补充）                      \n"
            "2. 使用Doxygen标准注释格式                                   \n"
            "3. 函数级注释包含：                                          \n"
            "   - 功能描述（@brief）                                      \n"
            "   - 参数说明（@param 类型 参数名 描述）                     \n"
            "   - 返回值说明（@return 类型 描述）                         \n"
            "   - 异常说明（@exception/@throw）                           \n"
            "   - 使用示例（@example）                                    \n"
            "4. 复杂逻辑添加行内注释说明算法，行注释优先保持存放在代码上一行        \n"
            "5. 类成员变量注释注明单位/取值范围                           \n"
            "6. 模板参数需要特殊说明                                      \n"
            "                                                             \n"
            "[注释层级]                                                   \n"
            "/**                                                          \n"
            " * @file    文件头注释（包含版权、许可证、简要描述）         \n"
            " * @author  作者信息                                         \n"
            " * @date    最后修改日期                                     \n"
            " */                                                          \n"
            "                                                             \n"
            "/// 类/函数级块注释（使用Doxygen标签）                       \n"
            "//! 单行注释（用于快速注释）                                 \n"
            "                                                             \n"
            "// 行内逻辑注释（双斜杠）                                    \n"
            "constexpr int MAX_RETRY = 5;  ///< 配置项注释（含默认值说明）\n"
        );
    }
    if (format_code.empty())
    {
        // C/C++ 规范 (Allman Style)
        format_code = std::string(
            "请根据目标语言自动应用以下对应规范：                   \n"
            "# 一、特定语言规范                      \n"
            "## - C/C++ 规范 (Allman Style)                      \n"
            "## - Python 规范 (PEP8)                             \n"
            "## - Java 规范 (K&R变体)                            \n"
            "# 二、通用规则                                       \n"
            "1. **格式统一**                                        \n"
            "   - 无特殊说明的，请使用该编程语言最通用的编码规范标准\n"
            "   - 运算符两侧单空格                                  \n"
            "   - 保留所有原始注释                                  \n"
            "   - 单行长度≤80字符（超长需换行对齐）                 \n"
            "                                                       \n"
            "2. **空行规则**                                        \n"
            "   - 函数/类之间：2空行                                \n"
            "   - 逻辑块之间：1空行                                 \n"
            "                                                       \n"
            "3. **特殊处理**                                        \n"
            "   - Python：import分三部分(标准库/第三方/本地)        \n"
            "   - Java：package/import语句无空行                    \n"
            "   - C/C++：头文件保护宏统一格式                       \n"
            "   ```c                                                \n"
            "   #ifndef __HEADER_NAME_H__                           \n"
            "   #define __HEADER_NAME_H__                           \n"
            "   // content                                          \n"
            "   #endif                                              \n"
            "   ```                                                 \n"
            "                                                       \n"
            "**输出要求：**                                         \n"
            "1. 严格保持原代码功能                                  \n"
            "2. 仅返回格式化后的代码                                \n"
            "3. 自动识别输入代码语言，适配其语言规范                   \n"
            "4. 对不符合项必须修正                                  \n"
            "5. 保留所有原始注释                                    \n"
            "                                                       \n"
            "请格式化以下代码(数据)：                                   \n"
            "{}                                                     \n"
        );
    }
}
