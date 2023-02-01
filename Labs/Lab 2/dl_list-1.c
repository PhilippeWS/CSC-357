#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct listelement{
    struct listelement *prev, *next;
    char text[1000]; 
};

struct listelement *head = NULL;
int listSize = 0;

//Questions:
    //Will you be testing with command line arguements? do I have to read in arguements as such?

void printMenu();
int validateIntegerInput(char stringInput[], int min, int max);
void menuAction(int menuChoice);
void pushItem(char data[]);
void deleteItem(int index);

int main(void){
    int menuChoice = -1;
    char menuChoiceString[100] = "";

    printf("Select one of the following.\n");
    printMenu();
    scanf("%s", menuChoiceString);
    menuChoice = validateIntegerInput(menuChoiceString, 1, 4);    
    while(menuChoice != 4){
        menuAction(menuChoice);
        printMenu();
        while (getchar() != '\n');
        scanf("%s", menuChoiceString);
        menuChoice = validateIntegerInput(menuChoiceString, 1, 4);    
    }

    for(int i = 1; i <= listSize; i++){
        deleteItem(i);
    }

    printf("Exiting.\n"); 
    return 0;
}

void printMenu(){
    printf("\n1: Push String \n2: Print List \n3: Delete Item \n4: End Program \n\n");
}

int validateIntegerInput(char stringInput[], int min, int max){
    int inputValidate = -1;
    int integerChoice = atoi(stringInput);
    while(inputValidate != 1){
        if(integerChoice != 0){
            for(int i = 0; i < strlen(stringInput); i++){
                if(stringInput[i] == '.'){
                    integerChoice = 0;
                    break;
                }
            }
            if(integerChoice != 0){
                if(integerChoice <= max && integerChoice >= min){
                    inputValidate = 1;
                }else{
                    integerChoice = 0;
                }
            }
        }else{
            printf("Invalid input, please enter a valid option.\n");
            while (getchar() != '\n');
            scanf("%s", stringInput);
            integerChoice = atoi(stringInput);
        }
    }
    return integerChoice;
}

void pushItem(char data[]){
    struct listelement *newItem = (struct listelement*)malloc(sizeof(struct listelement));
    strncpy((*newItem).text, data, sizeof((*newItem).text));

    if(head == NULL){
        newItem->prev = NULL;
        newItem->next = NULL;
        head = newItem;
    }else{
        struct listelement *lastElement = head;
        while(lastElement->next != NULL){
            lastElement = lastElement->next;
        }
        lastElement->next = newItem;
        newItem->prev = lastElement;
        newItem->next = NULL;
    }                
    listSize++;
}

void deleteItem(int index){
    struct listelement *currentElement = head;
    if(index == 1){
        if(listSize == 1){
            head->prev = NULL;
            head->next = NULL;
            head = NULL;
        }else{
            head = currentElement->next;
            currentElement->next->prev = NULL;
        }
    }else if(index == listSize){
        while(currentElement->next != NULL){
            currentElement = currentElement->next;
        }
        (currentElement->prev)->next = NULL;
    }else{
        for(int i = 1; i != index; i++){
            currentElement = currentElement->next;
        }
        (currentElement->next)->prev = currentElement->prev;
        (currentElement->prev)->next = currentElement->next;
    }
    free(currentElement);
    listSize--;
}

void menuAction(int menuChoice){
//Menu Choice 1---------------------------------------------------
    if(menuChoice == 1){
        char inputString[1000];
        printf("Enter a string:\n");
        while (getchar() != '\n');
        scanf("%[^\n]s", inputString);
        pushItem(inputString);
        printf("\nEntry Successful.\n");   
//Menu Choice 2---------------------------------------------------
    }else if(menuChoice == 2){
        if(listSize == 0){
            printf("Nothing to print.\n");
        }else{
            printf("\nPrinting list: \n");
            struct listelement *currentElement = head;
            while(currentElement != NULL){
                printf("%s\n", currentElement->text);
                currentElement = currentElement->next;
            }
        }
//Menu Choice 3---------------------------------------------------
    }else if(menuChoice == 3){
        if(listSize == 0){
            printf("Nothing to delete, list size 0.\n");
        }else{
            char index[100];
            printf("Enter an index:\n");
            scanf("%s", index);
            deleteItem(validateIntegerInput(index, 1, listSize));
            
            printf("Index deleted.\n");

        }
    }
}
