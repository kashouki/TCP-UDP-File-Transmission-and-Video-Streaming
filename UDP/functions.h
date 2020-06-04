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

#define SEND 0
#define PLAY 1

typedef struct {
    int length;
    int seqNumber;
    int ackNumber;
    int fin;
    int syn;
    int ack;
} header;

typedef struct{
    header head;
    char data[1000];
} segment;

/*CREATE FOLDER WITH FOLDERNAME*/
/*IF YES RETURN 1, IF NO RETURN -1*/
int createFolder(char pathname[]) {
	struct stat st = {0};
	if(stat(pathname, &st) < 0) {
		if(mkdir(pathname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
			return -1;
		}
		else {
			return 1;
		}
	}
}

/*CHECK IF <FILENAME> IN <DIRECTORY>*/
/*IF YES RETURN 1, IF NO RETURN -1*/
int checkFileInDir(char dirname[], char filename[]) {
    int found = -1;
    DIR *d;
    struct dirent *dir;
    d = opendir(dirname);
    if(d) {
        while((dir = readdir(d)) != NULL) {
            if(strcmp(dir->d_name, filename) == 0) {
                found = 1;
            }
        }
        closedir(d);
    }
    return found;
}

/*SET SEGMENT HEADER TO THE ASSIGNED VALUES*/
void setHeader(segment* seg, int seqNumber, int ackNumber, int fin, int syn, int ack) {
    seg->head.length = sizeof(seg->data);
    if(seg->head.length > 1000) {
        seg->head.length = 1000;
    }
    seg->head.seqNumber = seqNumber;
    seg->head.ackNumber = ackNumber;
    seg->head.fin = fin;
    seg->head.syn = syn;
    seg->head.ack = ack;
}

void setVHeader(segment* seg, int seqNumber, int length, int ackNumber, int fin, int syn, int ack) {
    seg->head.length = length;
    if(seg->head.length > 1000) {
        seg->head.length = 1000;
    }
    seg->head.seqNumber = seqNumber;
    seg->head.ackNumber = ackNumber;
    seg->head.fin = fin;
    seg->head.syn = syn;
    seg->head.ack = ack;
}