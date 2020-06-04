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
#include "funciones.h"
#include <signal.h>

#define BUFF_SIZE 1024
using namespace std;
using namespace cv;
int main(int argc, char** argv){
	signal(SIGPIPE, SIG_IGN);
	char in[32];
	strcpy(in, argv[1]);
    char ip[16], puertoc[16];
	int puerto;
    char *head;
    int written = 0;

	//split ip:port
    head = strtok(in, ":");
    while(head != NULL) {
        if(written == 0) {
            strcpy(ip, head);
			written++;
        }
        else if(written == 1) {
        	strcpy(puertoc, head);
			written++;
        }
        else break;
        head = strtok(NULL, ":");
    }
	puerto = atoi(puertoc);
	
	char foldername[1024];
	sprintf(foldername, "client_folder");
    crearCarpeta(foldername);

	//create socket and establish connection
    int clientesocket;
    struct sockaddr_in clienteaddr;
    
    clientesocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientesocket < 0) {
        fprintf(stderr, "socket() failed.\n");
        exit(1);
    }
    
    clienteaddr.sin_family = PF_INET;
    clienteaddr.sin_addr.s_addr = inet_addr(ip);
    clienteaddr.sin_port = htons(puerto);
    
    if(connect(clientesocket, (struct sockaddr *)&clienteaddr, sizeof(clienteaddr)) < 0) {
        fprintf(stderr, "connect() failed.\n");
        exit(1);
    }
    
    fprintf(stderr, "Connected to %s:%d succesfully.\n", ip, puerto);
	
	//ready for service
    while(1) {
		char comando[1024] = {};
		char mensajerecibido[1024] = {};
		int enviado, recibido;

        fgets(comando, sizeof(char) * 1024, stdin);
		char cmd[512] = {}, arg[512] = {};
		sscanf(comando, "%s%s", cmd, arg);

		if(strcmp(cmd, "ls") == 0) {//LS
			enviado = send(clientesocket, comando, strlen(comando), 0);
			recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);
			fprintf(stderr, "%s", mensajerecibido);
		}
		else if(strcmp(cmd, "put") == 0) {//PUT
			if(strlen(arg) > 0 &&(comprobarArchivoEnDir(foldername, arg) == -1)) {
				fprintf(stderr, "The %s doesn't exist.\n", arg);
			}
			else {
                char filename[1024] = {};
                sprintf(filename, "client_folder/%s", arg);
                FILE *fp = fopen(filename, "r");
                if(fp == NULL) {
                    fprintf(stderr, "Cannot open %s.\n", filename);
                }
                else {
                    //send command
                    enviado = send(clientesocket, comando, strlen(comando), 0);
                    recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);
        
                    if(strcmp(mensajerecibido, "Server starts receiving file.\n") == 0) {
                        fprintf(stderr, "%s", mensajerecibido);

                        //send file
                        int n;
                        char buf[1024] = {};
                        while((n = fread(buf, sizeof(char), 1024, fp)) > 0) {
                            if(n != 1024 && ferror(fp)) {
                                fprintf(stderr, "Read file error.\n");
                                break;
                            }
                            if(send(clientesocket, buf, n, 0) < 0) {
                                fprintf(stderr, "Cannot send file.\n");
                                break;
                            }
                            memset(buf, 0, 1024);
                        }
						fclose(fp);
						sprintf(comando, "..END..OF..FILE..");
						enviado = send(clientesocket, comando, strlen(comando), 0);
                        recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);
                        fprintf(stderr, "%s", mensajerecibido);
                    }
                    else {//error in server
                        fprintf(stderr, "%s", mensajerecibido);
                    }
                }
			}
		}
		else if(strcmp(cmd, "get") == 0) {//GET
			enviado = send(clientesocket, comando, strlen(comando), 0);
			recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);

            if(strcmp(mensajerecibido, "Server starts sending file.\n") == 0) {
                fprintf(stderr, "%s", mensajerecibido);
				sprintf(comando, "Client starts receiving file.\n");
                enviado = send(clientesocket, comando, strlen(comando), 0);
				char filename[1024] = {};
           		sprintf(filename, "client_folder/%s", arg);
            	FILE *fp = fopen(filename, "w");
            	if(fp == NULL) {
            		fprintf(stderr, "Cannot open %s.\n", filename);
            	}
				fprintf(stderr, "Created %s.\n", filename);
				fflush(stderr);
                int success = 1;
                int n;
                char buf[1024] = {};
                while((n = recv(clientesocket, buf, 1024, 0)) > 0) {
                    if(strcmp(buf, "..END..OF..FILE..") == 0) {
                        break;
                    }
                    if(n < 0) {
                        fprintf(stderr, "Receive file error.\n");
                        success = 0;
                        break;
                    }
                    if(fwrite(buf, sizeof(char), n, fp) != n) {
                        fprintf(stderr, "Write file error.\n");
                        success = 0;
                        break;
                    }
                    memset(buf, 0, 1024);
                }
                fclose(fp);
				sprintf(comando, "client received file.\n");
				enviado = send(clientesocket, comando, strlen(comando), 0);
				recibido = recv(clientesocket, mensajerecibido, sizeof(char) * 1024, 0);
				fprintf(stderr, "%s", mensajerecibido);
                if(success == 1) {
                    fprintf(stderr, "File downloaded successfully.\n");
                }
                else {
                    fprintf(stderr, "Failed to download file.\n");
                }
            }
            else {
                fprintf(stderr, "%s", mensajerecibido);
            }
			
		}
		else if(strcmp(cmd, "play") == 0) {//PLAY
			enviado = send(clientesocket, comando, strlen(comando), 0);// send put command
			recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);//receive start or error
			if(strcmp(mensajerecibido, "Server starts sending frames.\n") == 0) {
				sprintf(comando, "!!!Client starts !!!receiving frames.\n");
				enviado = send(clientesocket, comando, strlen(comando), 0);
				recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);//receive width and height
				int width, height;
				sscanf(mensajerecibido, "%d,%d", &width, &height);
				memset(comando, 0, 1024);
				sprintf(comando, "WH received.\n");
				enviado = send(clientesocket, comando, strlen(comando), 0);
				Mat imgServer,imgClient;
				imgClient = Mat::zeros(height,width, CV_8UC3);
				if(!imgClient.isContinuous()){
					imgClient = imgClient.clone();
				}
				while(1){
					int imgSize;
					recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);//receive size
					sscanf(mensajerecibido, "%d", &imgSize);
					memset(comando, 0, 1024);
					sprintf(comando, "imgSize Received\n");
					enviado = send(clientesocket, comando, strlen(comando), 0);


					uchar buffer[imgSize];
			
					int total = 0;
					int left = imgSize;
					while(total < left) {
						recibido = recv(clientesocket, buffer + total, sizeof(char) * left, 0);
						total += recibido;
					}

					uchar *iptr = imgClient.data;
					memcpy(iptr,buffer,imgSize);
					  
					imshow("Video", imgClient);
					char c = (char)waitKey(33.3333);
					memset(comando, 0, 1024);
					if(c==27) {
						sprintf(comando, "Halt");
						enviado = send(clientesocket, comando, strlen(comando), 0);
						break;
					}
					sprintf(comando, "Cont");
					enviado = send(clientesocket, comando, strlen(comando), 0);
				}
				destroyAllWindows();
			}
			else {
				fprintf(stderr, "%s", mensajerecibido);
			}	
		}
		else {//OTHER
			enviado = send(clientesocket, comando, strlen(comando), 0);
			recibido = recv(clientesocket, mensajerecibido, sizeof(char)*1024, 0);
			fprintf(stderr, "%s", mensajerecibido);
		}		
    }
    
    exit(0);
}









