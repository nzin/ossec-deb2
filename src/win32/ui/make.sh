echo Making windows agent UI

i586-mingw32msvc-windres -o resource.o win32ui.rc
i586-mingw32msvc-gcc -o os_win32ui.exe -Wall -DARGV0=\"ossec-agent\" -DCLIENT -DWIN32 resource.o ../os_net/*.c ../os_xml/*.c ../addagent/b64.c ../shared/validate_op.c ../shared/debug_op.c ../win_service.c *.c -I../headers/ -I../ -lcomctl32 -mwindows -lwsock32
cp -pr  os_win32ui.exe ../
cd ../
