
mapcgi.cgi: mapcgi.o libcgi.o
	cc -o mapcgi.cgi mapcgi.o libcgi.o libstr.o libpr.o

mapcgi.o: mapcgi.c
	cc -c -o mapcgi.o mapcgi.c

libcgi.o: libcgi.h libcgi.c
	cc -c -o libstr.o libstr.c
	cc -c -o libpr.o libpr.c
	cc -c -o libcgi.o libcgi.c

