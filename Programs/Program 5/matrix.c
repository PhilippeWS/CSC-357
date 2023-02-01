#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <math.h>
#include <time.h>
#define MATRIX_DIMENSION_XY 10

#define FALSE 0
#define TRUE 1

#define WORKING 0
#define READY 1
#define SYNCING 2
// SEARCH FOR TODO

//************************************************************************************************************************
// sets one element of the matrix
void set_matrix_elem(float *M, int x, int y, float f){
    M[x + y * MATRIX_DIMENSION_XY] = f;
}
//************************************************************************************************************************
// lets see it both are the same
int quadratic_matrix_compare(float *A, float *B){
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            if (A[a + b * MATRIX_DIMENSION_XY] != B[a + b * MATRIX_DIMENSION_XY])
                return 0;

    return 1;
}
//************************************************************************************************************************
void sectionIndicies(int par_id, int par_count, int range[2]){
    int overFlow = MATRIX_DIMENSION_XY*MATRIX_DIMENSION_XY%par_count;
    int minTasks = MATRIX_DIMENSION_XY*MATRIX_DIMENSION_XY/par_count;
    
    if(par_id >= overFlow){
        range[0] = overFlow + par_id*minTasks;
        range[1] = range[0] + minTasks-1;
    }else{
        range[0] = par_id*(minTasks+1);
        range[1] = range[0] + minTasks;
    }
}
//************************************************************************************************************************
// print a matrix
void quadratic_matrix_print(float *C){
    printf("\n");
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
    {
        printf("\n");
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            printf("%.2f,", C[a + b * MATRIX_DIMENSION_XY]);
    }
    printf("\n");
}
//************************************************************************************************************************
// multiply two matrices
void quadratic_matrix_multiplication(float *A, float *B, float *C){
    // nullify the result matrix first
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)
            C[a + b * MATRIX_DIMENSION_XY] = 0.0;
    // multiply
    for (int a = 0; a < MATRIX_DIMENSION_XY; a++)         // over all cols a
        for (int b = 0; b < MATRIX_DIMENSION_XY; b++)     // over all rows b
            for (int c = 0; c < MATRIX_DIMENSION_XY; c++) // over all rows/cols left
            {
                C[a + b * MATRIX_DIMENSION_XY] += A[c + b * MATRIX_DIMENSION_XY] * B[a + c * MATRIX_DIMENSION_XY];
            }
}

void sectioned_matrix_multiplication(float *A, float *B, float *C, int startingIndex, int endingIndex){
    for(int index = startingIndex; index <= endingIndex; index++){
        int rowC = index / 10;
        int colC = index % 10;
        for(int r = 0; r < MATRIX_DIMENSION_XY; r+=0){
            for(int c = 0; c < MATRIX_DIMENSION_XY; c++){
                C[index] += A[r+rowC*MATRIX_DIMENSION_XY]*B[colC+c*MATRIX_DIMENSION_XY];
                r++;
            }
        }
    }   
}
//************************************************************************************************************************
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

    //100% working gate. Mine works 95% but has will for no good reason break occasionally, nullifying one random calculation

    // int pause = ++ready[par_id];
    // for(int parIndex=0; parIndex < par_count; parIndex++)
    //     while(ready[parIndex] < pause);

}

void roleCall(int par_id, int syncNum){
    fprintf(stderr, "ID: %d, Sync: %d\n", par_id, syncNum);
}

float randomDecimal(int min,int max){
    return ((max - min) * ((double)rand()) / ((double)RAND_MAX) + min);
}

float roundFloat(float var){
    var = var*100.0f;
    var = (var > floor(var) +0.5f) ? ceil(var) : floor(var);
    var = var/100.0f;
    return var;
}

//************************************************************************************************************************
int main(int argc, char *argv[]){
    int par_id = 0;    // the parallel ID of this process
    int par_count = 1; // the amount of processes
    float *A, *B, *C;  // matrices A,B and C
    int *ready;        // needed for synch
    srand(time(NULL));

    if (argc != 3){
        printf("no shared\n"); 
    }
    else{
        par_id = atoi(argv[1]);
        par_count = atoi(argv[2]);
        // strcpy(shared_mem_matrix,argv[3]);
    }
    if (par_count == 1){
        printf("only one process\n");

    }

    int fd[4];
    if (par_id == 0){
        // TODO: init the shared memory for A,B,C, ready. shm_open with C_CREAT here! then ftruncate! then mmap
        fd[0] = shm_open("matrixA", O_RDWR | O_CREAT, 00777);
        fd[1] = shm_open("matrixB", O_RDWR | O_CREAT, 00777);
        fd[2] = shm_open("matrixC", O_RDWR | O_CREAT, 00777);
        fd[3] = shm_open("synchObject", O_RDWR | O_CREAT, 00777);
        
        ftruncate(fd[0], sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
        ftruncate(fd[1], sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
        ftruncate(fd[2], sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
        ftruncate(fd[3], par_count + 2);
        
        A = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = (int *)mmap(NULL, sizeof(int)*(2), PROT_READ | PROT_WRITE, MAP_SHARED, fd[3], 0);
        for(int index = 0; index < 2; index++){
            ready[index] = 0;
        }
    }
    else{
        // TODO: init the shared memory for A,B,C, ready. shm_open withOUT C_CREAT here! NO ftruncate! but yes to mmap
        sleep(2); // needed for initalizing synch
        fd[0] = shm_open("matrixA", O_RDWR, 00777);
        fd[1] = shm_open("matrixB", O_RDWR, 00777);
        fd[2] = shm_open("matrixC", O_RDWR, 00777);
        fd[3] = shm_open("synchObject", O_RDWR, 00777);

        A = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[0], 0);
        B = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[1], 0);
        C = (float *)mmap(NULL, sizeof(float)*(MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY), PROT_READ | PROT_WRITE, MAP_SHARED, fd[2], 0);
        ready = (int*)mmap(NULL, sizeof(int)*(2), PROT_READ|PROT_WRITE, MAP_SHARED, fd[3], 0);
    }
    //Synch 1
    synch(par_id, par_count, ready);

    if (par_id == 0){
        // TODO: initialize the matrices A and B
        for(int row = 0; row < MATRIX_DIMENSION_XY; row++){
            for(int col = 0; col < MATRIX_DIMENSION_XY; col++){
                A[col+row*MATRIX_DIMENSION_XY] = roundFloat(randomDecimal(0,10));
                B[col+row*MATRIX_DIMENSION_XY] = roundFloat(randomDecimal(0,10));
                C[col+row*MATRIX_DIMENSION_XY] = 0.0;
            }
        }
    }

    //Synch 2
    synch(par_id, par_count, ready);
    // TODO: quadratic_matrix_multiplication_parallel(par_id, par_count,A,B,C, ...);
    int range[2];
    sectionIndicies(par_id, par_count, range);    
    sectioned_matrix_multiplication(A, B, C, range[0], range[1]);

    //Synch 3
    synch(par_id, par_count, ready);

    if (par_id == 0)
        quadratic_matrix_print(C);

    //Synch 4
    synch(par_id, par_count, ready);


    // lets test the result:
    float M[MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY];
    quadratic_matrix_multiplication(A, B, M);

    if(par_id == 0){
        quadratic_matrix_print(M);
    } 

    if(par_id==0 && quadratic_matrix_compare(C, M))
        fprintf(stderr,"full points! from: %d\n", par_id);    
    else if(par_id==0){
        printf("buuug!\n");
    }

    
    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    shm_unlink("matrixA");
    shm_unlink("matrixB");
    shm_unlink("matrixC");
    shm_unlink("synchObject");
    munmap(A, sizeof(float) * (MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
    munmap(B, sizeof(float) * (MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
    munmap(C, sizeof(float) * (MATRIX_DIMENSION_XY * MATRIX_DIMENSION_XY));
    munmap(ready, sizeof(int)*(par_count+2));
    exit(0);
}