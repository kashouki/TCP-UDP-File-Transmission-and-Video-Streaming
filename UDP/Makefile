CC = g++
C- = gcc
OPENCV =  `pkg-config --cflags --libs opencv`
PTHREAD = -pthread

SENDER = sender.cpp
RECEIVER = receiver.cpp
AGENT = agent.c
SEN = sender
REC = receiver
AGE = agent
FUNC = functions.h

all: sender receiver agent
  
sender: $(SENDER)
	$(CC) $(SENDER) -o $(SEN)  $(OPENCV) $(FUNC)
receiver: $(RECEIVER)
	$(CC) $(RECEIVER) -o $(REC)  $(OPENCV) $(FUNC)
agent: $(AGENT)
	$(C-) $(AGENT) -o $(AGE)

.PHONY: clean

clean:
	rm $(SEN) $(REC) $(AGE)
