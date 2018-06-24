#ifndef XML_CONFIG_H_INCLUDED
#define XML_CONFIG_H_INCLUDED

#include <string>
#include "tinyxml.h"

using namespace std;

class XMLConfig
{
public:
    XMLConfig(const char* xmlFileName)
        :mXmlConfigFile(xmlFileName),mRootElem(0)
    {
        mXmlConfigFile.LoadFile();
        mRootElem=mXmlConfigFile.RootElement();
    }

public:
    string GetValue(const string& nodeName);

private:
    XMLConfig();

private:
    TiXmlDocument    mXmlConfigFile;
    TiXmlElement*    mRootElem;

};


#endif
