#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#define PAGESIZE 4096
typedef unsigned char byte;

typedef struct chunkhead{
    unsigned int size;
    unsigned int info;
    unsigned char *next, *prev;
} chunkhead; //24 bytes

void *head = NULL;

unsigned char *mymalloc(unsigned int size);
void myfree(unsigned char *address);
void analyze();
chunkhead* getLastChunk();


int main(){
    // analyze();
    // byte *a = mymalloc(5000);
    // byte *b = mymalloc(1000);
    // byte *c = mymalloc(1000);
    // byte *d = mymalloc(1000);
    // analyze();
    // myfree(a);
    // analyze();
    // byte *e = mymalloc(1000);
    // analyze();
    // myfree(c);
    // analyze();
    // myfree(e);
    // analyze();
    // myfree(b);
    // analyze();
    // myfree(d);
    // analyze();

    analyze();
    byte*a[100];
    clock_t ca = clock();
    for(int i=0; i<100; i++){
        a[i]=mymalloc(1000);
    }
    for(int i=0; i<90; i++){
        myfree(a[i]);
    }
    analyze();
    myfree(a[95]);
    analyze();
    a[95] = mymalloc(100);
    analyze();
    for(int i = 90; i<100;i++){
        myfree(a[i]);
    }
    clock_t cb = clock();
    printf("\n duration: %f \n", ((double)(cb-ca))/CLOCKS_PER_SEC);
    analyze();
    return 0;
}

unsigned char *mymalloc(unsigned int size){
    chunkhead *currentChunk = head;
    void *cCP;
    size = sizeof(chunkhead) + size;
    int pagesToAllocate = ceil(1.0 * size / PAGESIZE);
    int newChunkSize = pagesToAllocate * PAGESIZE;
    chunkhead *returnChunk;
    chunkhead *bestFitChunk = NULL;
    
    if(!head){
        head = sbrk(0);
        cCP = sbrk(newChunkSize);
        if (cCP == (void *)(-1)){
            fprintf(stderr, "\nSbrk failure.");
            return NULL;
        }
        returnChunk = (chunkhead *)head;
        returnChunk->info = 1;
        returnChunk->size = newChunkSize;
        returnChunk->prev = returnChunk->next  = NULL;        
    }else{
        while (currentChunk != NULL)
        {
            if (currentChunk->info != 1){
                if(bestFitChunk == NULL && currentChunk->size >= newChunkSize){
                    bestFitChunk = currentChunk;
                }else if(currentChunk->size >= newChunkSize && currentChunk->size < bestFitChunk->size){
                    bestFitChunk = currentChunk;
                }
            }
            currentChunk = (chunkhead *)(currentChunk->next);
        }
        
        if(bestFitChunk != NULL){
            if(bestFitChunk->size > newChunkSize){
                chunkhead *splitChunk = (chunkhead*)(((void *)bestFitChunk) + newChunkSize);
                splitChunk->info = 0;
                splitChunk->size = bestFitChunk->size - newChunkSize;
                splitChunk->prev = (unsigned char*)bestFitChunk;
                splitChunk->next = bestFitChunk->next;
                bestFitChunk->next = (unsigned char*)splitChunk;
                ((chunkhead *)bestFitChunk->next)->prev = (unsigned char*)splitChunk;
            }
            bestFitChunk->size = newChunkSize;
            bestFitChunk->info = 1;
            returnChunk = bestFitChunk;
        }else{
            returnChunk = (chunkhead*)sbrk(0);
            cCP = sbrk(newChunkSize);
            if(cCP == (void *)(-1)){
                fprintf(stderr, "\nSbrk failure.");
                return NULL;
            }
            returnChunk->info = 1;
            returnChunk->prev = (unsigned char *)getLastChunk();

            ((chunkhead *)returnChunk->prev)->next = (unsigned char*)returnChunk;
            returnChunk->size = newChunkSize;
            returnChunk->next = NULL;
        }
    }

    return (unsigned char*)(((void *)returnChunk) + sizeof(chunkhead));

}

void myfree(unsigned char *address){
    if(address != NULL){
        chunkhead *chunkToBeFreed = (chunkhead *)(((void *)address) - sizeof(chunkhead));
        chunkhead *chunkAfter = (chunkhead *)chunkToBeFreed->next;
        chunkhead *chunkBefore = (chunkhead *)chunkToBeFreed->prev;
        chunkToBeFreed->info = 0;

        if (chunkAfter != NULL && chunkAfter->info == 0)
        {
            chunkToBeFreed->next = chunkAfter->next;
            if (chunkAfter->next != NULL)
            {
                ((chunkhead *)chunkAfter->next)->prev = (unsigned char *)chunkToBeFreed;
            }
            chunkToBeFreed->size += chunkAfter->size;//+sizeof(chunkhead);
            chunkAfter->prev = chunkAfter->next = NULL;
        }

        if (chunkBefore != NULL && chunkBefore->info == 0)
        {
            chunkBefore->next = chunkToBeFreed->next;
            if (chunkToBeFreed->next != NULL)
            {
                ((chunkhead *)chunkToBeFreed->next)->prev = (unsigned char *)chunkBefore;
            }
            chunkBefore->size += chunkToBeFreed->size;//+sizeof(chunkhead);

            chunkToBeFreed->prev = chunkToBeFreed->next = NULL;
        }

        if(getLastChunk() != NULL){
            if(chunkToBeFreed == getLastChunk() || getLastChunk()->info == 0){
                int pagesToAllocate = ceil(1.0 * getLastChunk()->size / PAGESIZE);
                if (head == getLastChunk()){
                    head = NULL;
                }
                brk(sbrk(0) - pagesToAllocate*PAGESIZE);
            }    
        }

    }
    
}

void analyze(){
    printf("\n----------------------------\n");
    if(!head){
        printf("No heap, Program break address: %p\n", sbrk(0));
        return;
    }
    chunkhead* ch = (chunkhead*)head;
    for(int i=0; ch; ch = (chunkhead*)ch->next,i++){
        printf("%d | Current Address: %p |", i, ch);
        printf("Size: %d | ", ch->size);
        printf("Info: %d | ", ch->info);
        printf("Next: %p | ", ch->next);
        printf("Prev: %p", ch->prev);
        printf("\t\n");   
    }
    fprintf(stderr,"Program break on address: %p\n\n", sbrk(0));
}


chunkhead* getLastChunk(){
    if(!head){
        return NULL;
    }

    chunkhead* ch = (chunkhead*)head;
    for(; ch->next; ch = (chunkhead*)ch->next);
    return ch;
}
