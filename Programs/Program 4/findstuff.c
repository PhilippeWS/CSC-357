#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <ftw.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
//This is supposedly supposed to define my flags for nftw, doesn't work, unsure how these flags are supposed to work. See Q2.
// #define _XOPEN_SOURCE 500 
// #ifdef _XOPEN_SOURCE
// #endif

//#define _GNU_SOURCE
#define RES_BUFFER_SIZE 5
#define STR_SIZE 150
#define MAX_LENGTH 1000
#define FALSE 0
#define TRUE 1

//Q1: Since you want us to search for the word in any file, How do we open non text files to search them?
    //FILE* fopen can open anyfile, even not text files.
        //Make sure to validate readability
//Q2: Trying to use asprintf to format my return string for output, but I dont understand the man pages. Using sprintf, which i heard was unsafe.
    //Q2.5: This also realtes to nftw, which I tried to use but the flags dont seem to be defined.
    //Q2.5.1: Dont fully understand all the parameters in ftw, need some help there as well.

typedef struct processInfo{
    int workingPID;
    int fileParse;
    int success;
    char *fileEnding; 
    char target[STR_SIZE];    
    char resultFile[RES_BUFFER_SIZE][STR_SIZE];
    int totalResults;
}processInfo;


int fd[2];
processInfo *processList[10];

//User Interface Related.
void prompt(char text[], int listItem);
char* formatErrorMessage(char errorTxt[]);
void removeChar(char *string, char targetChar);
int inputValidate(char input[], char **target, char **fileExt);

void switchSTDIN(int i){
    dup2(fd[0], STDIN_FILENO);
}

//Functionality Related
void waitOnAll();
void enterResult(int listId, const char* objectName);
int parseFileForString(const char *fileName, char* targetString);
int checkTarget(const char *objectName, const struct stat *objectProperties, int flag);
int parseFileForString(const char *fileName, char* targetString);
void createChildInstance(int fileParse,int subDirectoryParse, char **fileExtension, char **targetString);

int main(){
    char *target = (char *)mmap(NULL, STR_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    char *fileExt = (char *)mmap(NULL, STR_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    char userInput[MAX_LENGTH];
    int status, validate;
    int stdinFlag = dup(STDIN_FILENO);
    
    //Setup, catch signals, get prompt path, create pipe, flose writing end since uncessary.
    signal(SIGUSR1,switchSTDIN);
    pipe(fd);

    //Initialize the global array to blank values to ensure no nullity issues
    for(int index = 0; index < 10; index++){
        processList[index] = (processInfo *)mmap(NULL, sizeof(processInfo), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        processList[index]->workingPID = -1;
        processList[index]->fileParse = FALSE;
        processList[index]->success = FALSE;
        processList[index]->fileEnding = NULL;
        strcpy(processList[index]->target, "");
        for(int resI = 0; resI < RES_BUFFER_SIZE; resI++){
            strcpy(processList[index]->resultFile[resI], "");
        }
        processList[index]->totalResults = 0;
    }

    while(1){
        target = fileExt = "";
        prompt("Enter a query:", FALSE);
        dup2(stdinFlag, STDIN_FILENO);
        read(STDIN_FILENO, userInput, MAX_LENGTH);
        validate = inputValidate(userInput, &target, &fileExt);
        if(validate == 0){
            fprintf(stderr, "%s\n", userInput);
            prompt("",TRUE);
        }else if(validate == 1){
            prompt("Quitting...", TRUE);
            for (int index = 0; index < 10; index++){
                if (processList[index]->workingPID != -1){
                    kill(processList[index]->workingPID, SIGTERM);
                }
            }
            break;
        }else if(validate == 2){
            int entryMade = -1;
            for (int index = 0; index < 10; index++){
                if (processList[index]->workingPID != -1){
                    char processInfo[MAX_LENGTH];
                    entryMade = sprintf(processInfo, "Process %d: [PID: %d | FileParse: %s | Target: %s]", 
                                index+1, processList[index]->workingPID, (processList[index]->fileParse == TRUE) ? "True" : "False", 
                                processList[index]->target);
                    prompt(processInfo, TRUE);
                }
            }
            if (entryMade > 0){
                prompt("", TRUE);
            }else{
                prompt("No Active Processes.", TRUE);
            }
        }else if(validate == 4){
            createChildInstance(FALSE, FALSE, &fileExt, &target);
        }else if(validate == 5){
            createChildInstance(FALSE, TRUE, &fileExt, &target);
        }else if(validate == 6 || validate == 8){
            createChildInstance(TRUE, FALSE, &fileExt, &target);
        }else if(validate == 7 || validate == 9){
            createChildInstance(TRUE, TRUE, &fileExt, &target);
        }else if(validate > 10){
            kill(validate, SIGTERM);
            prompt("Process Killed", TRUE);
        }
        waitOnAll();
    }
    close(fd[1]);
    close(fd[0]);
    
    for(int index = 0; index < 10; index++){
        if(processList[index]->workingPID > 0){
            kill(processList[index]->workingPID, SIGTERM);
            waitpid(processList[index]->workingPID, NULL, 0);
        }
        munmap(processList[index], sizeof(processInfo));
    }
    munmap(target, STR_SIZE);
    munmap(fileExt, STR_SIZE);

    return 0;
}

void prompt(char text[], int listItem){
    fprintf(stderr,"\033[0;36mSearch Program: \033[0m$%s\n", text);
    if(listItem == FALSE){
        fprintf(stderr,"\033[0;36mSearch Program: \033[0m$");
    }
}

void commandError(char *errorMsg){
    char errorText[STR_SIZE];
    sprintf(errorText, "\033[1;31mERROR: \033[0m%s", errorMsg);
    prompt(errorText, TRUE);
} 

void removeChar(char *string, char targetChar){
    int strLength = strlen(string);
    for(int index1 = 0; index1 < strLength; index1++){
        if(string[index1] == targetChar){
            for(int index2 = index1; index2<strLength; index2++){
                string[index2] = string[index2+1];
            }
            strLength--;
            index1--;
        }
    }
}

int inputValidate(char input[], char **target, char **fileExt){
    char inputCpy[MAX_LENGTH];
    strcpy(inputCpy,input);
    char genericError[] = "Invalid Command.";
    inputCpy[strcspn(inputCpy, "\n")] = 0;
    char *token = strtok(inputCpy, " ");

    //Legend for Validations:
        //-1: Error, Invalid Command
        //0: Assume this is input from our child.
        //1: Quit command
        //2: List command
        //4: Query Type: File, Directory Parse: False, File Parse: False, File Ext: NULL
        //5: Query Type: File, Directory Parse: True, File Parse: False, FIle Ext: NULL
        //6: Query Type: Text, Directory Parse: False, File Parse: True, File Ext: NULL
        //7: Query Type: Text, Directory Parse: True, File Parse: True, File Ext: NULL
        //8: Query Type: Text, Directory Parse: False, File Parse: True, File Ext: True
        //9: Query Type: Text, Directory Parse: True, File Parse: True, File Ext: True
        //workingPID: Kill command


    if(token == NULL){
        commandError(genericError);
        return -1;
    }else if(strcmp("list", token) == 0){
        token = strtok(NULL, " ");
        if(token != NULL){
            commandError(genericError);
            return -1;
        }   
        waitOnAll();
        return 2;
    }else if(strcmp("quit", token) == 0 || strcmp("q", token) == 0){
        token = strtok(NULL, " ");
        if(token != NULL){
            commandError(genericError);
            return -1;
        }
        return 1;
    }else if(strcmp("kill", token) == 0){
        token = strtok(NULL, " ");
        char *furtherToken = strtok(NULL, " ");
        if(furtherToken != NULL){
            commandError(genericError);
            return -1;
        }else{
            char num = token[0];
            if(isdigit(num) != 0 && strlen(token) == 1){
                int targetProcessIndex = atoi(token);

                if(targetProcessIndex > 0 && targetProcessIndex < 11){
                    if (processList[targetProcessIndex - 1]->workingPID == -1){
                        commandError("Invalid process selection.");
                        return -1;
                    }
                    return processList[targetProcessIndex - 1]->workingPID;
                }
            }
            commandError(genericError);
            return -1;
        }
    }else if(strcmp("find", token) == 0){
        token = strtok(NULL, " ");
        if(token == NULL){
            commandError(genericError);
            return -1;
        }
        if(token[0] == '\"' && token[strlen(token)-1] == '\"'){
            removeChar(token, '\"');
            *target = token;
            token = strtok(NULL, " ");

            if(token == NULL){
                return 6;
            }else if(token[0] == '-' && token[1] == 'f' && token[2] == ':'){
                token += 3;
                *fileExt = token;
                token = strtok(NULL, " ");
                if(token == NULL){
                    return 8;
                }else if(strcmp("-s", token) == 0){
                    token = strtok(NULL, " ");
                    if(token != NULL){
                        commandError(genericError);
                        return -1;
                    }
                    return 9;
                }
            }else if(strcmp("-s", token) == 0){
                token = strtok(NULL, " ");
                if (token == NULL){
                    return 7;
                }
                if (token[0] == '-' && token[1] == 'f' && token[2] == ':'){
                    token += 3;
                    *fileExt = token;
                    token = strtok(NULL, " ");
                    if (token != NULL){
                        commandError(genericError);
                        return -1;
                    }
                    return 9;
                }else if(token == NULL){
                    return 7;
                }
                commandError(genericError);
                return -1;
            }
            commandError(genericError);
            return -1;
        }else{
            *target = token;
            token = strtok(NULL, " ");
            if(token == NULL){
                return 4;
            }else{
                if(strcmp("-s", token) == 0){
                    token = strtok(NULL, " ");
                    if (token != NULL){
                        commandError(genericError);
                        return -1;
                    }
                    return 5;
                }
                commandError("Invalid Flag.");
                return -1;
            }
        }
    }else if(strcmp("Process", token)==0){
        token = strtok(NULL, " ");
        token = strtok(NULL, " ");
        
        if(strcmp("Results", token) == 0 || strcmp("|", token)==0 || strcmp("Running", token) == 0 || strcmp("Finished", token) == 0)
            return 0;
        
        commandError(genericError);
        return -1;
    }else{
        commandError(genericError);
        return -1;
    }
}

void waitOnAll(){
    int status;
    for (int index = 0; index < 10; index++){
        if (processList[index]->workingPID > 0){
            if (waitpid(processList[index]->workingPID, &status, WNOHANG) != 0){
                processList[index]->workingPID = -1;
            }
        }
    }
}

// int myFgets(char *__restrict__ buffer, int size, FILE *__restrict__ fileStream){
// }

int parseFileForString(const char *fileName, char* targetString){
    //This needs to be super large to avoid stack smashing error, is there a way to fix that?
        //fgets: Reads until a new line, which may be the solution?
        //fscanf: Reads until a blank space, currently using. Cause of my issue.
        //fread: Reads a set number of bytes, may cut off a word.
        
    char line[MAX_LENGTH];
    FILE *fileToParse = fopen(fileName, "rb");
    int success = FALSE; 
    if (fileToParse == NULL)
        return success;
    
    // while (fscanf(fileToParse, "%s", line) == 1){ | fgets(line, MAX_LENGTH, fileToParse) != NULL
    while(fgets(line, MAX_LENGTH, fileToParse) != NULL){
        if (strstr(line, targetString) != NULL){
            success = TRUE;
            break;
        }
    }
    fclose(fileToParse);
    return success;
}

void enterResult(int listId, const char* objectName){
    int resultIndex = (processList[listId]->totalResults) % RES_BUFFER_SIZE;

    strcpy(processList[listId]->resultFile[resultIndex], objectName);
    processList[listId]->totalResults++;

    if (processList[listId]->totalResults % RES_BUFFER_SIZE == 0 && strcmp(processList[listId]->resultFile[0], "") != 0){
        char childResult[MAX_LENGTH];
        sprintf(childResult, "Process %d Running Results (Target: %s): \n%s\n%s\n%s\n%s\n%s",
                listId + 1, processList[listId]->target,
                processList[listId]->resultFile[0],
                processList[listId]->resultFile[1],
                processList[listId]->resultFile[2],
                processList[listId]->resultFile[3],
                processList[listId]->resultFile[4]);
        write(fd[1], childResult, MAX_LENGTH);
        kill(getppid(), SIGUSR1);

        for(int resI = 0; resI < RES_BUFFER_SIZE; resI++){
            strcpy(processList[listId]->resultFile[resI], "");
        }
    }
    processList[listId]->success = FALSE;
}

int checkTarget(const char *objectName, const struct stat *objectProperties, int flag){
    int currentPID = getpid();
    processInfo *currentProccessInfo;
    int listId = -1;
    for(int index = 0; index < 10; index++){
        if(processList[index]->workingPID == currentPID){
            listId = index;
            currentProccessInfo = processList[index];
            break;
        }
    }

    if(currentProccessInfo->fileParse == TRUE){
        if(flag == FTW_F && access(objectName, R_OK) == 0){
            if(strcmp(currentProccessInfo->fileEnding, "") != 0){
                char *currentFileExt = strrchr(objectName, '.');
                if(currentFileExt != NULL && strcmp(currentProccessInfo->fileEnding, currentFileExt+1) == 0){
                    if(parseFileForString(objectName,currentProccessInfo->target) == TRUE){
                        enterResult(listId, objectName);
                        return TRUE;
                    }
                }
                // return currentProccessInfo->success;
            }else{
                if(parseFileForString(objectName, currentProccessInfo->target) == TRUE){
                    enterResult(listId, objectName);
                }
            }
            
        }
    }else if(flag == FTW_F){
        char *fileNameWOPath = strrchr(objectName, '/');
        if(fileNameWOPath != NULL && strcmp(fileNameWOPath+1,currentProccessInfo->target) == 0){
            enterResult(listId, objectName);
        }
    }
    return FALSE;
}

void createChildInstance(int fileParse, int subDirectoryParse, char **fileExtension,  char **targetString){
    if(fork() == 0){
        clock_t start, end;
        double timeS = 0;
        int timeFormatted[4];
        close(fd[0]);
        int listId = -1;
        processInfo *currentProccessInfo = NULL;
        
        for (int index = 0; index < 10; index++){
            if(processList[index]->workingPID == -1){
                listId = index;
                currentProccessInfo = processList[index];
                break;
            }
        }

        start = clock();
        if(currentProccessInfo != NULL){
            currentProccessInfo->workingPID = getpid();
            currentProccessInfo->fileParse = fileParse;
            currentProccessInfo->fileEnding = *fileExtension;
            strcpy(currentProccessInfo->target, *targetString);

            if(subDirectoryParse == TRUE){
                ftw(".", checkTarget, 20);
            }else{
                DIR *dir;
                struct dirent *entry;
                dir = opendir(".");
                while ((entry = readdir(dir)) != NULL){
                    if(fileParse){
                        if (access(entry->d_name, R_OK) == 0 && entry->d_type == DT_REG){
                            if(parseFileForString(entry->d_name, *targetString) == TRUE){
                                enterResult(listId, entry->d_name);
                                break;
                            }
                        }
                    }
                    else{
                        if(strcmp(entry->d_name, currentProccessInfo->target) == 0){
                            enterResult(listId, entry->d_name);
                        }
                    }

                }
                closedir(dir);
            }
            end = clock();
            timeS = (double)(end-start)/CLOCKS_PER_SEC;
            timeFormatted[0] = (int)(timeS * 1000.0)%1000;
            timeFormatted[1] = (int)timeS%60;
            timeFormatted[2] = (int)(timeS/60)%60;
            timeFormatted[3] = (int)(timeS/3600)%3600;

            char childResult[MAX_LENGTH];
            if(currentProccessInfo->totalResults == 0){
                sprintf(childResult, "Process %d Results | (Target->%s): \n\033[0;31mNO RESULTS FOUND\033[0m\nElapsed Time: %02d:%02d:%02d:%03d", listId +1, currentProccessInfo->target, timeFormatted[3], timeFormatted[2], timeFormatted[1], timeFormatted[0]);
            }else if(currentProccessInfo->totalResults%RES_BUFFER_SIZE == 0){
                sprintf(childResult, "Process %d Finished | Elapsed Time: %02d:%02d:%02d:%03d", listId + 1, timeFormatted[3], timeFormatted[2], timeFormatted[1], timeFormatted[0]);
            }else{
                sprintf(childResult, "Process %d Finished Results | (Target->%s): \n%s%s%s%sElapsed Time: %02d:%02d:%02d:%03d",
                        listId + 1, currentProccessInfo->target,
                        (strcmp(currentProccessInfo->resultFile[0], "") != 0) ? strcpy(currentProccessInfo->resultFile[0], strcat(currentProccessInfo->resultFile[0], "\n")) : (""),
                        (strcmp(currentProccessInfo->resultFile[1], "") != 0) ? strcpy(currentProccessInfo->resultFile[1], strcat(currentProccessInfo->resultFile[1], "\n")) : (""),
                        (strcmp(currentProccessInfo->resultFile[2], "") != 0) ? strcpy(currentProccessInfo->resultFile[2], strcat(currentProccessInfo->resultFile[2], "\n")) : (""),
                        (strcmp(currentProccessInfo->resultFile[3], "") != 0) ? strcpy(currentProccessInfo->resultFile[3], strcat(currentProccessInfo->resultFile[3], "\n")) : (""),
                        timeFormatted[3], timeFormatted[2], timeFormatted[1], timeFormatted[0]);
            }
            write(fd[1], childResult, MAX_LENGTH);
            kill(getppid(), SIGUSR1);
            close(fd[1]);
            for(int resI = 0; resI < RES_BUFFER_SIZE; resI++){
                strcpy(currentProccessInfo->resultFile[resI], "");
            }
        }
        else{
            fprintf(stderr,"Maximum number of process reached.\n");
            prompt("Enter a query", FALSE);
        }
        exit(0);
    }else{
        return;
    }
}