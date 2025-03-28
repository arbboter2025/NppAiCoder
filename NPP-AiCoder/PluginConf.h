#include "pch.h"
#include <string>
NAMEPACE_BEG(Scintilla)

class PlatformConf
{
public:
    // 基础功能字段
    std::string _baseUrl;
    std::string _apiSkey;
    std::string _modelName;
    std::string _generateEndpoint;
    std::string _chatEndpoint;

    // 从json字符串读取配置内容
    bool Load(const std::string& sdat);
};

NAMEPACE_END