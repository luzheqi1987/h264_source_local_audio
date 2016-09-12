setlocal
if  not  exist ..\x64\Debug\h264_source_local_audio.exe  (uic.exe jerky_main.ui -o ui_jerky_main_window.h)
for /f "tokens=1,2,3* delims= " %%a in ('dir /tc jerky_main.ui ^| findstr /ic:"jerky_main.ui"') do (
	set oldtime=%%a %%b
	)
for /f "tokens=1,2,3* delims= " %%x in ('dir /tc ..\x64\Debug\h264_source_local_audio.exe ^| findstr /ic:"h264_source_local_audio.exe"') do (
	set newtime=%%x %%y
	)
if "%oldtime%" gtr "%newtime%" (uic.exe jerky_main.ui -o ui_jerky_main_window.h)