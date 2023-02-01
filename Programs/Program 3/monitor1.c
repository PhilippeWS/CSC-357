#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>

#define MAXLENGTH 1000

int inputValidate(char input[], char** terminalPath);
void printTerminalPath(char* path);
void prompt();
void getCurrentPath();
void printStat(struct stat sb);
void voided(){};

int main(){
    char *terminalPath = (char *)mmap(NULL, MAXLENGTH * sizeof(unsigned char), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    time_t *start = (time_t *)mmap(NULL, sizeof(time_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    time_t *end = (time_t *)mmap(NULL, sizeof(time_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

    signal(SIGINT, voided);
    signal(SIGQUIT, voided);
    signal(SIGSTOP, voided);
    signal(SIGTERM, voided);
    signal(SIGHUP, voided);
    signal(SIGTSTP, voided);

    int pID = fork();
    if(pID == 0){
        getCurrentPath(&terminalPath);
        char inputS[MAXLENGTH];
        DIR *dir;
        struct dirent *entry;
        struct stat sb;
        int statVal, input;

        memset(inputS, 0, strlen(inputS));
        while (input != -1){
            //Get and clean user input.
            time(start);
            prompt(terminalPath);
            fflush(stdout);
            fflush(stdin);
            fgets(inputS, MAXLENGTH, stdin);
            inputS[strcspn(inputS, "\n")] = 0;
            input = inputValidate(inputS, &terminalPath);

            if(input == 2){
                dir = opendir(".");
                while ((entry = readdir(dir)) != NULL){
                    fprintf(stderr, "%s\n", entry->d_name);
                }
                closedir(dir);
            }else if(input == 1){
                statVal = stat(inputS, &sb);
                if (statVal != 0){
                    printTerminalPath(terminalPath);
                    printf("No file with specified file name found.\n");
                }else{
                    printStat(sb);
                }
            }else if(input == -1){
                *start = 1000;
            }
        }
    }else if(pID > 0){
        while(1){
            //The line below is techinically unecessary, but since you asked for it to check on a period I left it in.
                //This makes quitting much slower if it is at ten seconds.
                //Its much easier to have it instead continuously check every 1 second or not sleep at all.
            //sleep(10);
            sleep(1);
            time(end);
            if(difftime(*end, *start) >= 10.0){
                kill(pID,SIGKILL);
                wait(0);
                break;
            }
        }
        munmap(terminalPath, MAXLENGTH * sizeof(unsigned char));
        munmap(start, sizeof(time_t));
        munmap(end, sizeof(time_t));
    }
}

int inputValidate(char input[], char** terminalPath){
    if(strcmp(input ,"list") == 0)
        return 2;
    else if(strcmp(input ,"q") == 0)
        return -1;
    else if(strcmp(input, "..") == 0 || input[0] == '/'){
        //Clean up input for chdir if changing path
        if(input[0] == '/'){
            for(int i = 0; i < strlen(input); i++){
                input[i] = input[i+1];
            }
        }
        if(chdir(input) == 0){
            getCurrentPath(terminalPath);
        }else{
            printTerminalPath(*terminalPath);
            fprintf(stderr,"Invalid Directory, chdir(%s) failed. \n", input);
        }
        fflush(0);
        return 0;
    }
    else
        return 1;
}

void printTerminalPath(char *path){
    fprintf(stderr,"\033[0;36m");
    fprintf(stderr,"monitor1 program: %s", path);
    fprintf(stderr,"\033[0m");
    fprintf(stderr,"$");
}

void prompt(char *path){
    printTerminalPath(path);
    printf("Enter a command: \n");
    printTerminalPath(path);
}

void getCurrentPath(char **terminalPath){
    char buff[MAXLENGTH];
    getcwd(buff, MAXLENGTH);
    strcpy(*terminalPath, buff);
}

void printStat(struct stat sb){
    printf("File type:                ");
    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:  printf("block device\n");            break;
    case S_IFCHR:  printf("character device\n");        break;
    case S_IFDIR:  printf("directory\n");               break;
    case S_IFIFO:  printf("FIFO/pipe\n");               break;
    case S_IFLNK:  printf("symlink\n");                 break;
    case S_IFREG:  printf("regular file\n");            break;
    case S_IFSOCK: printf("socket\n");                  break;
    default:       printf("unknown?\n");                break;
    }
    printf("I-node number:            %ju\n", (long)sb.st_ino);
    printf("Mode:                     %jo (octal)\n", (unsigned long)sb.st_mode);
    printf("Link count:               %ju\n", (long)sb.st_nlink);
    printf("Ownership:                UID=%ju   GID=%ju\n", (long)sb.st_uid, (long)sb.st_gid);
    printf("Preferred I/O block size: %jd bytes\n", (long)sb.st_blksize);
    printf("File size:                %lld bytes\n", (long long)sb.st_size);
    printf("Blocks allocated:         %lld\n", (long long)sb.st_blocks);
    printf("Last status change:       %s", ctime(&sb.st_ctime));
    printf("Last file access:         %s", ctime(&sb.st_atime));
    printf("Last file modification:   %s", ctime(&sb.st_mtime));
}
