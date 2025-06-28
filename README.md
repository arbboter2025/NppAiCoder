# AiCoder for Notepad++ 用户手册

## 一、产品简介
AiCoder是一款为Notepad++设计的轻量级AI辅助插件，提供以下核心功能：
- **嵌入式提问**：对选中的文本内容进行AI分析，通过侧边栏聊天界面与AI交互，实现多轮对话、问题解答或代码生成。
- **对话式提问**：独立的AI对话界面，通过侧边栏聊天界面与AI交互，实现多轮对话、问题解答或代码生成。

注意：本插件不包含代码自动补全、智能重构等深度集成功能，仅提供基础AI问答服务

---

## 二、安装指南

### 2.1 系统要求
- **操作系统**：Windows 7/10/11（32位或64位）
- **Notepad++版本**：v7.9及以上
- **网络连接**：需联网调用AI模型服务
- **AI服务**：AI模型接口及密钥


### 2.2 安装步骤

1. 下载安装包，解压至任意目录（建议关闭杀毒软件避免误拦截）。

2. 根据Notepad++版本选择对应脚本：
   - **32位用户**：双击运行 `32位安装卸载.bat`
   - **64位用户**：双击运行 `64位安装卸载.bat`

3. **右键以管理员身份运行脚本**，按提示完成安装。

4. 操作流程示例  
   ```bat
   请选择需要对Notepad++的AiCode插件进行的操作：Y-安装 N-卸载 [Y/N] Y
   检测到Notepad++安装目录: C:\Program Files\Notepad++
   正在安装到 C:\Program Files\Notepad++\plugins\AiCoder...
   安装成功，请重启Notepad++！
   ```
   
### 2.3 验证安装
1. 重启Notepad++。
2. 在菜单栏或插件列表中查看是否出现 **`AiCoder`** 选项。


### 2.4 卸载方法
1. 重新运行安装时使用的脚本（如 `64位安装卸载.bat`）。
2. 选择卸载选项并按提示操作。
3. 手动删除插件目录（可选）。

**右键以管理员身份运行脚本**，按提示输入`N`完成卸载（请注意关闭Notepad++程序）。
```
请选择需要对Notepad++的AiCode插件进行的操作：Y-安装 N-卸载 [Y/N] N
正在卸载，删除目录 C:\Program Files\Notepad++\plugins\AiCoder...
卸载成功！
```

---

## 三、功能使用

### 3.1 基础操作
| 功能类型   | 操作方式                                      | 界面示意 |
| ---------- | --------------------------------------------- | -------- |
| 嵌入式提问 | 1. 选中文本<br>2. 插件菜单或快捷键 → [Ask AI] |          |
| 对话式提问 | 插件菜单或快捷键 → [Open AI Chat]             |          |

### 3.2 配置说明
1. 编辑`config.json`文件或配置参数界面
```json
{
    "platform": "INFINI-AI",
    "timeout": 90,
    "platforms": {
        "INFINI-AI": {
            "enable_ssl": true,
            "base_url": "cloud.infini-ai.com",
            "authorization": {
                "type": "Bearer",
                "data": "sk-xxx"
            },
            "model_name": "deepseek-r1-distill-qwen-32b",
            "models": [ "deepseek-r1-distill-qwen-32b", "deepseek-r1", "deepseek-v3" ],
            "generate_endpoint": {
                "method": "post",
                "api": "/maas/v1/completions",
                "prompt": ""
            },
            "chat_endpoint": {
                "method": "post",
                "api": "/maas/v1/chat/completions",
                "prompt": ""
            },
            "models_endpoint": {

            }
        }
    }
}
```
**推荐平台**：  
- 无问芯穹：如果你还在犹豫用哪个AI平台，建议注册一个[无问芯穹](https://cloud.infini-ai.com/platform/ai)账户，可以免费申请密钥使用，配置文件中填入自己的密钥可开箱使用。（提供免费密钥申请）
- 通过菜单栏`插件 > 参数配置`可视化修改，无需手动编辑文件  

2. 保存文件并重启Notepad++生效。

### 3.3 功能使用
1. 嵌入式AI提问
**适用场景**：快速优化代码片段、生成注释或解释代码逻辑。
**操作步骤**：
1. 在编辑器中选中文本或代码。
2. 右键单击选择 `AiCoder > 分析/优化选中内容`，或使用快捷键 `Alt+A`。
3. AI返回结果将直接插入到光标位置。

2. 对话式AI提问
**适用场景**：对话调试、复杂问题咨询。
**操作步骤**：
1. 点击菜单栏 `插件 > AiCoder > 显示窗口`，或使用快捷键 `Alt+K`。
2. 在侧边栏输入问题（如“生成Python排序函数”）。
3. 按 `Ctrl+Enter` 发送，AI回复将实时显示在对话历史中。
4. 输入框的右下角有个按钮可提交或中断AI提问，也可看到AI提问状态

### 3.4 界面说明
- **菜单栏入口**：`插件 > 参数配置`，提供配置菜单界面，不需要手工编辑配置文件
- **侧边栏对话窗口**：`插件 > 显示窗口 Alt+K`，支持调整窗口大小，提供发送和停止AI提供按钮。
- **解读代码**：`插件 > 解读代码 Alt+J`，根据模板内容对选中内容的代码进行解读，支持默认模板。
- **优化代码**：`插件 > 优化代码 Alt+Y`，根据模板内容对选中内容的代码进行优化，支持默认模板。
- **代码注释**：`插件 > 代码注释 Alt+Z`，根据模板内容对选中内容的代码代码注释，支持默认模板。
- **选中即问**：`插件 > 选中即问 Alt+A`，将选中内容作为输入直接向AI提问。

---

## 四、常见问题

### Q1：安装后未显示插件

1. 检查Notepad++位数是否与插件匹配
2. 确认安装路径正确性：
   ```
   %Notepad++%\plugins\ 应包含 AiCoder.dll
   ```
3. 重启Notepad++

### Q2：API调用失败处理
1. 确认 `config.json` 中的API密钥有效且网络连接正常。
2. 错误代码对照表：
   | 代码 | 含义                  | 解决方案               |
   |------|-----------------------|------------------------|
   | 401  | 无效API密钥           | 检查密钥有效性         |
   | 429  | 请求频率过高          | 降低提问频率           |
   | 503  | 服务不可用            | 等待5分钟后重试        |

3. 测试命令：
   ```powershell
   curl -X POST https://api.openai.com/v1/chat/completions
   ```
### Q3：如何修改快捷键？
- 不支持


---

## 五、技术支持
- **反馈意见或问题**：留言或发送邮件至 support@aicoder.com，[![arbboter@hotmail.com](https://img.shields.io/badge/contact-aicoder_support%40example.com-blue)](mailto:arbboter@hotmail.com)

联系我：
![WeChat](https://i-blog.csdnimg.cn/direct/5a72844fcf7245cf84b8816f63337989.jpeg)

版本更新记录：  
`v1.0.0` - 2025.04 初版发布

> 免责声明：本插件与OpenAI无官方关联，API使用需遵守相关服务条款
