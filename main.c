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

#define BUFFER_SIZE 8192 // Buffer size
#define numProcess 5 // Process number
#define MAX_TOKENS 100
#define MSGSZ 128

struct message {
  long type;
  int flag; // 1 when process finished the reading, 2 when the process finished to process the buffer 

  int newStartP; // Variable to save the start of the next process
  int activos[numProcess]; // Variable to know what process are working
  int numTokens; // Number of tokens from array
  char tokens[50][150]; // Matrix wherematched lines are saved
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

int calculateLenght(FILE *fp){ // Extract the file size
    int fileSize;
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp); 
    fseek(fp, 0L, SEEK_SET);
    return fileSize;
};

int main (int argc, char *argv[]) {

    // Validation of the format
    if (argc != 3) {
        printf("El formato correcto para utilizar el programa es: grep ¨'pattern1|pattern2|pattern3|...' archivo¨ \n");
        return 1;
    }

    clock_t start_time, end_time; // Time
    double elapsed_time;
    char *regexEx;
    regex_t regex;
    int i, regComp, last = 0, contador = 0;
    pid_t pid; // Variable to save pid from process
    char *filename = argv[2]; // Variable that saves the filename
    long fileSize; // Variable to save file Size
    
    int lista[numProcess]; // Create the queue

    key_t msqkey = 999; // Messagges key
    int msqid = msgget(msqkey, IPC_CREAT | S_IRUSR | S_IWUSR); // Id from messagges

    // Open the file
    FILE *fp = fopen(filename, "rb"); 
    if (fp == NULL) {
        printf("Error al abrir el archivo \n");
        exit(1);
    }
    fileSize = calculateLenght(fp); // Calculate file Size
    printf("fileSize %d: \n", fileSize);

    // Compile regex
    regexEx = argv[1]; //Regex Expression
    regComp = regcomp(&regex, regexEx, 0); 
    if (regComp) {
        printf("Error al compilar la expresión regular \n");
        exit(1);
    }
    
    for(i = 0; i < numProcess; i++) {
      pid = fork();
      if (pid !=0)
        lista[i] = pid;
      
      if (pid == 0)
        break; // If its son it breaks
    }

    
    if (pid != 0) { // Father
        msg.type = lista[contador]; //Initialize first messagge
        msg.newStartP = 0;
        msg.flag = 0;
        msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT);

        printf("Mensaje enviado \n");
        while (1) {
            msgrcv(msqid, &msg, sizeof(msg), 1, 0);

            if (msg.flag == 1) { //If end to read
                if (msg.newStartP < fileSize) {
                    contador++;
                    if (contador == numProcess)
                        contador = 0;
                    msg.type = lista[contador];

                    msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT);
                } else {
                    printf("No hay más lineas que leer \n");
                    last = 1;
                }

            } if (msg.flag == 2) { //If end to process
                int arrayPos = msg.numTokens;
                for(i = 0; i < arrayPos; i++) {
                    printf("%s \n", msg.tokens[i]);
                    printf("\n");
                } 

            } if (last == 1) { // Exit condition
                int exitCondition = 0;
                for (int j = 0; j < numProcess; j++) {
                    if (msg.activos[j] == 0) {
                        exitCondition++;
                    }
                }
                if (exitCondition == numProcess-1) 
                    break;
            }
        }
    } else { // Son
        while (1) {
            int newStart = 0, start = 0;
            long linesBefore = 0,  totalLines = 0;
            
            msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
            
            printf("PID del hijo: %ld\n",(long)getpid());
            
            // Allocate memory for the buffer
            char *buffer = malloc(BUFFER_SIZE);

            start = msg.newStartP; // The start position for reading
            printf("Bytes de start: %ld\n",start);
            
            linesBefore = calculateLinesBeforeStart(fp, start); //lines

            fseek(fp, start, SEEK_SET); // Pointer to star position

            long bytesRead = fread(buffer, 1, BUFFER_SIZE, fp); // Fill the buffer
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

            msg.type = 1;
            msg.flag = 1;
            msg.activos[i] = 0;
            msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT); //Se vuelve a mandar la lista al padre
            
            //process buffer with regex
            int contador2 = 0;
            int regexLine = 1;
            char *token = strtok(buffer,"\n");
            int totalregex = 0;
            while ( token  != NULL) { 
                if (regexec(&regex, token, 0, NULL, 0) == 0) {
                    
                    // If expression match the line
                    strcpy(msg.tokens[contador2],token);
                    contador2++;

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
            msg.numTokens = contador2;

            msgsnd(msqid, (void *)&msg, sizeof(msg), IPC_NOWAIT); //Se vuelve a mandar la lista al padre
            free(buffer);
        }
    }
    
    msgctl(msqkey, IPC_RMID, NULL);
    fclose(fp);
    regfree(&regex);

    end_time = clock();
    elapsed_time = (double) (end_time - start_time) / CLOCKS_PER_SEC;

    printf("Tiempo transcurrido: %f segundos \n", elapsed_time);
    return 0;
}