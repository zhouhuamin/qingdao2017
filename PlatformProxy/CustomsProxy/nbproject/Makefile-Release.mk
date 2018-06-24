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
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/262068057/MutexGuard.o \
	${OBJECTDIR}/_ext/262068057/SysMutex.o \
	${OBJECTDIR}/_ext/262068057/cppmysql.o \
	${OBJECTDIR}/CJZPlatformProxy.o \
	${OBJECTDIR}/cbasicdataaccess.o \
	${OBJECTDIR}/tinyXML/tinystr.o \
	${OBJECTDIR}/tinyXML/tinyxml.o \
	${OBJECTDIR}/tinyXML/tinyxmlerror.o \
	${OBJECTDIR}/tinyXML/tinyxmlparser.o


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
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCustomsProxy.${CND_DLIB_EXT}

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCustomsProxy.${CND_DLIB_EXT}: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCustomsProxy.${CND_DLIB_EXT} ${OBJECTFILES} ${LDLIBSOPTIONS} -shared -fPIC

${OBJECTDIR}/_ext/262068057/MutexGuard.o: ../include/MutexGuard.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/262068057
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/262068057/MutexGuard.o ../include/MutexGuard.cpp

${OBJECTDIR}/_ext/262068057/SysMutex.o: ../include/SysMutex.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/262068057
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/262068057/SysMutex.o ../include/SysMutex.cpp

${OBJECTDIR}/_ext/262068057/cppmysql.o: ../include/cppmysql.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/262068057
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/262068057/cppmysql.o ../include/cppmysql.cpp

${OBJECTDIR}/CJZPlatformProxy.o: CJZPlatformProxy.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/CJZPlatformProxy.o CJZPlatformProxy.cpp

${OBJECTDIR}/cbasicdataaccess.o: cbasicdataaccess.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cbasicdataaccess.o cbasicdataaccess.cpp

${OBJECTDIR}/tinyXML/tinystr.o: tinyXML/tinystr.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinystr.o tinyXML/tinystr.cpp

${OBJECTDIR}/tinyXML/tinyxml.o: tinyXML/tinyxml.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxml.o tinyXML/tinyxml.cpp

${OBJECTDIR}/tinyXML/tinyxmlerror.o: tinyXML/tinyxmlerror.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxmlerror.o tinyXML/tinyxmlerror.cpp

${OBJECTDIR}/tinyXML/tinyxmlparser.o: tinyXML/tinyxmlparser.cpp 
	${MKDIR} -p ${OBJECTDIR}/tinyXML
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -fPIC  -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tinyXML/tinyxmlparser.o tinyXML/tinyxmlparser.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/libCustomsProxy.${CND_DLIB_EXT}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
