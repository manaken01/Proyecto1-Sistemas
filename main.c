//Proyecto 1 de sistemas operativos 
//Creado por Mariangeles Carranza y Anthony Jiménez-2023 
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 8192
/*struct message {
  long type;
  char nums1[MSGSZ];
  int nums2[MSGSZ];
} msg;*/

int main (int argc, char* argv[]) {

    if (argc != 3) {
        printf("El formato correcto para utilizar el programa es: grep ¨'pattern1|pattern2|pattern3|...' archivo¨ \n");
        return 1;
    }

    clock_t start_time, end_time;
    double elapsed_time;

    int i;
    pid_t pid; //Variable para guardar el id de los procesos
    char *filename = argv[2]; //Se guarda el nombre del archivo
    int numProcesos = 2; //Los procesos que se crearan del programa
    long fileSize; //Variable para guardar el tamaño del archivo
    key_t msqkey = 999; //Key de los mensajes
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); //Id para pasar los mensajes

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    for(i = 0; i < numProcesos; i++) {
      pid = fork();
      if (pid == 0)
        break; //Si es hijo se sale
    }

    if (pid != 0) {

    } else {
        sleep(1);
        msgrcv(msqid, &msg, MSGSZ, 1, 0);
    }

    return 0;
}