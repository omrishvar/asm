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
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/asm.o \
	${OBJECTDIR}/buffer.o \
	${OBJECTDIR}/helper.o \
	${OBJECTDIR}/lex.o \
	${OBJECTDIR}/linestr.o \
	${OBJECTDIR}/list.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/memstream.o \
	${OBJECTDIR}/output.o \
	${OBJECTDIR}/symtable.o


# C Compiler Flags
CFLAGS=-pedantic

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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/asm

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/asm: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/asm ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/asm.o: asm.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/asm.o asm.c

${OBJECTDIR}/buffer.o: buffer.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/buffer.o buffer.c

${OBJECTDIR}/helper.o: helper.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/helper.o helper.c

${OBJECTDIR}/lex.o: lex.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/lex.o lex.c

${OBJECTDIR}/linestr.o: linestr.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/linestr.o linestr.c

${OBJECTDIR}/list.o: list.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/list.o list.c

${OBJECTDIR}/main.o: main.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.c

${OBJECTDIR}/memstream.o: memstream.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/memstream.o memstream.c

${OBJECTDIR}/output.o: output.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/output.o output.c

${OBJECTDIR}/symtable.o: symtable.c
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -Wall -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/symtable.o symtable.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
