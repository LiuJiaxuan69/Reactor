client=client_cal.o
server=main.o

.PHONY:all
all:$(client) $(server)

$(client):client_cal.cc
	g++ -o $@ $^ -std=c++17 -ljsoncpp
$(server):main.cc
	g++ -o $@ $^ -std=c++17 -ljsoncpp

.PHONY:clean
clean:
	rm -rf *.o