#if !defined _SYS_INCLUDE_H_
#define _SYS_INCLUDE_H_

//通用包含
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>

#ifdef WIN32                                //WIndows下
    #include <conio.h>
    #include <windows.h>
    #include <process.h>
    #include <winsock.h>
#else                                        //linux下
    #include <pthread.h>
    #include <sys/time.h>

#endif



#endif
