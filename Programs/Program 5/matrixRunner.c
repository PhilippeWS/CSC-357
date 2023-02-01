#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define GNU_SOURCE

void createMatrixInstance(char *argv[], char **childArgs, int par_id, int *pids){
    if(fork()==0){     
        pids[par_id] = getpid();  
        strcpy(childArgs[0], argv[1]);
        sprintf(childArgs[1], "%d", par_id);
        strcpy(childArgs[2], argv[2]);
        childArgs[3] = NULL;
        //fprintf(stderr, "par_id: %d, childArg1: %s, childArg2: %s, childArg3: %s\n", par_id,childArgs[0], childArgs[1], childArgs[2]);
        execv(argv[1],childArgs);
    }
}

int main(int argc, char *argv[]){
    int par_count = atoi(argv[2]);
    int pids[par_count];
    int status; 

    char *childArgs[4];
    for(int i =0; i< 4; i++)
        childArgs[i] = (char*)mmap(NULL, 100, 0x1 | 0x2, 0x20 | 0x01, -1, 0);

    clock_t start, end;
    start = clock();

    for(int par_id = 0; par_id < par_count; par_id++)
        createMatrixInstance(argv, childArgs, par_id, pids);

    for(int index = 0; index < par_count; index++)
        waitpid(pids[index], &status, 0);
    
    end = clock();
    double timeTaken = (double)(end-start)/1000000;
    fprintf(stderr,"Time Take: %f\n", timeTaken);
    munmap(childArgs, 4 * sizeof(100));
}