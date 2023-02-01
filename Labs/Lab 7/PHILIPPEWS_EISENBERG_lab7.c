#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>

#define FALSE 0
#define TRUE 1

#define PROCESS1 0
#define PROCESS2 1

#define IDLE 0
#define WAITING 1
#define ACTIVE 2

#define LENGTH 1000

int main(){
    char *quote1 = "Oh, the misery-Everybody wants to be my enemy-Spare the sympathy-Everybody wants to be my enemy-My enemy-But I'm ready\n";
    char *quote2 = "My bad habits lead to late nights endin' alone-Conversations with a stranger I barely know-Swearin' this will be the last, but it probably won't I got nothin' left to lose, or use, or do\n";
    char *currentQuote = (char *)mmap(NULL, LENGTH, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *flags = (int *)mmap(NULL, 2*sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    int *turn = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    flags[PROCESS1] = flags[PROCESS2] = IDLE;
    *turn = PROCESS1;
    int totalProcesses = 2;

    int pID = fork();
    if(pID == 0){
        int quoteToCopy = 1;
        while (1){
            int index;
            do{
                flags[PROCESS1] = WAITING;
                index = *turn;
                while (index != PROCESS1){
                    if (flags[index] != IDLE)
                        index = *turn;
                    else
                        index = (index + 1) % totalProcesses;
                }

                flags[PROCESS1] = ACTIVE;

                index = 0;
                while (index < totalProcesses && (index == PROCESS1 || flags[index] != ACTIVE)){
                    index++;
                }
            }while(!(index >= totalProcesses && (*turn == PROCESS1 || flags[*turn] == IDLE)));
            *turn = PROCESS1;
            //Crit Section Start
            if (quoteToCopy == 1){
                strcpy(currentQuote, quote1);
                quoteToCopy = 2;
            }else{
                strcpy(currentQuote, quote2);
                quoteToCopy = 1;
            }
            //Crit Section End

            index = (*turn + 1) % totalProcesses;
            while (flags[index] == IDLE){
                index = (index + 1) % totalProcesses;
            }
            *turn = index;
            flags[PROCESS1] = IDLE;
        }
    }else if(pID > 0){
        while (1){
            int index;
            do{
                flags[PROCESS2] = WAITING;
                index = *turn;
                while (index != PROCESS2){
                    if (flags[index] != IDLE)
                        index = *turn;
                    else
                        index = (index + 1) % totalProcesses;
                }

                flags[PROCESS2] = ACTIVE;

                index = 0;
                while (index < totalProcesses && (index == PROCESS2 || flags[index] != ACTIVE)){
                    index++;
                }
            }while(!(index >= totalProcesses && (*turn == PROCESS2 || flags[*turn] == IDLE)));

            *turn = PROCESS2;
            //Crit Section Start
            char toPrint[LENGTH];
            strcpy(toPrint, currentQuote);
            fprintf(stderr,"%s", toPrint);
            //Crit Section End

            index = (*turn + 1)%totalProcesses;
            while (flags[index] == IDLE){
                index = (index + 1) % totalProcesses;
            }
            *turn = index;
            flags[PROCESS2] = IDLE;
        }
        wait(0);
    }

    munmap(currentQuote, 1000);
    return 0;
}