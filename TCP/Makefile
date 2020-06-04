CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread

CLIENT = client.cpp
SERVER = server.cpp
CLI = client
SER = server
FUNCTIONS = funciones.h

all: server client
  
server: $(SERVER)
	$(CC) $(SERVER) -o $(SER)  $(OPENCV) $(PTHREAD) $(FUNCTIONS)
client: $(CLIENT)
	$(CC) $(CLIENT) -o $(CLI)  $(OPENCV) $(PTHREAD) $(FUNCTIONS)

.PHONY: clean

clean:
	rm $(CLI) $(SER)
