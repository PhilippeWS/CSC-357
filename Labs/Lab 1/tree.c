#include <stdio.h>
#include <stdlib.h>

int drawTree(int height);
void drawToFile(int height, FILE *fp);

//NOTE: I tried to only write one function where the tree was transcribed onto a 2D array, then read into the terminal
    //And the file, which would have made for many less lines of code, but i could not figure out how
    //to debug the strange issues I was getting. I will have to ask about it next class period, but for now,
    //The code works with two functions, one to write to a file and the otehr to write to the terminal.

//void generateTreeArray(int rows, int columns, char treeArr[rows][columns]);

int main(int argc, char *argv[])
{
    FILE *file;
    int height = 0;
    //int width = 0;

    int treeValidate = -1;
    int inputValidate = -1;

    if (argc == 1){
        printf("Enter a height between 0 and 15: \n");
        inputValidate = scanf("%d", &height);

        while (treeValidate != 0){
            if (inputValidate != 1){
                printf("Invalid input, please try again.\n");
                while (getchar() != '\n');
                inputValidate = scanf("%d", &height);
            }else{
                // width = 2 * (height - 4) + 1;
                // char treeArray[height][width];

                // if (width <= 0){
                //     generateTreeArray(height, 0, treeArray);
                //     for(int row = 0; row<height; row++){
                //         for(int col = 0; col < width; col++){
                //             putchar(treeArray[row][col]);
                //         }
                //     }
                // }else{
                //     generateTreeArray(height, width, treeArray);
                //     for(int row = 0; row<height; row++){
                //         for(int col = 0; col < width; col++){
                //             putchar(treeArray[row][col]);
                //         }
                //     }
                // }
                treeValidate = drawTree(height);
                if (treeValidate == -1){
                    inputValidate = -1;
                }
            }
        }
    }else if (argc == 2){
        height = atoi(argv[1]);
        drawTree(height);
    }else if (argc == 3){
        height = atoi(argv[1]);
        file = fopen(argv[2], "w");
        drawTree(height);
        drawToFile(height, file);
        fclose(file);
    }
}

int drawTree(int height){
    if (height <= 15 && height >= 4){
        int rowMetric = height - 3;
        for (int i = 0; i < height; i++){
            //Prints Stem
            if (i > rowMetric){
                for (int x = 3; x > 0; x--){
                    for (int e = rowMetric - 1; e > 0; --e){
                        printf(" ");
                    }
                    printf("*\n");
                }
                break;
            }else{
                //Prints Spaces
                for (int u = rowMetric - i; u > 0; --u){
                    printf(" ");
                }
                //Print Stars
                for (int a = (2 * i - 1); a > 0; --a){
                    printf("*");
                }
            }
            printf("\n");
        }
        return 0;
    }
    else if (height < 4 && height > -1){
        for (int h = 0; h < height; ++h){
            printf(" *\n");
        }
        return 0;
    }
    else{return -1;}
}

void drawToFile(int height, FILE *file){
    if (height <= 15 && height > 4){
        int rowMetric = height - 3;
        for (int i = 0; i < height; i++){
            //Prints Stem
            if (i > rowMetric){
                for (int x = 3; x > 0; x--){
                    for (int e = rowMetric - 1; e > 0; --e){
                        fputs(" ", file);
                    }
                    fputs("*\n", file);
                }
                break;
            }
            else{
                //Prints Spaces
                for (int u = rowMetric - i; u > 0; --u){                        
                        fputs(" ", file);
                }
                //Print Stars
                for (int a = (2 * i - 1); a > 0; --a){
                    fputs("*", file);
                }
                

            }
            fputs("\n", file);
        }
    }else if (height < 4 && height > -1){
        for (int h = 0; h < height; ++h){
            fputs(" *\n", file);
        }
    }
}

// void generateTreeArray(int rows, int columns, char treeArr[rows][columns])
// {
//     if (columns = 0){
//         for (int i = rows; i > 0; i--){
//             treeArr[i][0] = ' ';
//             treeArr[i][1] = '*';
//         }
//     }
//     if (rows <= 15){
//         int rowMetric = rows - 3;
//         for (int row = 1; row < rows; row++){
//             //Prints Stem
//             if (row+1 > rowMetric){
//                 for (int rowIndex = rowMetric; rowIndex < rows; rowIndex++){
//                     for (int colIndex = 0; colIndex < rowMetric - 1; colIndex++){
//                         treeArr[rowIndex][colIndex] = ' ';
//                     }
//                     treeArr[rowIndex][rows-1] = '*';
//                 }
//                 break;
//             }else{
//                 //Prints Spaces
//                 for (int colIndex = 0; colIndex < rowMetric - row; colIndex++){
//                     treeArr[row][colIndex] = ' ';
//                 }
//                 //Print Stars
//                 for (int colIndex = (2*row-1); colIndex > 0; --colIndex){
//                     treeArr[row][colIndex] = '*';
//                 }
//             }
//         }           
//     } 
// }