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

	if(argc != 8) {
		fprintf(stderr, "usage: ./sender <receiver IP> <agent IP> <sender port> <agent port> <receiver port> <command> <pathname>");
		exit(1);
	}
	else {
		sscanf("127.0.0.1", "%s", sender_ip);
		sscanf(argv[1], "%s", receiver_ip);
		sscanf(argv[2], "%s", agent_ip);

		sscanf(argv[3], "%d", &sender_port);
		sscanf(argv[4], "%d", &agent_port);
		sscanf(argv[5], "%d", &receiver_port);

		sprintf(path, "sender_folder/%s", argv[7]);
	}

	int sender_socket, agent_socket;
	struct sockaddr_in sender_addr, agent_addr, tmp_addr;
	socklen_t agent_size, tmp_size;

	sender_socket = socket(PF_INET, SOCK_DGRAM, 0);

    sender_addr.sin_family = AF_INET;
    sender_addr.sin_port = htons(sender_port);
    sender_addr.sin_addr.s_addr = inet_addr(sender_ip);
    memset(sender_addr.sin_zero, '\0', sizeof(sender_addr.sin_zero));  

    bind(sender_socket, (struct sockaddr *)&sender_addr, sizeof(sender_addr));

    agent_addr.sin_family = AF_INET;
    agent_addr.sin_port = htons(agent_port);
    agent_addr.sin_addr.s_addr = inet_addr(agent_ip);
    memset(agent_addr.sin_zero, '\0', sizeof(agent_addr.sin_zero));  

    agent_size = sizeof(agent_addr);
    tmp_size = sizeof(tmp_addr);

    fd_set receive_set;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10;

    char mpg_extension[16] = ".mpg";

    int type;

    int threshold = 16;
    int winSize = 1;
    int sent = 0, ack = 0, index = 1;
    int t_sent = 0;
    int end = 0;

    Mat img;
    VideoCapture cap(path);
    int imgSize, segs_per_frame;
    int frameHead = 0;
    int seg_of_frame = 0;
    int at_frame = 0;
    int accumulated = 0;
    int width, height;
    int framebase = 1;
    uchar *bufff;

    segment first_seg, buff, received;
    memset(&first_seg, 0, sizeof(first_seg));
    memset(&buff, 0, sizeof(buff));
    memset(&received, 0, sizeof(received));

    FILE *fp;

    if(strcmp(argv[6], "send") == 0) {
    	type = SEND;
    	fp = fopen(path, "r");
		if(fp == NULL) {
			fprintf(stderr, "cannot open %s in sender_folder", argv[7]);
			exit(1);
		}
		sprintf(first_seg.data, "send %s", argv[7]);
	}
	else if(strcmp(argv[6], "play") == 0) {
		type = PLAY;
		if(strstr(argv[7], mpg_extension) == NULL) {
			fprintf(stderr, "'%s' cannot be played", argv[7]);
			exit(1);
		}

    	width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    	height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    	
   	 	img = Mat::zeros(height, width, CV_8UC3);    

    	if(!img.isContinuous()){
        	 img = img.clone();
    	}
    	cap >> img;
        imgSize = img.total() * img.elemSize();
        bufff = (uchar*) malloc(imgSize);
        memcpy(bufff, img.data, imgSize);
		sprintf(first_seg.data, "play %d %d %d", imgSize, width, height);
	}
	else {
		fprintf(stderr, "command '%s' not found", argv[6]);
		exit(1);
	}
	
    //CONTENT SEG AND ACK
    if(type == SEND) {
    	//FILE TRANSMISSION
   		while(1) {
   			for(int i = 0; i < winSize; i++) {
   				memset(&buff, 0, sizeof(buff));
   				int set = index - 2;
   				if(set < 0) {
   					set = 0;
  				}
   				fseek(fp, set * 1000 , SEEK_SET);
   				if(index == 1) {
    				setHeader(&first_seg, index, 0, 0, 0, 0);
    				sendto(sender_socket, &first_seg, sizeof(first_seg), 0, (struct sockaddr *) &agent_addr, agent_size);
    				if(index <= t_sent) {
    					fprintf(stderr, "resend\tdata\t#%d,\twinSize = %d\n", index, winSize);
    					sent++;
    				}
    				else {
    					fprintf(stderr, "send\tdata\t#%d,\twinSize = %d\n", index, winSize);
    					sent++;
    					t_sent++;
    				}
    				index++;
    			}
    			else if(fread(buff.data, sizeof(char), 1000, fp) > 0) {
    				setHeader(&buff, index, 0, 0, 0, 0);
    				sendto(sender_socket, &buff, sizeof(buff), 0, (struct sockaddr *) &agent_addr, agent_size);
    				if(index <= t_sent) {
    					fprintf(stderr, "resend\tdata\t#%d,\twinSize = %d\n", index, winSize);
    					sent++;
    				}
    				else {
    					fprintf(stderr, "send\tdata\t#%d,\twinSize = %d\n", index, winSize);
    					sent++;
    					t_sent++;
    				}
    				index++;
    			}
    			else {
    				end = 1;
    				break;
    			}
    		}

    		int winSize_t = winSize;
   		 	if(winSize < threshold) {
   				winSize *= 2;
   			}
    		else {
    			winSize++;
    		}

    		while(ack < sent) {
    			//TIMEOUT
    			FD_ZERO(&receive_set);
    			FD_SET(sender_socket, &receive_set);
    			if(!select(sender_socket + 1, &receive_set, NULL, NULL, &timeout)) {
    				threshold = winSize_t / 2;
    				if(threshold < 1) {
    					threshold = 1;
    				}
    				winSize = 1;
    				fprintf(stderr, "time\tout\t\tthreshold = %d\n", threshold);
    				index = ack + 1;
    				sent = ack;
    				break;
    			}
    			recvfrom(sender_socket, &received, sizeof(received), 0, (struct sockaddr *)&agent_addr, (socklen_t *)&agent_size);
    			fprintf(stderr, "recv\tack\t#%d\n", received.head.ackNumber);
    			ack = received.head.ackNumber;
    			index = ack + 1;
    		}	

    		if(end == 1 && ack == t_sent) {
    			break;
    		}
    	}
	}
	else {
		//VIDEO STREAMING
		while(1) {
			char c = (char) waitKey(33.3333);
			if(c == 27) {
				break;
			}
			while(1) {
				for(int i = 0; i < winSize; i++) {
   					memset(&buff, 0, sizeof(buff));
   					if(index == 1) {
    					setHeader(&first_seg, index, 0, 0, 0, 0);
    					sendto(sender_socket, &first_seg, sizeof(first_seg), 0, (struct sockaddr *) &agent_addr, agent_size);
    					if(index <= t_sent) {
    						fprintf(stderr, "resend\tdata\t#%d,\twinSize = %d\n", index, winSize);
    						sent++;
    					}
    					else {
    						fprintf(stderr, "send\tdata\t#%d,\twinSize = %d\n", index, winSize);
    						sent++;
    						t_sent++;
    					}
    					index++;
    				}
    				else {
    					if((imgSize - accumulated) < 1000) {
    						memcpy(buff.data, bufff + accumulated, imgSize - accumulated);
    						setVHeader(&buff, index, imgSize - accumulated, 0, 0, 0, 0);
    						accumulated += imgSize - accumulated;
    					}
    					else {
    						memcpy(buff.data, bufff + accumulated, 1000);
    						setVHeader(&buff, index, 1000, 0, 0, 0, 0);
    						accumulated += 1000;
    					}
    					
    					sendto(sender_socket, &buff, sizeof(buff), 0, (struct sockaddr *) &agent_addr, agent_size);

    					if(index <= t_sent) {
    						fprintf(stderr, "resend\tdata\t#%d,\twinSize = %d\n", index, winSize);
    						sent++;
    					}
    					else {
    						fprintf(stderr, "send\tdata\t#%d,\twinSize = %d\n", index, winSize);
    						sent++;
    						t_sent++;
    					}
    					index++;
    					if(accumulated == imgSize) {
    						break;
    					}
    				}
    			}	

    			int winSize_t = winSize;
   		 		if(winSize < threshold) {
   					winSize *= 2;
   				}
    			else {
    				winSize++;
    			}	

    			while(ack < sent) {
    				//TIMEOUT
    				FD_ZERO(&receive_set);
    				FD_SET(sender_socket, &receive_set);
    				if(!select(sender_socket + 1, &receive_set, NULL, NULL, &timeout)) {
    					threshold = winSize_t / 2;
    					if(threshold < 1) {
    						threshold = 1;
    					}
    					winSize = 1;
    					fprintf(stderr, "time\tout\t\tthreshold = %d\n", threshold);
    					index = ack + 1;
    					sent = ack;
    					accumulated = (ack - framebase) * 1000; 
    					break;
    				}
    				recvfrom(sender_socket, &received, sizeof(received), 0, (struct sockaddr *)&agent_addr, (socklen_t *)&agent_size);
    				fprintf(stderr, "recv\tack\t#%d\n", received.head.ackNumber);
    				ack = received.head.ackNumber;
    				index = ack + 1;
    				//accumulated = (ack - framebase) * 1000;
    			}
    			if(accumulated == imgSize) {
    				framebase = index - 1;
    				accumulated = 0;
    				break;
    			}
    			if(end == 1 && ack == t_sent) {
    				break;
    			}
			}
			img = Mat::zeros(height, width, CV_8UC3);    
    		if(!img.isContinuous()){
        		img = img.clone();
    		}
			cap >> img;
			memset(bufff, 0, sizeof(bufff));
			memcpy(bufff, img.data, imgSize);
   		}
	}
    //FIN AND FINACK
    setHeader(&buff, index, 0, 1, 0, 0);
    sendto(sender_socket, &buff, sizeof(buff), 0, (struct sockaddr *) &agent_addr, agent_size);
    fprintf(stderr, "send\tfin\n");
    recvfrom(sender_socket, &received, sizeof(received), 0, (struct sockaddr *)&agent_addr, (socklen_t *)&agent_size);
    fprintf(stderr, "recv\tfinack\n");
    if(type == SEND) {
		fclose(fp);
	}
	return 0;
}

