#if !defined DATA_ACCESS_H
#define DATA_ACCESS_H

#include "cppmysql.h"
#include <vector>
#include <map>
#include <list>
#include <string>
using namespace std;

class CDataAccess
{
public:

    CDataAccess()
    {
    }
    ;

    virtual ~CDataAccess()
    {
    };

public:

    virtual int RecordPassVehicleInfo(CppMySQL3DB* pDataabse, char* szXML)
    {
        return 0;
    }
    
    
    virtual int RecordPassResult(CppMySQL3DB* pDataabse, char* szXML)
    {
        return 0;
    }
    
};

// the types of the class factories
typedef CDataAccess* create_t();
typedef void destroy_t(CDataAccess*);


#endif

