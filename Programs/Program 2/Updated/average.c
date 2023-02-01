#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    FILE *infile = fopen("times.txt", "r");
    double average = 0;
    while(!feof(infile)){
        char current[10];
        fscanf(infile, "%s", current);
        average += atof(current);
    }
    average = average/1000.0;
    printf("Average: %f\n", average);
    fclose(infile);
}