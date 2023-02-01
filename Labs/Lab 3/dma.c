#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PAGESIZE 1024
#define HEAPSIZE 1048576
unsigned char myheap[HEAPSIZE];
int chunksAllocated = 0;

typedef struct chunkhead{
    unsigned int size;
    unsigned int info;
    unsigned char *next, *prev;
} chunkhead; //24 bytes

struct chunkhead *head;

unsigned char *mymalloc(unsigned int size);
void myfree(unsigned char *address);
void analyse();
void initializeHeap();

int main(){
    initializeHeap();
    unsigned char *a,*b,*c;
    a = mymalloc(2000);
    b = mymalloc(1000);
    c = mymalloc(2000);
    printf("Iteration 1------------------------------\n");
    analyse();
    myfree(b);
    printf("Iteration 2------------------------------\n");
    myfree(a);
    myfree(c);
    analyse();
    return 0;
}

unsigned char *mymalloc(unsigned int size){
    chunkhead *currentChunk = head;
    unsigned char *cCP = &myheap[0];
    size = sizeof(chunkhead) + size;
    int pagesToAllocate = ceil(1.0*size/PAGESIZE);
    chunkhead *newChunk;

    while(currentChunk != NULL){
        if(currentChunk->info != 1){
            if(currentChunk->size >= size){
                newChunk = (chunkhead *)&myheap[PAGESIZE*chunksAllocated];
                newChunk->size = pagesToAllocate*PAGESIZE - sizeof(chunkhead);
                newChunk->info = 1;
                newChunk->prev = currentChunk->prev;
                newChunk->next = currentChunk->next;
                //cCP = &myheap[PAGESIZE*chunksAllocated + 24];
                if (currentChunk->next == NULL)
                {
                    chunksAllocated += pagesToAllocate;
                    chunkhead *heapChunk = (chunkhead *)&myheap[PAGESIZE*chunksAllocated];
                    heapChunk->size = HEAPSIZE - chunksAllocated*PAGESIZE - sizeof(chunkhead);
                    heapChunk->info = 0;
                    heapChunk->next = NULL;
                    heapChunk->prev = (unsigned char *)newChunk;
                    newChunk->next = (unsigned char *)heapChunk;
                    break;
                }else{
                    chunksAllocated += pagesToAllocate;
                    break;
                }
            }   
        }
        cCP = currentChunk->next;
        currentChunk = (chunkhead *)cCP;
    }
    return cCP+sizeof(chunkhead);
}

void myfree(unsigned char *address){
    chunkhead *chunkToBeFreed = (chunkhead *)(address - sizeof(chunkhead));
    
    chunkhead *chunkAfter = (chunkhead *)chunkToBeFreed->next;
    chunkhead *chunkBefore = (chunkhead *)chunkToBeFreed->prev;
    unsigned char *cCP = &myheap[0];
    address = address - sizeof(chunkhead);

    chunkToBeFreed->info = 0;
    
    if(chunkAfter != NULL && chunkAfter->info == 0){
        chunkToBeFreed->next = chunkAfter->next;
        chunkToBeFreed->size = chunkToBeFreed->size+chunkAfter->size;
        chunkAfter->prev = chunkAfter->next = NULL;
    }

    if(chunkBefore != NULL && chunkBefore->info == 0){
        chunkBefore->next = chunkToBeFreed->next;
        chunkBefore->size = chunkBefore->size+chunkToBeFreed->size;
        chunkToBeFreed->prev = chunkToBeFreed->next = NULL;
    }
}

void analyse(){
    chunkhead *currentChunk = head;
    int chunkID = 1;
    unsigned char *cCP = &myheap[0];
    while(currentChunk != NULL){
        printf("Chunk #%d\n", chunkID);
        printf("%d\n", currentChunk->size);
        if(currentChunk->info == 1){
            printf("Occupied\n");
        }else{
            printf("Free\n");
        }
        printf("Next = %p\n", currentChunk->next);
        printf("Prev = %p\n\n", currentChunk->prev);
        //currentChunk = currentChunk->next;
        chunkID++;
        cCP = currentChunk->next;
        currentChunk = (chunkhead *)cCP;
    }
}



void initializeHeap(){
    head = (chunkhead *)&myheap[0];
    head->size = HEAPSIZE - sizeof(chunkhead);
    head->info = 0;
    head->next = head->prev = NULL;
}