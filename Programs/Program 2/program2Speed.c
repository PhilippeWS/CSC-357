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
void *firstFreeChunk = NULL;
void *tail = NULL;

unsigned char *mymalloc(unsigned int size);
void myfree(unsigned char *address);
void analyze();

int main(){
    byte*a[80];
    clock_t ca = clock();
    // analyze();  
    for(int i=10; i<80; i++){
        a[i]=mymalloc(1000);
    }
    for(int i=11; i<50; i++){
        myfree(a[i]);
    }
    // analyze();
    myfree(a[5]);
    a[75] = mymalloc(1000);
    // analyze();
    for(int i = 70; i<80;i++){
        myfree(a[i]);
    }
    // analyze();

    clock_t cb = clock();
    printf("\n duration: %f \n", ((double)(cb-ca))/CLOCKS_PER_SEC);
    return 0;
}

unsigned char *mymalloc(unsigned int size){
    chunkhead *currentChunk = firstFreeChunk;
    if(!head || ((chunkhead*)firstFreeChunk)->info == 1)
        currentChunk = NULL;
    
    size = sizeof(chunkhead) + size;
    int newChunkSize = ceil(1.0 * size / PAGESIZE) * PAGESIZE;
    chunkhead *returnChunk;
    chunkhead *bestFitChunk = NULL;
    
    if(!head){
        head = sbrk(newChunkSize);
            //---Effeciency Change
            tail = head;
            firstFreeChunk = head;
        if (head == (void *)(-1)){
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
                
                if(bestFitChunk == firstFreeChunk){
                    firstFreeChunk = splitChunk;
                    for(chunkhead * ch = (chunkhead*)head;  ch->next != firstFreeChunk; ch = (chunkhead*)ch->next){
                        if(ch->info == 0){
                            firstFreeChunk = ch;
                            break;
                        }
                    }
                }
                    
                
                splitChunk->info = 0;
                splitChunk->size = bestFitChunk->size - newChunkSize;
                splitChunk->prev = (unsigned char*)bestFitChunk;
                splitChunk->next = bestFitChunk->next;
                ((chunkhead *)bestFitChunk->next)->prev = bestFitChunk->next = (unsigned char*)splitChunk;
            }
            bestFitChunk->size = newChunkSize;
            bestFitChunk->info = 1;
            returnChunk = bestFitChunk;
        }else{
            returnChunk = (chunkhead*)sbrk(newChunkSize);
            if(((void*)returnChunk) == (void *)(-1)){
                fprintf(stderr, "\nSbrk failure.");
                return NULL;
            }
            returnChunk->info = 1;
            //---Effeciency Change
            returnChunk->prev = (unsigned char *)tail;
            tail = (void *)returnChunk;

            ((chunkhead *)returnChunk->prev)->next = (unsigned char*)returnChunk;
            returnChunk->size = newChunkSize;
            returnChunk->next = NULL;
        }
    }

    return (unsigned char*)(((void *)returnChunk) + sizeof(chunkhead));

}

void myfree(unsigned char *address){
    if(address != NULL && !(address < (unsigned char *)head) && !(address < (unsigned char *)head) ){
        chunkhead *chunkToBeFreed = (chunkhead *)(((void *)address) - sizeof(chunkhead));

        if (chunkToBeFreed->prev || chunkToBeFreed->next)
            return;

        if(chunkToBeFreed->info == 0)
            return;

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
            if (chunkAfter != NULL)
            {
                ((chunkhead *)chunkToBeFreed->next)->prev = (unsigned char *)chunkBefore;
            }
            chunkBefore->size += chunkToBeFreed->size;//+sizeof(chunkhead);

                //Effeciency Change
            if(((void*)chunkToBeFreed) == tail){
                tail = chunkBefore;
            }
                
            
            chunkToBeFreed->prev = chunkToBeFreed->next = NULL;
            chunkToBeFreed = chunkBefore;
        }

        chunkhead* test = ((chunkhead*)firstFreeChunk);

        if (((void *)chunkToBeFreed < firstFreeChunk || ((chunkhead *)firstFreeChunk)->info == 1)){
                firstFreeChunk = chunkToBeFreed;
        }
        
        

        //Effeciency Change
        if(tail != NULL){
            if(((chunkhead *)tail)->info == 0){ 
                void *newBrk;       
                if (((chunkhead*)head)->next == NULL){
                    chunkToBeFreed = head;
                    newBrk = head;
                    head = NULL;
                    tail = firstFreeChunk = head;
                }else{
                    if(tail != NULL){
                        tail = test->prev;
                        ((chunkhead *)tail)->next = NULL;
                    }
                    newBrk = tail + ((chunkhead*)tail)->size;
                }

                brk(newBrk);
            }    
        }
    }
    
}

void analyze(){
    printf("\n----------------------------\n");
    void *progBreak = sbrk(0);
    if(!head){
        printf("No heap, Program break address: %p\n", progBreak);
        return;
    }
    chunkhead* ch = (chunkhead*)head;
    for(int i=0; ch; ch = (chunkhead*)ch->next,i++){
        fprintf(stderr,"%d | Current Address: %p |", i, ch);
        fprintf(stderr,"Size: %d | ", ch->size);
        fprintf(stderr,"Info: %d | ", ch->info);
        fprintf(stderr,"Next: %p | ", ch->next);
        fprintf(stderr,"Prev: %p", ch->prev);
        fprintf(stderr,"\t\n");   
    }
    fprintf(stderr,"Program break on address: %p\n\n", progBreak);
}

