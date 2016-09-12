setlocal
for /f "tokens=1,2,3* delims= " %%a in ('dir /tc ui_jerky_main_window.h ^| findstr /ic:"ui_jerky_main_window"') do (
	set oldtime=%%a %%b
	)
for /f "tokens=1,2,3* delims= " %%x in ('dir /tc ..\x64\Debug\h264_source_local_audio.exe ^| findstr /ic:"h264_source_local_audio.exe"') do (
	set newtime=%%x %%y
	)
if "%oldtime%" gtr "%newtime%" (uic.exe jerky_main.ui -o ui_jerky_main_window.h)