ROOT=..\..\..\..\..
SRCDIR=..

WANT_C1132=TRUE
WANT_WDMDDK=TRUE
IS_32 = TRUE
IS_SDK = TRUE
IS_PRIVATE = TRUE
WIN32 = TRUE
DEPENDNAME=..\depend.mk
WIN32_LEAN_AND_MEAN=0

L32EXE = HCLIENT.EXE
L32RES = HCLIENT.res
PROPBINS=$(L32EXE) HCLIENT.sym
TARGETS=$(L32EXE) HCLIENT.sym
L32OBJS = hclient.obj pnp.obj report.obj strings.obj logpnp.obj buffers.obj ecdisp.obj
L32FLAGS = /MAP  /subsystem:windows  /machine:I386

CFLAGS = /nologo /Oi /W3 /D "WIN32" /D "_WINDOWS" /YX /c $(CFLAGS)
L32LIBSNODEP = $(ROOT)\dev\tools\c932\lib\libcmt.lib  \
               $(W32LIBID)\kernel32.lib \
               $(W32LIBID)\USER32.LIB \
               $(W32LIBID)\GDI32.LIB \
               $(W32LIBID)\WINSPOOL.LIB \
               $(W32LIBID)\COMDLG32.LIB \
               $(W32LIBID)\advapi32.lib \
               $(W32LIBID)\shell32.lib \
               $(W32LIBID)\uuid.lib  \
               $(ROOT)\dev\tools\c932\lib\crtdll.lib \
               $(ROOT)\wdm\ddk\lib\i386\hid.lib \
               $(ROOT)\wdm\ddk\lib\i386\hidclass.lib \
               $(ROOT)\dev\lib\setupapi.lib 

!include $(ROOT)\dev\master.mk

RCSRCS=hclient.rc

INCLUDE=$(ROOT)\wdm\ddk\inc;$(ROOT)\dev\inc;$(ROOT)\dev\ntddk\inc;$(INCLUDE);$(ROOT)\dev\msdev\mfc\include;
