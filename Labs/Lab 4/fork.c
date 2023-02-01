#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

//#define CLOCKS_PER_SEC ((clock_t) 1000000)
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int LONG;
typedef unsigned char byte;
struct tagBITMAPFILEHEADER
{
    WORD bfType;      //file type
    DWORD bfSize;     //size in bytes of file
    WORD bfReserved1; //must be 0, reserved
    WORD bfReserved2; //must be 0, reserved
    DWORD bfOffBits;  //offset in bytes from the bitmapfileheader to the bitmap bits
};
struct tagBITMAPINFOHEADER{
    DWORD biSize;         //number of bytes required by teh struct
    LONG biWidth;         //width in pixels
    LONG biHeight;        //height in pixels
    WORD biPlanes;        //number of color planes, must be 1
    WORD biBitCount;      //number of but per pixel
    DWORD biCompression;  //type of compression
    DWORD biSizeImage;    //size of image in bytes
    LONG biXPelsPerMeter; //number of pixels per meter in x axis
    LONG biYPelsPerMeter; //number of pixels per meter in y axis
    DWORD biClrUsed;      //number of colors used be the bitmap
    DWORD biClrImportant; //number of colors that are important
};
struct HEADER{
    struct tagBITMAPFILEHEADER *fileHeader;
    struct tagBITMAPINFOHEADER *infoHeader;
};

struct PIXEL{
    byte blue;
    byte green;
    byte red;
};

void printManual(char * errorMessge);
char * inputValidate(int numArg, char *argv[]);
char * numericValidate(char stringChar[], int allowedFloat);
int validateFileType(char fileName[]);
struct PIXEL brightenPixel(int brightnessFactor, struct PIXEL *pixel);
struct PIXEL** readInPixelData(char fileName[], struct PIXEL **pixels, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader);
int calculatePaddingOffset(int imageWidth);
void readFileHeaders(char fileName[], struct tagBITMAPFILEHEADER *fileHeader, struct tagBITMAPINFOHEADER *infoHeader);
void writeFileHeaders(FILE *outputFile, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader);
void writeImageToFile(FILE *outputFile, struct PIXEL **pixels, int imageHeight, int imageWidth, int paddingOffset);

int main(int argc, char *argv[]){
    // fileName brightness parrallel outputfile
    if(inputValidate(argc, argv) == NULL){
        FILE *outPutImage;
        char *fileName = argv[1];

        struct tagBITMAPFILEHEADER imageFileH;
        struct tagBITMAPINFOHEADER imageInfoH;
        
        readFileHeaders(fileName, &imageFileH, &imageInfoH);

        int brightnessFactor = atof(argv[2]) * 255;
        int parrallel = atoi(argv[3]);
        outPutImage = fopen(argv[4], "w");

        struct PIXEL **imagePixels = (struct PIXEL **)mmap(NULL, imageInfoH.biHeight * sizeof(void *), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        imagePixels = readInPixelData(fileName, imagePixels, imageFileH, imageInfoH);

        byte paddingByte = 0;
        int paddingOffset = calculatePaddingOffset(imageInfoH.biWidth);
        writeFileHeaders(outPutImage, imageFileH, imageInfoH);
        fseek(outPutImage, imageFileH.bfOffBits, SEEK_SET);
        
        clock_t start = clock();
        char *processingType;
        int pID = -1;
        if(parrallel == 1){
            int lower = ceil(imageInfoH.biHeight / 2);
            int upper = imageInfoH.biHeight - lower;
            processingType = "Parallel";
            pID = fork();
            if(pID == 0){
                for (int row = 0; row < lower; row++)
                {
                    for (int col = 0; col < imageInfoH.biWidth; col++)
                    {
                        imagePixels[row][col] = brightenPixel(brightnessFactor, &imagePixels[row][col]);
                    }
                }
            }
            else if(pID > 0){
                for (int row = upper; row < imageInfoH.biHeight; row++)
                {
                    for (int col = 0; col < imageInfoH.biWidth; col++)
                    {
                        imagePixels[row][col] = brightenPixel(brightnessFactor, &imagePixels[row][col]);
                    }
                }
                wait(NULL);
            }
        }else{
            processingType = "Non-Parallel";
            for (int row = 0; row < imageInfoH.biHeight; row++)
            {
                for (int col = 0; col < imageInfoH.biWidth; col++)
                {
                    imagePixels[row][col] = brightenPixel(brightnessFactor, &imagePixels[row][col]);
                }
            }
        }   

        if(parrallel == 0 || pID > 0){
            clock_t end = clock();
            fprintf(stderr, "\n%s processing time: %f\n", processingType,((double)(end - start)/CLOCKS_PER_SEC));
            //write to file
            writeImageToFile(outPutImage, imagePixels, imageInfoH.biHeight, imageInfoH.biWidth, paddingOffset);

            //Clean up and Free memory
            for (int row = 0; row < imageInfoH.biHeight; row++)
                munmap(imagePixels[row], sizeof(imageInfoH.biWidth * sizeof(struct PIXEL)));
            munmap(imagePixels, sizeof(imageInfoH.biHeight * sizeof(void *)));

            fclose(outPutImage);
        } 
        }else{
            printManual(inputValidate(argc, argv));
        }
    }

void printManual(char *errorMessage){
    printf("\t\t\t\t\t---MANUAL PAGE---");
    printf("\nSYNOPSIS\n");
    printf("\t./fork [imageName.bmp] [brightnessFactor] [parallelProcessing] [outPutImageName]\n");
    printf("\nDESCRIPTION\n");
    printf("\tFork recieves an images of the .bmp format, brightens it by the input factor, then returns the image as [outPutImageName].bmp.");
    printf("\nSYNTAX\n");
    printf("\t./fork - Call within the command line to run the program. Requires four arguements to be passed along side for full functionality.\n");
    printf("\n\tArguments:\n");
    printf("\t\timageName.bmp      - Datatype: string  | Description: The name of the image file to be brightened. This image will be\n\t\t\t\tbrightened by [birghtnessFactor]*255. Must be of the .bmp file type.\n\n");
    printf("\t\tbrightnessFactor   - Datatype: float   | Description: The numeric value that will used to brighten the image by [brightnessFactor]*255.\n\t\t\t\tMust be between 0.0 and 1.0.\n\n");
    printf("\t\tparallelProcessing - Datatype: int     | Description: A 0(False) or 1(True) indicating whether or not to complete the brightening through \n\t\t\t\tparrallel processing.\n\n");
    printf("\t\toutPutImageName    - Datatype: string  | Description: The name of the resulting file where the brightened image will reside \n\t\t\t\tMust be of .bmp file type.\n");
    printf("ERROR\n");
    printf("\t%s\n", errorMessage);

}

char * inputValidate(int numArg, char *argv[]){
    char *errorMsg = NULL;
    if (numArg > 5 || numArg < 5){
        errorMsg = "Invalid number of arguments.";
        return errorMsg;
    }else if(validateFileType(argv[1]) == -1 || validateFileType(argv[4]) == -1){
        errorMsg = "Invalid file type(s).";
        return errorMsg;
    }else if (atof(argv[2]) < 0.0 || atof(argv[2]) > 1.0){
        errorMsg = "Invalid ratio.";
        return errorMsg;
    }else if(access(argv[1], F_OK) != 0){
        errorMsg = "Invalid input file name.";
        return errorMsg;
    }else if(!(atoi(argv[3]) == 0 || atoi(argv[3]) == 1)){
        int test = atoi(argv[3]);
        errorMsg = "Invalid boolean.";
        return errorMsg;
    }else{
        if(numericValidate(argv[2], 1) == NULL && numericValidate(argv[3], 0) == NULL){
            return errorMsg;
        }else{
            errorMsg = "Invalid numeric value(s).";
            return errorMsg;
        }
    }
}

char * numericValidate(char stringChar[], int allowedFloat){
    char * errorMsg = NULL;
    if (atof(stringChar) == 0){
        int periodAmnt = 0;
        for (int i = 0; i < strlen(stringChar); i++){
            if (isalpha(stringChar[i]) != 0){
                errorMsg = "Invalid numerical value.";
                break;
            }else if (ispunct(stringChar[i]) != 0){
                if (strchr(&stringChar[i], '.') != NULL){
                    periodAmnt++;
                    if(allowedFloat == 0){
                        errorMsg = "Invalid integer value.";
                        break;
                    }else if(periodAmnt > 1){
                        errorMsg = "Invalid float value.";
                        break;
                    }
                }else{
                    errorMsg = "Invalid ratio.";
                    break;
                }
            }
        }
        return errorMsg;
    }else{
        return errorMsg;
    }
}

int validateFileType(char fileName[]){
    int fileNameLength = strlen(fileName);
    if (fileNameLength < 4){
        return -1;
    }
    else{
        char fileType[4];
        for (int i = 0; i < 4; i++){
            fileType[i] = fileName[fileNameLength -(4-i)];
        }
        if(strcmp(".bmp", fileType) == 0){
            return 0;
        }
        else{
            return -1;
        }
    }
}

struct PIXEL brightenPixel(int brightnessFactor, struct PIXEL *pixel){
    struct PIXEL newPixel;
    newPixel.blue = (brightnessFactor + pixel->blue > 255) ? 255 : brightnessFactor + pixel->blue;
    newPixel.green = (brightnessFactor + pixel->green > 255) ? 255 : brightnessFactor + pixel->green;
    newPixel.red = (brightnessFactor + pixel->red > 255) ? 255 : brightnessFactor + pixel->red;
    return newPixel;
}

struct PIXEL** readInPixelData(char fileName[], struct PIXEL **pixels, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader){
    FILE *image = fopen(fileName, "rb");
    byte nullByte = 0;

    fseek(image, fileHeader.bfOffBits, SEEK_SET);

    int paddingOffset = calculatePaddingOffset(infoHeader.biWidth);

    //Store pixel data in Array        
    for (int row = 0; row < infoHeader.biHeight; row++){
        pixels[row] = (struct PIXEL *)mmap(NULL, infoHeader.biWidth*sizeof(struct PIXEL), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        fread(pixels[row], sizeof(struct PIXEL), infoHeader.biWidth, image);
        for (int i = 0; i < paddingOffset; i++)
        {
            fread(&nullByte, sizeof(byte), 1, image);
        }

    }
    fclose(image);
    return pixels;
}

int calculatePaddingOffset(int imageWidth){
    int paddingOffset = 4 - (imageWidth*3)%4;
    if((imageWidth*3)%4 == 0){
        paddingOffset = 0;
    }
    return paddingOffset;
}

void readFileHeaders(char fileName[], struct tagBITMAPFILEHEADER* fileHeader, struct tagBITMAPINFOHEADER* infoHeader){
    FILE *file = fopen(fileName, "rb");
    struct tagBITMAPFILEHEADER *fH = fileHeader;
    struct tagBITMAPINFOHEADER *fI = infoHeader;
    //File Header read in
    fread(&fH->bfType, sizeof(WORD), 1, file);
    fread(&fH->bfSize, sizeof(DWORD), 1, file);
    fread(&fH->bfReserved1, sizeof(WORD), 1, file);
    fread(&fH->bfReserved2, sizeof(WORD), 1, file);
    fread(&fH->bfOffBits, sizeof(DWORD), 1, file);
    //Info Header read in
    fread(fI, sizeof(struct tagBITMAPINFOHEADER), 1, file);

    fclose(file);
}

void writeFileHeaders(FILE *outputFile, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader){
    //Write both headers
    fwrite(&fileHeader.bfType, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfSize, sizeof(DWORD), 1, outputFile);
    fwrite(&fileHeader.bfReserved1, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfReserved2, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfOffBits, sizeof(DWORD), 1, outputFile);
    fwrite(&infoHeader, sizeof(struct tagBITMAPINFOHEADER), 1, outputFile);
    //Prep to write pixels
    fseek(outputFile, fileHeader.bfOffBits, SEEK_SET);
}

void writeImageToFile(FILE *outputFile, struct PIXEL **pixels, int imageHeight, int imageWidth, int paddingOffset){
    byte paddingByte = 0;
    for (int row = 0; row < imageHeight; row++)
    {
        for (int col = 0; col < imageWidth; col++)
        {
            fwrite(&pixels[row][col], sizeof(struct PIXEL), 1, outputFile);
        }
        for (int i = 0; i < paddingOffset; i++)
        {
            fwrite(&paddingByte, sizeof(byte), 1, outputFile);
        }
    }
}
