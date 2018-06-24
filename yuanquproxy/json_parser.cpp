#include "json_parser.h"
#include <boost/progress.hpp>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/foreach.hpp>
#include <syslog.h>


using namespace std;
using namespace boost;

void json_parser::tester()
{
	CValidateCarPass pass;

	std::string carNo = "test";
	std::string stationNo = "test";
	std::string channelNo = "1111111111";
	std::string type = "E";
	std::string carCapturePic = "";
	std::string eTagNo = "test";
	std::string ic = "test";
	
	pass.setCarNo(carNo);
	pass.setStationNo(stationNo);
	pass.setChannelNo(channelNo);
	pass.setType(type);
	pass.setCarCapturePic(carCapturePic);
	pass.seteTagNo(eTagNo);
	pass.setIc(ic);
	
	std::string s = generateValidateCarPass(pass);
        
        parse(s);
}

std::string json_parser::generateValidateCarPass(CValidateCarPass& pass)
{
	boost::property_tree::ptree pt_root;
	boost::property_tree::ptree children;
	boost::property_tree::ptree child;
	
        std::string strKey0 = "seqNo";
	std::string strKey1 = "carNo";
	std::string strKey2 = "stationNo";
	std::string strKey3 = "channelNo";
	std::string strKey4 = "type";
	std::string strKey5 = "carCapturePic";
	std::string strKey6 = "eTagNo";
	std::string strKey7 = "ic";
	
	{
                pt_root.add(strKey0, pass.getSeqNo());
		pt_root.add(strKey1, pass.getCarNo());
		pt_root.add(strKey2, pass.getStationNo());
		pt_root.add(strKey3, pass.getChannelNo());
		pt_root.add(strKey4, pass.getType());
		pt_root.add(strKey5, pass.getCarCapturePic());
		pt_root.add(strKey6, pass.geteTagNo());
		pt_root.add(strKey7, pass.getIc());
		
		//children.push_back(std::make_pair("", child));
	}
	
	//pt_root.add_child("validateCarPass", children);
	std::stringstream ss;
	boost::property_tree::write_json(ss, pt_root);
	std::string s = ss.str();
	return s;
}


std::string json_parser::generateCarPassLog(CCarPassLog& log)
{
	boost::property_tree::ptree pt_root;
	boost::property_tree::ptree children;
	boost::property_tree::ptree child;
	
        std::string strKey0 = "seqNo";
	std::string strKey1 = "carNo";
	std::string strKey2 = "stationNo";
	std::string strKey3 = "channelNo";
	std::string strKey4 = "type";
	std::string strKey5 = "carCapturePic";
        std::string strKey6 = "time";
	std::string strKey7 = "eTagNo";
	std::string strKey8 = "ic";
	
	{
                pt_root.add(strKey0, log.getSeqNo());
		pt_root.add(strKey1, log.getCarNo());
		pt_root.add(strKey2, log.getStationNo());
		pt_root.add(strKey3, log.getChannelNo());
		pt_root.add(strKey4, log.getType());
		pt_root.add(strKey5, log.getCarCapturePic());
                pt_root.add(strKey6, log.getTime());
		pt_root.add(strKey7, log.geteTagNo());
		pt_root.add(strKey8, log.getIc());
		
		//children.push_back(std::make_pair("", child));
	}
	
	//pt_root.add_child("validateCarPass", children);
	std::stringstream ss;
	boost::property_tree::write_json(ss, pt_root);
	std::string s = ss.str();
	return s;
}




int json_parser::parse(const std::string &strData)
{
using namespace boost::property_tree;  

  std::stringstream ss(strData);
//std::stringstream ss(str);    
  ptree pt;  
  try
  {      
        read_json(ss, pt);  
  }  
  catch(ptree_error & e) 
  {  
        syslog(LOG_DEBUG, "%s", e.what());    
        return 1;   
  }  

  try
  {  
        std::string carNo = pt.get<std::string>("carNo");   // 得到"code"的value  
        std::string stationNo = pt.get<std::string>("stationNo");   // 得到"code"的value  
        std::string channelNo = pt.get<std::string>("channelNo");   // 得到"code"的value  
        std::string type = pt.get<std::string>("type"); 
        std::string carCapturePic = pt.get<std::string>("carCapturePic"); 
        std::string eTagNo = pt.get<std::string>("eTagNo"); 
        std::string ic = pt.get<std::string>("ic"); 
              
  }  
  catch (ptree_error & e)  
  {  
    return 2;  
  }  
  return 0; 
}