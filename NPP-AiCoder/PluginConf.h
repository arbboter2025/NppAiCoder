#include "pch.h"
#include <string>
NAMEPACE_BEG(Scintilla)

class PlatformConf
{
public:
    // ���������ֶ�
    std::string _baseUrl;
    std::string _apiSkey;
    std::string _modelName;
    std::string _generateEndpoint;
    std::string _chatEndpoint;

    // ��json�ַ�����ȡ��������
    bool Load(const std::string& sdat);
};

NAMEPACE_END