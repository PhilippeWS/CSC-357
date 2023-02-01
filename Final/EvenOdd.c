#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

int *pids;
int *changeFlag;

int* setArrayParameters(int argc, char *argv[], int *arrayLength);
void printArray(int arrayLength,int *array);
void printResult(int processCount, int arrayLength, int *unsortedArray, int *sortedArray, double timeTaken);
void synch(int par_id, int par_count, int *ready);
void createChildInstance(int processId, int processCount, int arrayLength, int *array,  int *ready);
void oddEvenSort(int start, int numPairs, int arrayLength, int *array);
void compareExchange(int start, int numPairs, int arrayLength, int *array);

int main(int argc, char *argv[]){
    int numberOfProcess = atoi(argv[1]);
    
    pids = (int *)mmap(NULL, sizeof(int)*numberOfProcess, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    changeFlag = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    *changeFlag = FALSE;
    int *ready = (int *)mmap(NULL, sizeof(int)*2, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    ready[0] = ready[1] = 0;
    int arrayLength = 0;
    //Hold the unsorted array for comparison
    int *unsortedArray = setArrayParameters(argc, argv, &arrayLength);

    if(arrayLength == 0){
        fprintf(stderr,"Nothing to sort.\n");
        return 0;
    }else if(arrayLength == 1){
        printResult(0, arrayLength, unsortedArray, unsortedArray, 0.0);
        return 0;
    }  

    if(arrayLength/2 < numberOfProcess)
        numberOfProcess = arrayLength/2;

    //Create the array to sort
    int *sortedArray = (int*)mmap(NULL, sizeof(int)*arrayLength, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0); 
    memcpy(sortedArray, unsortedArray, sizeof(int)*arrayLength);

    clock_t start, end;
    start = clock();
    for(int index = 0; index < numberOfProcess; index++)
        createChildInstance(index, numberOfProcess, arrayLength, sortedArray, ready);
    

    int status;
    for(int index = 0; index < numberOfProcess; index++)
        waitpid(pids[index], &status, 0);
    end = clock();
    double timeTaken = (double)(end-start)/1000000;

    printResult(numberOfProcess,arrayLength, unsortedArray, sortedArray, timeTaken);

    munmap(pids, sizeof(int)*numberOfProcess);
    munmap(changeFlag, sizeof(int));
    munmap(ready, sizeof(int)*2);
    munmap(unsortedArray, sizeof(int)*arrayLength);
    munmap(sortedArray, sizeof(int)*arrayLength);
}

int* setArrayParameters(int argc, char *argv[], int *arrayLength){
    int *array;
    // For file input
    char *buffer = NULL;
    size_t bufferSize = 0;
    // Check if STDIN is empty
    if(fseek(stdin, 0, SEEK_END), ftell(stdin) > 0){
        rewind(stdin);
        int charRead = getline(&buffer, &bufferSize, stdin);
        // Check if the stdin redirect has any data
        if (charRead == 0){
            *arrayLength = 0;
            return NULL;
        }
    }
    else{
        fprintf(stderr,"Enter an array: ");
        // Check if the stdin redirect has any data
        int charRead = getline(&buffer, &bufferSize, stdin);
        if (charRead == 0){
            *arrayLength = 0;
            return NULL;
        }
    }

    buffer[strcspn(buffer, "\n")] = 0;
    char bfrCpy[bufferSize];
    strcpy(bfrCpy, buffer);

    // Use copy to find length
    char *token = strtok(bfrCpy, " ");
    *arrayLength += 1;

    while ((token = strtok(NULL, " ")) != NULL)
        *arrayLength += 1;

    // Allocate and input the array.
    array = (int *)mmap(NULL, sizeof(int) * (*arrayLength), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    token = strtok(buffer, " ");
    for (int index = 0; index < *arrayLength; index++){
        array[index] = atoi(token);
        token = strtok(NULL, " ");
    }
    fflush(stdin);
    free(buffer);
    return array;
}

void printArray(int arrayLength,int *array){
    fprintf(stderr, "[");
    for(int index = 0; index < arrayLength; index++){
        if(index == arrayLength -1){
            fprintf(stderr, "%d", array[index]);
            fprintf(stderr, "]\n");
        }else
            fprintf(stderr, "%d, ", array[index]);
    }    
}

void printResult( int processCount, int arrayLength, int *unsortedArray, int *sortedArray, double timeTaken){
    fprintf(stderr, "Unsorted Array: ");
    printArray(arrayLength, unsortedArray);
    fprintf(stderr, "Sorted Array:   ");
    printArray(arrayLength, sortedArray);
    fprintf(stderr, "Process Used: %d\n", processCount);
    fprintf(stderr, "Time Elapsed: %f\n", timeTaken);
}

void synch(int par_id, int par_count, int *ready){
    ++ready[0];
    if(ready[0] == par_count){
        ready[1]=0;
        ++ready[0];
    }
    while(ready[0] < par_count+1);
    ++ready[1];

    if (ready[1] == par_count){
        ready[0] = 0;
        ++ready[1];
    }
    while(ready[1] < par_count+1);
}

void createChildInstance(int processId, int processCount, int arrayLength, int *array, int *ready){
    if(fork() == 0){
        pids[processId] = getpid();
        int stage, startingIndex, pairsToDo;
        stage = startingIndex = pairsToDo = 0;
        int minTasks = (arrayLength/2)/processCount;
        int remainder = (arrayLength/2)%processCount;
        //Use this line to print information about who and what is handling tasks. Same with the ones int he conditionals.
        //fprintf(stderr, "Number of pairs: %d, minTasks: %d, remainder %d\n", arrayLength/2, minTasks, remainder);
        do{
            synch(processId, processCount, ready);
            *changeFlag = FALSE;
            synch(processId, processCount, ready);
            stage = stage%2;
            if(processId >= remainder){
                startingIndex = (remainder + processId*minTasks)*2 + stage;
                pairsToDo = minTasks;
                //fprintf(stderr, "Process %d Start: %d PairsToDo: %d\n", processId, startingIndex, pairsToDo);
                oddEvenSort(startingIndex + 1 >= arrayLength ? -1 : startingIndex, pairsToDo, arrayLength, array);
            }else{
                startingIndex = (processId*(minTasks+1))*2 + stage;
                pairsToDo = minTasks+1;
                //fprintf(stderr, "Process %d Start: %d PairsToDo: %d\n", processId, startingIndex, pairsToDo);
                oddEvenSort(startingIndex + 1 >= arrayLength ? -1 : startingIndex, pairsToDo, arrayLength, array);
            }
            stage++;
            synch(processId, processCount, ready);
        }while(*changeFlag);
        exit(0);
    }else
        return;
}

void oddEvenSort(int start, int numPairs, int arrayLength, int *array){
    if(start == -1)
        return;
    compareExchange(start, numPairs, arrayLength, array);
}

void compareExchange(int start, int numPairs,int arrayLength,int *array){
    for (int index = start; index < start + (2 * numPairs); index += 2){
        //fprintf(stderr, "Comparing index: %d-%d, Values: %d = %d \n", index, index + 1, array[index], array[index + 1]);
        if (index+1 < arrayLength && array[index] > array[index + 1]){
            int temp = array[index];
            array[index] = array[index + 1];
            array[index + 1] = temp;
            *changeFlag = TRUE;
        }
    }
}

