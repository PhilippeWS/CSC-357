#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <sys/mman.h>


void printCountDown();
struct timeval getTimeStamp();
void validateTime(struct timeval start, struct timeval end);
void callProgramInstance(int argc,char *argv[], int argCount,char **newExecArgs);

#define INTERVAL 500
double intervalsLeft = 0;

int main(int argc, char *argv[]){
    signal(SIGALRM, printCountDown);
    struct itimerval itVal;
    itVal.it_value.tv_sec = INTERVAL/1000;
    itVal.it_value.tv_usec = (INTERVAL*1000)%1000000;
    itVal.it_interval = itVal.it_value;
    intervalsLeft = atof(argv[1]) * 2;

    char *parameters[argc-3 > 0 ? argc-1 : 1];

    //These comments were used to test the intervals and timing, feel free to use them to test as well.
    // struct timeval start, end;
    for(int counter = intervalsLeft; intervalsLeft > 0; intervalsLeft--){
        // start = getTimeStamp();
        setitimer(ITIMER_REAL, &itVal, NULL);
        pause();
        // end = getTimeStamp();
        // validateTime(start, end);
    }

    callProgramInstance(argc, argv, argc-3 > 0 ? argc-2 : 1, parameters);
    return 0;
}

void printCountDown(){
    for(int i = 0; i<intervalsLeft; i++){
        fprintf(stderr,".");
    }
    fprintf(stderr,"\n");
    fflush(stdin);
}

struct timeval getTimeStamp(){
    struct timeval timeStamp;
    gettimeofday(&timeStamp, NULL);
    return timeStamp;
}

void validateTime(struct timeval start, struct timeval end){
    long secTaken = (end.tv_sec-start.tv_sec);
    long microTaken = (end.tv_usec-start.tv_usec);
    double total = secTaken + microTaken*1e-6;
    fprintf(stderr,"Time Take: %f \n", total);
}

void callProgramInstance(int argc, char *argv[], int argCount,char **newExecArgs){
    if(fork() == 0){
        if (argCount > 0){
            for (int index = 0; index < argCount; index++){
                newExecArgs[index] = (char *)mmap(NULL, 255, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);;
                strcpy(newExecArgs[index], argv[index + 2]);
            }
            newExecArgs[argCount] = NULL;
        }
        else
            newExecArgs[0] = NULL;

        execv(argv[2], newExecArgs);
    }
}
