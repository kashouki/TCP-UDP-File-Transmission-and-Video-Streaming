#include "opencv2/opencv.hpp"
#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <signal.h>
#include <sys/time.h>
#include "functions.h"

using namespace std;
using namespace cv;


int main(int argc, char** argv){
	char sender_ip[32], receiver_ip[32], agent_ip[32];
	int sender_port, receiver_port, agent_port;
	char path[512];
	char command[16];

	if(argc != 6) {
		fprintf(stderr, "usage: ./receiver <sender IP> <agent IP> <sender port> <agent port> <receiver port>");
		exit(1);
	}
	else {
		sscanf("127.0.0.1", "%s", receiver_ip);
		sscanf(argv[1], "%s", sender_ip);
		sscanf(argv[2], "%s", agent_ip);

		sscanf(argv[3], "%d", &sender_port);
		sscanf(argv[4], "%d", &agent_port);
		sscanf(argv[5], "%d", &receiver_port);
	}

	int receiver_socket, agent_socket;
	struct sockaddr_in receiver_addr, agent_addr, tmp_addr;
	socklen_t agent_size, tmp_size;

	receiver_socket = socket(PF_INET, SOCK_DGRAM, 0);

    receiver_addr.sin_family = AF_INET;
    receiver_addr.sin_port = htons(receiver_port);
    receiver_addr.sin_addr.s_addr = inet_addr(receiver_ip);
    memset(receiver_addr.sin_zero, '\0', sizeof(receiver_addr.sin_zero));  

    bind(receiver_socket, (struct sockaddr *)&receiver_addr, sizeof(receiver_addr));


    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent_port);
    agent_addr.sin_addr.s_addr = inet_addr(agent_ip);
    memset(agent_addr.sin_zero, '\0', sizeof(agent_addr.sin_zero));  

    agent_size = sizeof(agent_addr);
    tmp_size = sizeof(tmp_addr);

    int ack = 0, index = 0;
    int imgHead = 0, buffHead = 0;

    segment buff[32], received;
    memset(buff, 0, sizeof(buff));
    memset(&received, 0, sizeof(received));

    FILE *fp;
    int type;

    Mat img;
    int segs_per_frame, imgSize, width, height;
    int accumulated = 0;
    uchar *bufff;

	while(1) {
		if(recvfrom(receiver_socket, &received, sizeof(received), 0, (struct sockaddr *)&agent_addr, (socklen_t *)&agent_size)) {
			/*DATA*/
			if(received.head.fin == 0) {
				if(received.head.seqNumber == ack + 1) {
					fprintf(stderr, "recv\tdata\t#%d\n", received.head.seqNumber);
					if(received.head.seqNumber == 1) {
						if(strstr(received.data, "send") != NULL) {
							type = SEND;
							sscanf(received.data, "%s %s", command, path);
							char pathname[512];
							sprintf(pathname, "receiver_folder/%s", path);
							fp = fopen(pathname, "w");
						}
						else if(strstr(received.data, "play") != NULL) {
							type = PLAY;
							sscanf(received.data, "%s %d %d %d",command, &imgSize, &width, &height);
							bufff = (uchar*) malloc(imgSize);
							img = Mat::zeros(height, width, CV_8UC3);
							if(!img.isContinuous()){
         						img = img.clone();
    						}
						}
						else {
							fprintf(stderr, "cannot identify transmission type");
							exit(1);
						}
					}
					else {
						memcpy(&buff[index], &received, sizeof(received));
						index++;
					}

					ack = received.head.seqNumber;
					setHeader(&received, 0, ack, 0, 0, 1);
					sendto(receiver_socket, &received, sizeof(received), 0, (struct sockaddr *) &agent_addr, agent_size);
					fprintf(stderr, "send\tack\t#%d\n", ack);
				}
				else {
					fprintf(stderr, "drop\tdata\t#%d\n", received.head.seqNumber);
					setHeader(&received, 0, ack, 0, 0, 1);
					sendto(receiver_socket, &received, sizeof(received), 0, (struct sockaddr *) &agent_addr, agent_size);
					fprintf(stderr, "send\tack\t#%d\n", ack);
				}
			}
			/*FIN*/
			else {
				fprintf(stderr, "recv\tfin\n");
				setHeader(&received, 0, ack, 1, 0, 1);
   				sendto(receiver_socket, &received, sizeof(received), 0, (struct sockaddr *) &agent_addr, agent_size);
   				fprintf(stderr, "send\tfinack\n");
   				break;
			}
		}
		if(index == 32) {
			for(int i = 0; i < 32; i++) {
				if(type == SEND) {
					fwrite(buff[i].data, 1, buff[i].head.length, fp);
				}
				else {
					memcpy(bufff + imgHead, buff[i].data, buff[i].head.length);
					imgHead += buff[i].head.length;
					if(imgHead >= imgSize) {
						memcpy(img.data, bufff, imgSize);
						imshow("Video", img);
						imgHead = 0;
						img = Mat::zeros(height, width, CV_8UC3);
						if(!img.isContinuous()){
         					img = img.clone();
    					}
    					memset(bufff, 0, sizeof(bufff));
    					char c = (char)waitKey(33.3333);
        				if(c==27) break;
					}
				}
			}
			index = 0;
			fprintf(stderr, "flush\n");
			memset(&received, 0, sizeof(received));
			memset(buff, 0, sizeof(buff));
		}
	}
	for(int i = 0; i < index; i++) {
		if(type == SEND) {
			fwrite(buff[i].data, 1, buff[i].head.length, fp);
		}
		else {
			memcpy(bufff + imgHead, buff[i].data, buff[i].head.length);
			imgHead += buff[i].head.length;
			if(imgHead >= imgSize) {
				memcpy(img.data, bufff, imgSize);
				imshow("Video", img);
				imgHead = 0;
				img = Mat::zeros(height, width, CV_8UC3);
				if(!img.isContinuous()){
        			img = img.clone();
    			}
   				memset(bufff, 0, sizeof(bufff));
    			char c = (char)waitKey(33.3333);
       			if(c==27) break;
			}
		}
	}
	fprintf(stderr, "flush\n");
	if(type == SEND) {
		fclose(fp);
	}
	return 0;
}