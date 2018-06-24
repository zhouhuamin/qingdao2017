#ifndef __jz__json_parser__
#define __jz__json_parser__
#include <string>
#include <sstream>
#include <vector>

class CValidateCarPass
{
public:
	CValidateCarPass()
	{
		;
	}
private:
    std::string seqNo;
	std::string carNo;
	std::string stationNo;
	std::string channelNo;
	std::string type;
	std::string carCapturePic;
	std::string eTagNo;
	std::string ic;
	
public:
	std::string getSeqNo() {
		return seqNo;
	}

	void setSeqNo(const std::string &seqNo) {
		this->seqNo = seqNo;
	}    
        
    
    
	std::string getCarNo() {
		return carNo;
	}

	void setCarNo(const std::string &carNo) {
		this->carNo = carNo;
	}

	std::string getStationNo() {
		return stationNo;
	}

	void setStationNo(const std::string &stationNo) {
		this->stationNo = stationNo;
	}

	std::string getChannelNo() {
		return channelNo;
	}

	void setChannelNo(const std::string &channelNo) {
		this->channelNo = channelNo;
	}

	std::string getType() {
		return type;
	}

	void setType(const std::string &type) {
		this->type = type;
	}

	std::string getCarCapturePic() {
		return carCapturePic;
	}

	void setCarCapturePic(const std::string &carCapturePic) {
		this->carCapturePic = carCapturePic;
	}

	std::string geteTagNo() {
		return eTagNo;
	}

	void seteTagNo(const std::string &eTagNo) {
		this->eTagNo = eTagNo;
	}

	std::string getIc() {
		return ic;
	}

	void setIc(const std::string &ic) {
		this->ic = ic;
	}
};



class CCarPassLog
{
public:
	CCarPassLog()
	{
		;
	}
private:
    std::string seqNo;
	std::string carNo;
	std::string stationNo;
	std::string channelNo;
	std::string type;
	std::string carCapturePic;
        std::string time;
	std::string eTagNo;
	std::string ic;
	
public:
	std::string getSeqNo() {
		return seqNo;
	}

	void setSeqNo(const std::string &seqNo) {
		this->seqNo = seqNo;
	}    
    
    
	std::string getCarNo() {
		return carNo;
	}

	void setCarNo(const std::string &carNo) {
		this->carNo = carNo;
	}

	std::string getStationNo() {
		return stationNo;
	}

	void setStationNo(const std::string &stationNo) {
		this->stationNo = stationNo;
	}

	std::string getChannelNo() {
		return channelNo;
	}

	void setChannelNo(const std::string &channelNo) {
		this->channelNo = channelNo;
	}

	std::string getType() {
		return type;
	}

	void setType(const std::string &type) {
		this->type = type;
	}

	std::string getCarCapturePic() {
		return carCapturePic;
	}

	void setCarCapturePic(const std::string &carCapturePic) {
		this->carCapturePic = carCapturePic;
	}
        
        
	std::string getTime() {
		return time;
	}

	void setTime(const std::string &time) {
		this->time = time;
	}        
        

	std::string geteTagNo() {
		return eTagNo;
	}

	void seteTagNo(const std::string &eTagNo) {
		this->eTagNo = eTagNo;
	}

	std::string getIc() {
		return ic;
	}

	void setIc(const std::string &ic) {
		this->ic = ic;
	}
};




class json_parser
{
public:
	std::string generateValidateCarPass(CValidateCarPass& pass);
        std::string generateCarPassLog(CCarPassLog& log);
	int parse(const std::string &strData);
public:
	void tester();
};
#endif

