#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/CmdHandler_T.o \
	${OBJECTDIR}/Cmd_Acceptor.o \
	${OBJECTDIR}/ControlCmdHandler.o \
	${OBJECTDIR}/Log/FreeLongLog.o \
	${OBJECTDIR}/Log/FreeLongSourceRegister.o \
	${OBJECTDIR}/MSG_Center.o \
	${OBJECTDIR}/MutexGuard.o \
	${OBJECTDIR}/MyLog.o \
	${OBJECTDIR}/MySQLConnectionPool.o \
	${OBJECTDIR}/PlatCmdHandler_T.o \
	${OBJECTDIR}/Plat_Cmd_Acceptor.o \
	${OBJECTDIR}/SimpleConfig.o \
	${OBJECTDIR}/SysMutex.o \
	${OBJECTDIR}/SysUtil.o \
	${OBJECTDIR}/cppmysql.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/tinyXML/tinystr.o \
	${OBJECTDIR}/tinyXML/tinyxml.o \
	${OBJECTDIR}/tinyXML/tinyxmlerror.o \
	${OBJECTDIR}/tinyXML/tinyxmlparser.o \
	${OBJECTDIR}/tinyXML/xml_config.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=../../../tools/ACE_wrappers/lib/libACE.so /usr/lib/mysql/libmysqlclient_r.so

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk /root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver

/root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver: ../../../tools/ACE_wrappers/lib/libACE.so

/root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver: /usr/lib/mysql/libmysqlclient_r.so

/root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver: ${OBJECTFILES}
	${MKDIR} -p /root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86
	${LINK.cc} -o /root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/CmdHandler_T.o: CmdHandler_T.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CmdHandler_T.o CmdHandler_T.cpp

${OBJECTDIR}/Cmd_Acceptor.o: Cmd_Acceptor.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Cmd_Acceptor.o Cmd_Acceptor.cpp

${OBJECTDIR}/ControlCmdHandler.o: ControlCmdHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ControlCmdHandler.o ControlCmdHandler.cpp

${OBJECTDIR}/Log/FreeLongLog.o: Log/FreeLongLog.cpp 
	${MKDIR} -p ${OBJECTDIR}/Log
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Log/FreeLongLog.o Log/FreeLongLog.cpp

${OBJECTDIR}/Log/FreeLongSourceRegister.o: Log/FreeLongSourceRegister.cpp 
	${MKDIR} -p ${OBJECTDIR}/Log
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Log/FreeLongSourceRegister.o Log/FreeLongSourceRegister.cpp

${OBJECTDIR}/MSG_Center.o: MSG_Center.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MSG_Center.o MSG_Center.cpp

${OBJECTDIR}/MutexGuard.o: MutexGuard.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MutexGuard.o MutexGuard.cpp

${OBJECTDIR}/MyLog.o: MyLog.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MyLog.o MyLog.cpp

${OBJECTDIR}/MySQLConnectionPool.o: MySQLConnectionPool.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/MySQLConnectionPool.o MySQLConnectionPool.cpp

${OBJECTDIR}/PlatCmdHandler_T.o: PlatCmdHandler_T.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PlatCmdHandler_T.o PlatCmdHandler_T.cpp

${OBJECTDIR}/Plat_Cmd_Acceptor.o: Plat_Cmd_Acceptor.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Plat_Cmd_Acceptor.o Plat_Cmd_Acceptor.cpp

${OBJECTDIR}/SimpleConfig.o: SimpleConfig.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SimpleConfig.o SimpleConfig.cpp

${OBJECTDIR}/SysMutex.o: SysMutex.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SysMutex.o SysMutex.cpp

${OBJECTDIR}/SysUtil.o: SysUtil.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SysUtil.o SysUtil.cpp

${OBJECTDIR}/cppmysql.o: cppmysql.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cppmysql.o cppmysql.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/tinyXML/tinystr.o: tinyXML/tinystr.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinystr.o tinyXML/tinystr.cpp

${OBJECTDIR}/tinyXML/tinyxml.o: tinyXML/tinyxml.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxml.o tinyXML/tinyxml.cpp

${OBJECTDIR}/tinyXML/tinyxmlerror.o: tinyXML/tinyxmlerror.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxmlerror.o tinyXML/tinyxmlerror.cpp

${OBJECTDIR}/tinyXML/tinyxmlparser.o: tinyXML/tinyxmlparser.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxmlparser.o tinyXML/tinyxmlparser.cpp

${OBJECTDIR}/tinyXML/xml_config.o: tinyXML/xml_config.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -g -I../../../tools/ACE_wrappers -I/usr/include/mysql -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/xml_config.o tinyXML/xml_config.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} /root/jiezisoft/source/yuanquproject/XZCenterServer/dist/Debug/GNU-Linux-x86/xzcenterserver

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
