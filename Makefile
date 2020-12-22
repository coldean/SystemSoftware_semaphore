all : client server

client: testcase.o LpcProxy.o LpcKey.o
	gcc -o client testcase.o LpcProxy.o LpcKey.o -lpthread

testcase.o: testcase.c
	gcc -c -o testcase.o testcase.c

LpcProxy.o: LpcProxy.c
	gcc -c -o LpcProxy.o LpcProxy.c -lpthread

LpcKey.o: LpcKey.c
	gcc -c -o LpcKey.o LpcKey.c

server: Server.o LpcStub.o LpcKey.o
	gcc -o server Server.o LpcStub.o LpcKey.o -lpthread

Server.o: Server.c
	gcc -c -o Server.o Server.c -lpthread

LpcStub.o: LpcStub.c
	gcc -c -o LpcStub.o LpcStub.c -lpthread

clean:
	rm -f *.o
