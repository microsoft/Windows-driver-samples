@echo off
@setlocal

@rem -------------------------------------------------------------------------
@rem OBTAIN INPUT
@rem -------------------------------------------------------------------------

set TR_MODULE=%1
shift
set TR_LEVEL=%1
shift
set TR_VERB=%1

@rem -------------------------------------------------------------------------
@rem VALIDATE COMPONENT & TRACING LEVEL
@rem -------------------------------------------------------------------------

if /i "%TR_MODULE%"=="MONITOR" (
   set TR_GUID={dd65554d-9925-49d1-83b6-46125feb4207}
   set TR_MODULE=MsnMntrMonitor

   if "%TR_LEVEL%"=="0" (
      set TR_BITS=
      set TR_LEVEL=0
   ) else if "%TR_LEVEL%"=="1" (
      set TR_BITS=
      set TR_LEVEL=1
   ) else if /i "%TR_LEVEL%"=="2" (
      set TR_BITS=
      set TR_LEVEL=2
   ) else if /i "%TR_LEVEL%"=="9" (
      set TR_BITS=
      set TR_LEVEL=9
   ) else (
      echo.
      echo Error: Monitor component does not support this trace detail.
      goto :show_usage_MsnMntrMonitor
   )
) else if /i "%TR_MODULE%"=="NOTIFY" (
   set TR_GUID={aca2f74a-7a0d-4f47-be4b-66900813b8e5}
   set TR_MODULE=MsnMntrNotify

   if "%TR_LEVEL%"=="0" (
      set TR_BITS=
      set TR_LEVEL=0
   ) else if "%TR_LEVEL%"=="1" (
      set TR_BITS=
      set TR_LEVEL=1
   ) else if /i "%TR_LEVEL%"=="2" (
      set TR_BITS=
      set TR_LEVEL=2
   ) else if /i "%TR_LEVEL%"=="3" (
      set TR_BITS=
      set TR_LEVEL=3
   ) else if /i "%TR_LEVEL%"=="9" (
      set TR_BITS=
      set TR_LEVEL=9
   ) else (
      echo.
      echo Error: Notify component does not support this trace detail.
      goto :show_usage_MsnMntrNotify
   )   
   
) else if /i "%TR_MODULE%"=="CONTROL" (
   set TR_GUID={eab718af-52de-477c-874d-cb49746bb131}
   set TR_MODULE=MsnMntrCtl

   if "%TR_LEVEL%"=="0" (
      set TR_BITS=
      set TR_LEVEL=0
   ) else if "%TR_LEVEL%"=="1" (
      set TR_BITS=
      set TR_LEVEL=1
   ) else if /i "%TR_LEVEL%"=="2" (
      set TR_BITS=
      set TR_LEVEL=2
   ) else if /i "%TR_LEVEL%"=="9" (
      set TR_BITS=
      set TR_LEVEL=9
   ) else (
      echo.
      echo Error: Control component does not support this trace detail.
      goto :show_usage_MsnMntrCtl
   )

) else if /i "%TR_MODULE%"=="INIT" (
   set TR_GUID={e7db16bb-41be-4c05-b73e-5feca06f8207}
   set TR_MODULE=MsnMntrInit

   if "%TR_LEVEL%"=="0" (
      set TR_BITS=
      set TR_LEVEL=0
   ) else if "%TR_LEVEL%"=="1" (
      set TR_BITS=
      set TR_LEVEL=1
   ) else if /i "%TR_LEVEL%"=="9" (
      set TR_BITS=
      set TR_LEVEL=9
   ) else (
      echo.
      echo Error: Init component does not support this trace detail.
      goto :show_usage_MsnMntrInit
   )  

) else (
   echo.
   echo Error: No module was selected.
   goto :show_usage
)

set TR_NAME=%TR_MODULE%
set TR_DIR=%SystemRoot%\Tracing\%TR_NAME%
set TR_LOG=%TR_DIR%\%TR_NAME%.etl
set TR_BITS=0xFFFFFFFF
set TR_OPTS=
set TR_RT_OPTS=-rt -ft 1

set TRACE_FORMAT_PREFIX=%%9!d!:%%3!04X! %%!FUNC!:
set TRACE_FORMAT_SEARCH_PATH=%TR_DIR%

@rem -------------------------------------------------------------------------
@rem VALIDATE VERB
@rem -------------------------------------------------------------------------

if /i "%TR_VERB%"=="start" (
   call :start_trace
) else if /i "%TR_VERB%"=="stop" (
   call :stop_trace
) else if /i "%TR_VERB%"=="pdb" (
   call :extract_format_info %1
) else if /i "%TR_VERB%"=="rt" (
   call :format_realtime
) else if /i "%TR_VERB%"=="fmt" (
   call :format_offline
) else (
      echo.
      echo Error: A supported verb has not been specified.
      goto :show_usage
)

goto :eof

:ShowSummary
   echo.
   echo      Trace name : %TR_NAME%
   echo Trace directory : %TR_DIR%
   echo       Trace log : %TR_LOG%
   echo     Trace level : %TR_LEVEL%

@rem -------------------------------------------------------------------------
@rem START TRACING
@rem -------------------------------------------------------------------------
:start_trace
   if not exist %TR_DIR% mkdir %TR_DIR%
   logman query %TR_NAME% -ets 1 > NUL
   if errorlevel 1 (
      logman start %TR_NAME% %TR_OPTS% -p %TR_GUID% %TR_BITS% %TR_LEVEL% -o %TR_LOG% -ets
   ) else (
      echo Collection is already started.
      )
   goto :eof

@rem -------------------------------------------------------------------------
@rem STOP TRACING
@rem -------------------------------------------------------------------------
:stop_trace
   logman query %TR_NAME% -ets 1>NUL
   if NOT errorlevel 1 (
      logman stop %TR_NAME% -ets
   )
   goto :eof

@rem -------------------------------------------------------------------------
@rem EXTRACT FORMAT INFO
@rem -------------------------------------------------------------------------
:extract_format_info
   if "%1" == "" (
      set TR_PDB=.\%TR_MODULE%.pdb
   )else (
      set TR_PDB=%1
   )
   tracepdb -f %TR_PDB% -p %TR_DIR%
   goto :eof

@rem -------------------------------------------------------------------------
@rem FORMAT REALTIME
@rem -------------------------------------------------------------------------
:format_realtime
   call :stop_trace
   set TR_OPTS=%TR_RT_OPTS%
   call :start_trace
   start "%TR_NAME% Tracing" /low tracefmt -displayonly -rt %TR_NAME%
   goto :eof

@rem -------------------------------------------------------------------------
@rem FORMAT OFFLINE
@rem -------------------------------------------------------------------------
:format_offline
   tracefmt -o %TR_NAME%.txt %TR_LOG% -display
   goto :eof

goto :eof

@rem -------------------------------------------------------------------------
@rem CONTEXT SENSITIVE HELP
@rem -------------------------------------------------------------------------
:show_usage
   call :show_usage_header
   echo    9   Display all trace events
   echo        Select a component to see individual supported tracing levels.
   call :show_usage_footer
   
   goto :eof

:show_usage_MsnMntrMonitor
   call :show_usage_header
   echo    0   Established flow
   echo    1   Change of state information
   echo    2   Layer notifications
   echo    9   Display all trace events
   call :show_usage_footer
   
   goto :eof

:show_usage_MsnMntrNotify
   call :show_usage_header
   echo    0   Client to server
   echo    1   Peer to peer
   echo    2   Unknown
   echo    3   All traffic
   echo    9   Display all trace events
   call :show_usage_footer

   goto :eof
   
:show_usage_MsnMntrCtl
   call :show_usage_header
   echo    0   Initialization
   echo    1   Device control
   echo    2   State
   echo    9   Display all trace events
   call :show_usage_footer

   goto :eof
   
:show_usage_MsnMntrInit
   call :show_usage_header
   echo    0   Initialization
   echo    1   Shutdown
   echo    9   Display all trace events
   call :show_usage_footer

   goto :eof
   
:show_usage_header
   echo.
   echo Usage: monitor_trace COMPONENT LEVEL VERB
   echo.
   echo Components: 
   echo    MONITOR, NOTIFY, CONTROL and INIT
   echo.
   echo Trace detail:
   goto :eof

:show_usage_footer
   echo.
   echo Verbs:
   echo    start        Start collection.
   echo    stop         Stop collection.
   echo    pdb          Extract format information from the pdb in the current
   echo                 directory.
   echo    pdb [file]   Like the above, but allows the full path to the pdb
   echo                 to be specified.
   echo    rt           Displays the trace output in real-time. This
   echo                 automatically stops any existing collection and begins
   echo                 a new one with appropriate parameters for real-time.
   echo    fmt          Format the trace logfile to the console.
   echo.
   echo Note:
   echo    The most common scenario is to extract the format information from
   echo    the pdb, and then display the output in real-time.
   echo.
   echo Example:
   echo    cd /d MySymbolDir
   echo    monitor_trace init 0 pdb
   echo    monitor_trace init 0 rt

   goto :eof