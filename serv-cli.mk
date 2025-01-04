#fisier folosit pentru compilarea serverului&clientului UDP

all:
	gcc serverAttempt.c -o serv
	gcc clientAttempt.c  -o cli
clean:
	rm -f cliUdp servUdp