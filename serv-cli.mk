#fisier folosit pentru compilarea serverului&clientului UDP
#	g++ serverAttempt.cpp -o serv -lsfml-graphics -lsfml-window -lsfml-system
#	g++ clientAttempt.cpp -o cli -lsfml-graphics -lsfml-window -lsfml-system
all:
	g++ serverAttempt.cpp -o serv -lsfml-graphics -lsfml-window -lsfml-system
	g++ clientAttempt.cpp -o cli -lsfml-graphics -lsfml-window -lsfml-system
clean:
	rm -f cliUdp servUdp