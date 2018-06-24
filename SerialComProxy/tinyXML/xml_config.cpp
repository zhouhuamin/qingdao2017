/*
** FileName:    xml_config.cpp
** Author:        hansen
** Date:            May 11, 2007
** Comment:
*/

#include "xml_config.h"
#include <iostream>

string XMLConfig::GetValue(const string& nodeName)
{
    if(!mRootElem)
    {
        cout<<"Read root node error!"<<endl;
        return "";
    }

    TiXmlElement* pElem=mRootElem->FirstChildElement(nodeName.c_str());
    if(!pElem)
    {
        cout<<"Read node "<<nodeName<<" error! "<<endl;
        return "";
    }
    return pElem->GetText();

}
