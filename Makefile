default:
		windres authclient.rc -o authclientrc.o
		gcc -c authclient.c
		gcc authclient.o authclientrc.o -o authclient.exe -mwindows -Wall -Wl,-lwsock32,--strip-all
		del *.o
		
debug:
		windres authclient.rc -o authclientrc.o
		gcc -g -c authclient.c -Wall
		gcc -Wall authclient.o authclientrc.o -o authclient.exe -mwindows -Wl,-lwsock32
		del *.o
