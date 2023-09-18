//Proyecto 1 de sistemas operativos 
//Creado por Mariangeles Carranza y Anthony Jiménez-2023 
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>
#include <time.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define BUFFER_SIZE 8192 //El tamaño del buffer
#define numProcesos 5 //El número de los procesos

#define MSGSZ 128
struct message {
  long type;
  int flag; //La posición 0 se encuentra el flag para saber si termino de leer o procesar, la posición 1 contendra la ultima posición del buffer
  int newStartP;
  int activos[numProcesos];
} msg;

long calculateLinesBeforeStart(FILE *file, int offset){
    
    long line = 0;
    long currentOffset = 0; // Current offset while reading

    fseek(file, 0, SEEK_SET); // Move to the beginning of the file

    // Read the file character by character
    int ch;
    while ((ch = fgetc(file)) != EOF && currentOffset < offset) {
        currentOffset++;

        if (ch == '\n') {
            line++; // Increment the line number when a newline character is encountered
        }
    }
    return line;

}

int calculateLenght(FILE *fp){
    int fileSize;
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp); //Se extrae el tamaño del archivo
    fseek(fp, 0L, SEEK_SET);
    return fileSize;
};

int main (int argc, char *argv[]) {

    if (argc != 3) {
        printf("El formato correcto para utilizar el programa es: grep ¨'pattern1|pattern2|pattern3|...' archivo¨ \n");
        return 1;
    }

    clock_t start_time, end_time; //tiempo
    double elapsed_time;
    char *regexEx;
    regex_t regex;
    int i, regComp, last = 0, contador = 0;
    pid_t pid; //Variable para guardar el id de los procesos
    char *filename = argv[2]; //Se guarda el nombre del archivo
    long fileSize; //Variable para guardar el tamaño del archivo
    
    int lista[numProcesos]; //Se crea la cola 

    key_t msqkey = 999; //Key de los mensajes
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); //Id para pasar los mensajes

    // Open the file
    FILE *fp = fopen(filename, "rb"); //Se abre el archivo
    if (fp == NULL) {
        printf("Error opening file\n");
        exit(1);
    }
    fileSize = calculateLenght(fp);
    printf("fileSize %d: \n", fileSize);

    // Compile regex
    regexEx = argv[1]; //Regex Expression
    regComp = regcomp(&regex, regexEx, 0); 
    if (regComp) {
        printf("Error al compilar la expresión regular\n");
        exit(1);
    }
    
    for(i = 0; i < numProcesos; i++) {
      pid = fork();
      if (pid !=0)
        lista[i] = pid;
      
      if (pid == 0)
        break; //Si es hijo se sale
    }

    
    if (pid != 0) { // el padre
        msg.type = lista[contador];
        msg.newStartP = 0;
        msg.flag = 0;
        msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT);
        printf("Message sent\n");
        while (1) {
            msgrcv(msqid, &msg, sizeof(msg), 1, 0);
            if (msg.flag == 1) {
                if (msg.newStartP < fileSize) {
                    contador++;
                    if (contador == numProcesos)
                        contador = 0;
                    msg.type = lista[contador];
                    //printf("Siguiente hijo\n");
                    msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT);
                } else {
                    printf("There is no more lines to read\n");
                    last = 1;
                }
            } if (msg.flag == 2) {
                /*int arrayPos = msg.arrayPP;
                for(i = 0; i < arrayPos; i++) {
                    printf("%d \n", msg.coincidences[i]);
                    printLineMatched(fp,msg.coincidences[i]);
                } */
            } if (last == 1) {
                int exitCondition = 0;
                for (int j = 0; j < numProcesos; j++) {
                    if (msg.activos[j] == 0) {
                        exitCondition++;
                    }
                }
                if (exitCondition == numProcesos-1) 
                    break;
            }
        }
    } else { // el hijo
        while (1) {
            int newStart = 0, start = 0;
            long linesBefore = 0,  totalLines = 0;
            
            msgrcv(msqid, &msg, MSGSZ, getpid(), 0);
            
            printf("PID DEL HIJO %ld\n",(long)getpid());
            
            // Allocate memory for the buffer
            char *buffer = malloc(BUFFER_SIZE);

            start = msg.newStartP;
            printf("Start %ld\n",start);
            // With the start position calculate the lines before
            linesBefore = calculateLinesBeforeStart(fp, start); //lines
            printf("lineb: %d\n", linesBefore);
            
            
            fseek(fp, start, SEEK_SET);

            long bytesRead = fread(buffer, 1, BUFFER_SIZE, fp);
            size_t dataLenght = 0;
            
            if (bytesRead > 0) {
                // Search lastNewLine from buffer
                char *lastNewLine = strrchr(buffer, '\n');
                //printf("bytesREad %d\n",bytesRead);
                if (lastNewLine != NULL) {
                    
                    dataLenght = lastNewLine - buffer + 1;
                    buffer[dataLenght + 1] = '\0';
                    newStart = start + dataLenght + 1; //Bytes

                    //Send messagge, end of read
                    printf("Donde debe leer bytes el proximo proceso %d: \n", newStart);
                    msg.newStartP = newStart;

                } else {
                    printf("No se encontró un salto de línea en los primeros %d bytes.\n", BUFFER_SIZE);
                }

            } else {
                printf("No se leyeron datos del archivo.\n");
            }
            
            totalLines = calculateLinesBeforeStart(fp, newStart); // Total lines 
            // Adjust position to the start, send by the father
            
            msg.type = 1;
            msg.flag = 1;
            msg.activos[i] = 0;
            msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT); //Se vuelve a mandar la lista al padre
            
            //procesar con regex
             
            int regexLine = 1;
            char *token = strtok(buffer,"\n");
            int totalregex = 0;
            while ( token  != NULL) { 
                if (regexec(&regex, token, 0, NULL, 0) == 0) {
                    
                    // If expression match the line
                    printf("Coincidencia en la línea del buffer %d \n", regexLine);
                    
                    // Add the line regex coincidences from the buffer
                    totalregex = linesBefore + regexLine;
                    printf("Coincidencia en la línea %d \n",(totalregex));
                    printf("Token: %s \n", token);
                    
                }
                token = strtok(NULL,"\n");
                regexLine++;
                
                if(totalregex > totalLines){
                    break;
                }
            }

            //printf("Termine de procesar\n");
            msg.flag = 2;
            msg.activos[i] = 1;
            msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT); //Se vuelve a mandar la lista al padre
            free(buffer);
        }
    }
    
    msgctl(msqkey, IPC_RMID, NULL);
    fclose(fp);
    regfree(&regex);
    end_time = clock();
    elapsed_time = (double) (end_time - start_time) / CLOCKS_PER_SEC;
    printf("Elapsed time: %f seconds\n", elapsed_time);
    return 0;
}