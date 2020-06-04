# TCP/UDP File Transmission and Video Streaming

This repository contains two file transmitting and video streaming applications, which use the TCP and UDP protocols respectively.

## Requrements

- GCC
- OpenCV 3.0 or higher

## 1. TCP Application

In this application, a client can list the files in the server, download files from the server, upload files to the server, and stream a .mpg video on the server.

### Usage

1. In the command line shell, change to the TCP directory and type `make`. The programs will be compiled with the provided MAKEFILE.
2. Type `./server [port]` in the shell to launch the server with a specific port. Here is an example: `./server 22`
4. Open another shell and change to the directory where the program "client" is. Type `./client [ip:port]` in the shell to launch a client and connect to the server. The `ip` is the IP address of the server and `port` is the same in step two. Here is an example: `./client 127.0.0.1:22`
5. Now, you can use the following command in the client:
    1. `ls` : the files in the server folder will be listed.
    2. `put <filename>` : upload a file in the client folder to the server folder.
    3. `get <filename>` : download a file from the server folder to the client folder.
    4. `play <videofile>` : stream a .mpg video in the server folder.

## 2. UDP Application

In this application, the sender can send a file to the folder of the receiver, or send a .mpg video frame by frame to the receiver. This application uses GO-BACK-N and congestion control to deal with package loss.

### Usage

1. In the command line shell, change to the UDP directory and type `make`. The programs will be compiled with the provided MAKEFILE.
2. Type `./agent <sender ip> <receiver ip> <sender port> <agent port> <receiver port> <lost rate>` in the shell to launch an agent which simulates package loss in the network. Here is an example: `./agent 127.0.0.1 127.0.0.1 5000 5001 5002 0.3`
3. Open another shell and change to the directory where the porgram "receiver" is. Type `./receiver <sender ip> <agent ip> <sender port> <agent port> <receiver port>` in the shell. Here is an example: `./receiver 127.0.0.1 127.0.0.1 5000 5001 5002`
4. Open another shell and change to the  directory where the program "sender" is. Type `./sender <receiver ip> <agent ip> <sender port> <agent port> <receiver port> <command> <filename>` in the shell. The following are the command options:
    1. `send` : the file specified in `<filename>` will be sent from the sender folder to the receiver folder.
    2. `play` : the video specified in `<filename>` in the sender folder will be played at the receiver end.

    Here are some examples: 
    `./sender 127.0.0.1 127.0.0.1 5000 5001 5002 send file.txt`
    `./sender 127.0.0.1 127.0.0.1 5000 5001 5002 play video.mpg`

## Note
- The TCP application supports multiple connections from clients to the server. The UDP application only support one sender, one receiver, and one agent at the same time.
- I did not implement the agent in the UDP application.
