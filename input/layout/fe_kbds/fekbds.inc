#****************************** File Header ******************************\
# File Name: Sources.inc for FE KBDs
#
# Copyright (c) 1985-2000, Microsoft Corporation
#
# Project specific SOURCES file.
#
# History:
# May-14-1999 HiroYama Created
#***************************************************************************/


TARGETNAME=$(LAYOUTNAME)


!include ..\..\..\sources.inc


# Override some of the values set by sources.inc
SOURCES=$(LAYOUTNAME).c $(LAYOUTNAME).rc

C_DEFINES=$(C_DEFINES) -DFE_SB -DFE_IME

