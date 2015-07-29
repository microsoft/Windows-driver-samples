TARGETTYPE=DRIVER

TARGETLIBS= \
        $(DDK_LIB_PATH)\portcls.lib\
        $(DDK_LIB_PATH)\stdunk.lib \
        $(SDK_LIB_PATH)\libcntpr.lib

INCLUDES= \
        $(DDK_INC_PATH); \
        ..

MSC_WARNING_LEVEL=-W4 -WX

C_DEFINES= $(C_DEFINES) -D_WIN32 -DUNICODE -D_UNICODE -DPC_IMPLEMENTATION

#
# Different levels of debug printage.  First is nothing but
# catastrophic errors, last is everything under the sun.
#
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_ERROR
C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_TERSE
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_VERBOSE
#C_DEFINES= $(C_DEFINES) -DDEBUG_LEVEL=DEBUGLVL_BLAB

SOURCES=\
        ..\adapter.cpp  \
        ..\basetopo.cpp \
        ..\common.cpp   \
        ..\hw.cpp       \
        ..\kshelper.cpp \
        ..\savedata.cpp \
        ..\tonegenerator.cpp \
        ..\sysvad.rc

