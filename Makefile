all: noisy_link remote_host base_station sensor_network

noisy_link: noisy_link.c
	g++ noisy_link.c -g -o bin/noisy_link

socket_utils.o: socket_utils.cpp
	g++ -c -Wall socket_utils.cpp -o bin/socket_utils.o  

remote_host: remote_host.cpp socket_utils.o
	g++ remote_host.cpp bin/socket_utils.o -g -o bin/remote_host

base_station: base_station.cpp socket_utils.o
	g++ base_station.cpp bin/socket_utils.o -g -o bin/base_station

sensor_network: sensor_network.c
	g++ sensor_network.c -g -o bin/sensor_network

clean:
	rm -rf bin/*
