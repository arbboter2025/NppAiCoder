#include "pch.h"
#include "PluginConf.h"
#include "Utils.h"
using namespace Scintilla;

// 加载配置
void PluginConfig::Load(const std::string& filename)
{
    Scintilla::Json json;
    if (!json.LoadFile(filename))
    {
        ShowMsgBox("加载配置文件失败:" + filename);
        return;
    }

    auto& j = json.Inst();
    // 解析基础配置
    Util::JsonGet(j, "platform", platform);
    Util::JsonGet(j, "timeout", timeout);

    // 解析提示词
    Util::JsonGet(j, "promt", promt);
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
}


// 保存配置
void PluginConfig::Save(const std::string& filename)
{
    /*
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
        platform_json["api_key"] = config.api_key.empty() ? nullptr : config.api_key;
        platform_json["model_name"] = config.model_name;

        platform_json["generate_endpoint"]["method"] = config.generate_endpoint.method;
        platform_json["generate_endpoint"]["api"] = config.generate_endpoint.api;
        platform_json["generate_endpoint"]["prompt"] = config.generate_endpoint.prompt.empty() ? nullptr : config.generate_endpoint.prompt;

        platform_json["chat_endpoint"]["method"] = config.chat_endpoint.method;
        platform_json["chat_endpoint"]["api"] = config.chat_endpoint.api;
        platform_json["chat_endpoint"]["prompt"] = config.chat_endpoint.prompt.empty() ? nullptr : config.chat_endpoint.prompt;

        platform_json["models_endpoint"]["method"] = config.models_endpoint.method;
        platform_json["models_endpoint"]["api"] = config.models_endpoint.api;
        platform_json["models_endpoint"]["prompt"] = config.models_endpoint.prompt.empty() ? nullptr : config.models_endpoint.prompt;

        platform_json["models"] = config.models;

        platforms_json[name] = platform_json;
    }
    j["platforms"] = platforms_json;

    // 写入文件
    std::ofstream file(filename);
    file << j.dump(4);
    */
}

void Scintilla::PromtConfig::SetDefault()
{
    if (read_code.empty())
    {
        read_code = std::string(
            "[任务说明]                            "
            "请对用户提供的代码进行专业解读，要求："
            "1. 分步骤分析代码结构和执行流程       "
            "2. 解释核心算法/逻辑的实现原理        "
            "3. 说明输入输出格式及数据处理方式     "
            "4. 标注关键代码段的作用               "
            "5. 指出可能存在的潜在风险             "
            "                                      "
            "[输入示例]                            "
            "def fibonacci(n):                     "
            "    a, b = 0, 1                       "
            "    result = []                       "
            "    while len(result) < n:            "
            "        result.append(a)              "
            "        a, b = b, a+b                 "
            "    return result                     "
            "                                      "
            "[输出要求]                            "
            "采用Markdown格式，包含：              "
            "- 函数功能概述                        "
            "- 执行流程图解（文字描述）            "
            "- 时间复杂度分析                      "
            "- 关键变量说明表                      "
            "- 异常处理建议                        "
        );
    }
    if (optimize_code.empty())
    {
        optimize_code = std::string(
            "[优化任务]                           "
            "请对以下代码进行专业级优化，要求：   "
            "1. 保持原有功能不变的前提下提升性能  "
            "2. 优化代码可读性和可维护性          "
            "3. 应用最新语言特性/设计模式         "
            "4. 添加必要的类型提示和错误处理      "
            "5. 给出优化前后的性能对比数据        "
            "                                     "
            "[优化原则]                           "
            "- 遵循PEP8/PEP20等规范               "
            "- 优先选择时间复杂度更优的算法       "
            "- 消除代码冗余和魔法数字             "
            "- 合理拆分复杂函数                   "
            "- 添加防御性编程措施                 "
            "                                     "
            "[输出格式]                           "
            "分两栏对比显示：                     "
            "| 原始代码 | 优化后代码 | 改进说明 | "
            "|----------|------------|----------| "
            "[示例优化项]                         "
            "- 循环结构优化                       "
            "- 内存使用优化                       "
            "- 并行计算改造                       "
            "- 缓存机制引入                       "
        );
    }
    if (add_comment.empty())
    {
        add_comment = std::string (
            "[注释规范]                                                   "
            "请为以下代码添加专业级注释，要求：                           "
            "1. 中英文双语注释（英文为主，中文补充）                      "
            "2. 使用Doxygen标准注释格式                                   "
            "3. 函数级注释包含：                                          "
            "   - 功能描述（@brief）                                      "
            "   - 参数说明（@param 类型 参数名 描述）                     "
            "   - 返回值说明（@return 类型 描述）                         "
            "   - 异常说明（@exception/@throw）                           "
            "   - 使用示例（@example）                                    "
            "4. 复杂逻辑添加行内注释说明算法                              "
            "5. 类成员变量注释注明单位/取值范围                           "
            "6. 模板参数需要特殊说明                                      "
            "                                                             "
            "[注释层级]                                                   "
            "/**                                                          "
            " * @file    文件头注释（包含版权、许可证、简要描述）         "
            " * @author  作者信息                                         "
            " * @date    最后修改日期                                     "
            " */                                                          "
            "                                                             "
            "/// 类/函数级块注释（使用Doxygen标签）                       "
            "//! 单行注释（用于快速注释）                                 "
            "                                                             "
            "// 行内逻辑注释（双斜杠）                                    "
            "constexpr int MAX_RETRY = 5;  ///< 配置项注释（含默认值说明）"
        );
    }
}
