#include <stdio.h>
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

/*CREATE FOLDER WITH FOLDERNAME*/
void crearCarpeta(char nombre[]) {
	struct stat st = {0};
	if(stat(nombre, &st) < 0) {
		if(mkdir(nombre, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
			fprintf(stderr, "mkdir() failed for %s.\n", nombre);
			exit(1);
		}
		else {
			fprintf(stderr, "%s created.\n", nombre);
		}
	}
}

/*CHECK IF <FILENAME> IN <DIRECTORY>*/
/*IF YES RETURN 1, IF NO RETURN -1*/
int comprobarArchivoEnDir(char directorio[], char nombre[]) {
    int found = -1;
    DIR *d;
    struct dirent *dir;
    d = opendir(directorio);
    if(d) {
        while((dir = readdir(d)) != NULL) {
            if(strcmp(dir->d_name, nombre) == 0) {
                found = 1;
            }
        }
        closedir(d);
    }
    return found;
}
