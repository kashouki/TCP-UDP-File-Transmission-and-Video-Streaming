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
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int newfd;
    
    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    
    int servidorsocket, clientesocket;
    int puerto = atoi(argv[1]);
    struct sockaddr_in servidoraddr, clienteaddr;
    unsigned int addrlen = sizeof(struct sockaddr_in);
    
	char foldername[1024];
	sprintf(foldername, "server_folder");
    crearCarpeta(foldername);
    
    servidorsocket = socket(AF_INET, SOCK_STREAM, 0);
    if(servidorsocket < 0) {
        fprintf(stderr, "socket() failed.\n");
        exit(1);
    }
    
    servidoraddr.sin_family = AF_INET;
    servidoraddr.sin_addr.s_addr = INADDR_ANY;
    servidoraddr.sin_port = htons(puerto);
    
    if(bind(servidorsocket, (struct sockaddr *) &servidoraddr, sizeof(servidoraddr)) < 0) {
        fprintf(stderr, "bind() failed.\n");
        exit(1);
    }
    
    if(listen(servidorsocket, 3) < 0) {
        fprintf(stderr, "listen() failed.\n");
        exit(1);
    }
    
    FD_SET(servidorsocket, &master);
    fdmax = servidorsocket;
    
    while(1) {
        read_fds = master;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            fprintf(stderr,"select() error.\n");
            exit(1);
        }

        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == servidorsocket) {
                    addrlen = sizeof clienteaddr;
                    newfd = accept(servidorsocket,
                                   (struct sockaddr *)&clienteaddr,
                                   &addrlen);
                    
                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); 
                        if (newfd > fdmax) {   
                            fdmax = newfd;
                        }
                        printf("Connection accepted\n");
                    }
                } else {
					char buf[1024] = {};
					char comandorecibido[1024] = {};
                    char mensaje[1024] = {};
                    int recibido, enviado;
                    if ((recibido = recv(i, comandorecibido, sizeof(char) * 1024, 0)) <= 0) {
                        if (recibido == 0) {
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master);
                    } else {
                        char comando[512] = {}, arg[512] = {};
                        sscanf(comandorecibido, "%s%s", comando, arg);
                        
                        if(strcmp(comando, "ls") == 0) {//LS
                            DIR *d;
                            struct dirent *dir;
                            d = opendir("server_folder");
                            if(d) {
                                while((dir = readdir(d)) != NULL) {
                                    if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                                        strcat(mensaje, dir->d_name);
                                        strcat(mensaje, " ");
                                    }
                                }
                                strcat(mensaje, "\n");
                                closedir(d);
                            }
                            enviado = send(i, mensaje, strlen(mensaje), 0);
                        }
                        else if(strcmp(comando, "put") == 0) {//PUT
                            if(strlen(arg) == 0) {
                                sprintf(mensaje, "Command format error.\n");
                                enviado = send(i, mensaje, strlen(mensaje), 0);
                            }
                            else{
                                char filename[1024] = {};
                                sprintf(filename, "server_folder/%s", arg);
                                FILE *fp = fopen(filename, "w");
                                if(fp == NULL) {
                                    fprintf(stderr, "Cannot open %s.\n", filename);
                                    sprintf(mensaje, "Server failed to create file.\n");
                                    enviado = send(i, mensaje, strlen(mensaje), 0);
                                }
                                else {
                                    //receive file
                                    sprintf(mensaje, "Server starts receiving file.\n");
                                    enviado = send(i, mensaje, strlen(mensaje), 0);
                                    int success = 1;
                                    int n;
                                    char buf[1024] = {};
                                    while((n = recv(i, buf, 1024, 0)) > 0) {
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
                                    if(success == 1) {
                                        sprintf(mensaje, "File uploaded successfully.\n");
                                    }
                                    else {
                                        sprintf(mensaje, "Failed to upload file.\n");
                                    }
                                    enviado = send(i, mensaje, strlen(mensaje), 0);
                                }
                            }
                        }
                        else if(strcmp(comando, "get") == 0) {//GET
                            if(strlen(arg) == 0) {
                                sprintf(mensaje, "Command format error.\n");
                                enviado = send(i, mensaje, strlen(mensaje), 0);
                            }
                            else{
                                if(comprobarArchivoEnDir(foldername, arg) == -1) {
                                    sprintf(mensaje, "The %s doesn't exist.\n", arg);
                                    enviado = send(i, mensaje, strlen(mensaje), 0);
                                }
                                else {
                                    sprintf(mensaje, "The %s exists.\n", arg);
                                    char filename[1024] = {};
                                    sprintf(filename, "server_folder/%s", arg);
                                    FILE *fp = fopen(filename, "r");
                                    if(fp == NULL) {
                                        fprintf(stderr, "Cannot open %s.\n", filename);
                                        sprintf(mensaje, "Server cannot open file.\n");
                                        enviado = send(i, mensaje, strlen(mensaje), 0);
                                    }
                                    else {
                                        sprintf(mensaje, "Server starts sending file.\n");
                                        enviado = send(i, mensaje, strlen(mensaje), 0);
                                        recibido = recv(i, comandorecibido, sizeof(char) * 1024, 0);
                                        fprintf(stderr, "%s", comandorecibido);
                                        //send file
                                        int n;
                                        char buf[1024] = {};
                                        while((n = fread(buf, sizeof(char), 1024, fp)) > 0) {
                                            if(n != 1024 && ferror(fp)) {
                                                fprintf(stderr, "Read file error.\n");
                                                break;
                                            }
                                            if(send(i, buf, n, 0) < 0) {
                                                fprintf(stderr, "Cannot send file.\n");
                                                break;
                                            }
                                            memset(buf, 0, 1024);
                                        }
                                        fclose(fp);
                                        sprintf(mensaje, "..END..OF..FILE..");
                                        enviado = send(i, mensaje, strlen(mensaje), 0);
										recibido = recv(i, comandorecibido, sizeof(char) * 1024, 0);
										fprintf(stderr, "%s", comandorecibido);
                                        sprintf(mensaje, "Server finished sending file.\n");
                                        enviado = send(i, mensaje, strlen(mensaje), 0);
                                    }
                                }
                            }
                        }
                        else if(strcmp(comando, "play") == 0) {//PLAY
                            if(strlen(arg) == 0) {
                                sprintf(mensaje, "Command format error.\n");
                                enviado = send(i, mensaje, strlen(mensaje), 0);
                            }
                            else {
                                if(comprobarArchivoEnDir(foldername, arg) == 1) {
                                    sprintf(mensaje, "Server starts sending frames.\n");
									enviado = send(i, mensaje, strlen(mensaje), 0);
									char filename[1024] = {};
									sprintf(filename, "server_folder/%s", arg);
									Mat imgServer;
									VideoCapture cap(filename);

									recibido = recv(i, comandorecibido, sizeof(char) * 1024, 0);//receive client start frame

									int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
									int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
									cout  << width << ", " << height << endl;
									sprintf(mensaje, "%d,%d", width, height);
									enviado = send(i, mensaje, strlen(mensaje), 0);	
		
									imgServer = Mat::zeros(height,width, CV_8UC3);    
									if(!imgServer.isContinuous()){
										 imgServer = imgServer.clone();
									}
									memset(comandorecibido, 0, 1024);
									recibido = recv(i, comandorecibido, sizeof(char) * 1024, 0);
									
									while(1){
										cap >> imgServer;
										int imgSize = imgServer.total() * imgServer.elemSize();
										sprintf(mensaje, "%d", imgSize);
										enviado = send(i, mensaje, strlen(mensaje), 0);
										memset(comandorecibido, 0, 1024);
										recibido = recv(i, comandorecibido, sizeof(char) * 17, 0);
										if(strcmp(comandorecibido, "imgSize Received\n") != 0) {
											break;
										}

										uchar buffer[imgSize];
										memcpy(buffer,imgServer.data, imgSize);

										int total = 0;
										int left = imgSize;
										while(total < left) {
											enviado = send(i, buffer + total, left, 0);
											total += enviado;
										}			
	
										memset(comandorecibido, 0, 1024);
										recibido = recv(i, comandorecibido, sizeof(char) * 4, 0);
										if(strcmp(comandorecibido, "Halt") == 0) break;
									}
									cap.release();
									destroyAllWindows();
                                }
                                else {
                                    sprintf(mensaje, "The %s doesn't exist.\n", arg);
                                }
                            }
                        }
                        else {//OTHER
                            sprintf(mensaje, "Command not found.\n");
                            enviado = send(i, mensaje, strlen(mensaje), 0);
                        }
                        
                        
                        
                    }
                } 
            } 
        } 
    }
	exit(0);
}
