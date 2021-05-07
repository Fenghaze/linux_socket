CC=g++
CFLAGS+=-pthread -c -g -Wall
LDFLAGS+=-pthread

Server: http.o 15-6WebServer.cpp
	$(CC) -fdump-rtl-expand -pthread -g -Wall $^ -o $@

%.o: %.cpp
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *.o