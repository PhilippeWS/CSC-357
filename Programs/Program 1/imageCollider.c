#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>

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

struct INTERPOLATEDATA{
    int largerImageX;
    int largerImageY;
    int smallerImageWidth;
    int smallerImageHeight;
    int largerImageWidth;
    int largerImageHeight;
};

void printManual(char * errorMessge);
char * inputValidate(int numArg, char *argv[]);
int validateFileType(char fileName[]);
struct PIXEL combinePixel(float ratio, struct PIXEL *pixel1, struct PIXEL *pixel2);
struct PIXEL** readInPixelData(char fileName[], struct PIXEL **pixels, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader);
struct PIXEL interpolatePixel(struct INTERPOLATEDATA intData, struct PIXEL **smallerPixelData);
struct PIXEL scalePixelValues(struct PIXEL pixel, float scalar);
int calculatePaddingOffset(int imageWidth);
void readFileHeaders(char fileName[], struct tagBITMAPFILEHEADER *fileHeader, struct tagBITMAPINFOHEADER *infoHeader);
void writeFileHeaders(FILE *outputFile, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader);

int main(int argc, char *argv[]){
    if(inputValidate(argc, argv) == NULL){
        FILE *outPutImage;

        char *fileName1 = argv[1];
        char *fileName2 = argv[2];

        struct tagBITMAPFILEHEADER image1FileH;
        struct tagBITMAPINFOHEADER image1InfoH;

        struct tagBITMAPFILEHEADER image2FileH;
        struct tagBITMAPINFOHEADER image2InfoH;
        
        readFileHeaders(fileName1, &image1FileH, &image1InfoH);
        readFileHeaders(fileName2, &image2FileH, &image2InfoH);
        
        float ratio = atof(argv[3]);
        outPutImage = fopen(argv[4], "w");

        struct PIXEL **pixelsImage1 = (struct PIXEL **)malloc(image1InfoH.biHeight * sizeof(void *));
        pixelsImage1 = readInPixelData(fileName1, pixelsImage1, image1FileH, image1InfoH);

        struct PIXEL **pixelsImage2 = (struct PIXEL **)malloc(image2InfoH.biHeight * sizeof(void *));
        pixelsImage2 = readInPixelData(fileName2, pixelsImage2, image2FileH, image2InfoH);

        byte paddingByte = 0;
        int paddingOffset = calculatePaddingOffset(image1InfoH.biWidth);

        if(image1InfoH.biWidth != image2InfoH.biWidth){
            struct INTERPOLATEDATA data;
            struct PIXEL **largerPixelArray;
            struct PIXEL **smallerPixelArray;
            if(image1InfoH.biWidth > image2InfoH.biWidth){ //Image1 larger than image 2 case.
                data.largerImageWidth = image1InfoH.biWidth;
                data.largerImageHeight = image1InfoH.biHeight;
                data.smallerImageWidth = image2InfoH.biWidth;
                data.smallerImageHeight = image2InfoH.biHeight;
                largerPixelArray = pixelsImage1;
                smallerPixelArray = pixelsImage2;
                writeFileHeaders(outPutImage, image1FileH, image1InfoH);
            }else{
                data.largerImageWidth = image2InfoH.biWidth;
                data.largerImageHeight = image2InfoH.biHeight;
                data.smallerImageWidth = image1InfoH.biWidth;
                data.smallerImageHeight = image1InfoH.biHeight;
                largerPixelArray = pixelsImage2;
                smallerPixelArray = pixelsImage1;
                writeFileHeaders(outPutImage, image2FileH, image2InfoH);
                paddingOffset = calculatePaddingOffset(image2InfoH.biWidth);

            }

            for (int row = 0; row < data.largerImageHeight; row++){
                for (int col = 0; col < data.largerImageWidth; col++){
                    data.largerImageX = col;
                    data.largerImageY = row;
                    struct PIXEL interpolatedPixel = interpolatePixel(data, smallerPixelArray);
                    struct PIXEL newPixel = combinePixel(ratio, &largerPixelArray[row][col], &interpolatedPixel);
                    fwrite(&newPixel, sizeof(struct PIXEL), 1, outPutImage);
                }
                for (int i = 0; i < paddingOffset; i++){
                    fwrite(&paddingByte, sizeof(byte), 1, outPutImage);
                }
            }
        }else{ //Same Resolution Files Case
            //Write Headers to output file
            writeFileHeaders(outPutImage, image1FileH, image1InfoH);
            //Combine images
            for (int row = 0; row < image1InfoH.biHeight; row++){
                for (int col = 0; col < image1InfoH.biWidth; col++){
                    struct PIXEL newPixel = combinePixel(ratio, &pixelsImage1[row][col], &pixelsImage2[row][col]);
                    fwrite(&newPixel, sizeof(struct PIXEL), 1, outPutImage);
                }
                for (int i = 0; i < paddingOffset; i++){
                    fwrite(&paddingByte, sizeof(byte), 1, outPutImage);
                }
            }
        }

        //Clean up and Free memory
        for (int row = 0; row < image1InfoH.biHeight; row++)
            (free(pixelsImage1[row]));
        free(pixelsImage1);

        for (int row = 0; row < image2InfoH.biHeight; row++)
            (free(pixelsImage2[row]));
        free(pixelsImage2);

        fclose(outPutImage);
    }else{
        printManual(inputValidate(argc, argv));
    }

}

void printManual(char *errorMessage){
    printf("\t\t\t\t\t---MANUAL PAGE---");
    printf("\nSYNOPSIS\n");
    printf("\t./imageCollider imageName1.bmp imageName2.bmp imageRatio outPutImageName\n");
    printf("\nDESCRIPTION\n");
    printf("\tImage Collider recieves two images of the .bmp format, combines both images pixel values together and outputs them as a new \n\tspecified file in the resolution of the larger image.\n");
    printf("\nSYNTAX\n");
    printf("\t./imageCollider - Call within the command line to run the program. Requires four arguements to be passed along side for full functionality.\n");
    printf("\n\tArguments:\n");
    printf("\t\timageName1.bmp  - Datatype: string | Description: The name of the first image file to be combined. This image will be\n\t\t\t\tmixed by *(imageRatio) with the second image.\n\n");
    printf("\t\timageName2.bmp  - Datatype: string | Description: The name of the second image file to be combined. This image will be\n\t\t\t\tmixed by *(1-imageRatio) with the first image.\n\n");
    printf("\t\timageRatio      - Datatype: float  | Description: The numeric value that will used to ratio the pixel values of Image1\n\t\t\t\tand Image2 while combining. Must be between 0.0 and 1.0.\n\n");
    printf("\t\toutPutImageName - Datatype: string | Description: The name of the resulting file where the combination of Image1 and\n\t\t\t\tImage2 will be stored.\n");
    printf("ERROR\n");
    printf("\t%s\n", errorMessage);

}

char * inputValidate(int numArg, char *argv[]){
    char *errorMsg = NULL;
    if (numArg > 5 || numArg < 5){
        errorMsg = "Invalid number of arguments.";
        return errorMsg;
    }else if(validateFileType(argv[1]) == -1 || validateFileType(argv[2]) == -1 || validateFileType(argv[4]) == -1){
        errorMsg = "Invalid file type(s).";
        return errorMsg;
    }else if (atof(argv[3]) < 0.0 || atof(argv[3]) > 1.0){
        errorMsg = "Invalid ratio.";
        return errorMsg;
    }else if(access(argv[1], F_OK) != 0 || access(argv[2], F_OK) != 0){
        errorMsg = "Invalid file name(s).";
        return errorMsg;
    }
    else{
        if(atof(argv[3]) == 0){
            int periodAmnt = 0;
            char *input = argv[3];
            for(int i = 0; i < strlen(argv[3]); i++){
                if(isalpha(input[i]) != 0){
                    errorMsg = "Invalid ratio.";
                    break;
                }else if(ispunct(input[i]) != 0){
                    if(strchr(&input[i], '.') != NULL){
                        periodAmnt++;
                        if(periodAmnt > 1){
                            errorMsg = "Invalid ratio.";
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

struct PIXEL combinePixel(float ratio, struct PIXEL *pixel1, struct PIXEL *pixel2){
    struct PIXEL newPixel;
    newPixel.red = pixel1->red * ratio + pixel2->red * (1 - ratio);
    newPixel.green = pixel1->green * ratio + pixel2->green * (1 - ratio);
    newPixel.blue = pixel1->blue * ratio + pixel2->blue * (1 - ratio);
    return newPixel;
}

struct PIXEL** readInPixelData(char fileName[], struct PIXEL **pixels, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader){
    FILE *image = fopen(fileName, "rb");
    byte nullByte = 0;

    fseek(image, fileHeader.bfOffBits, SEEK_SET);

    int paddingOffset = calculatePaddingOffset(infoHeader.biWidth);

    //Store pixel data in Array        
    for (int row = 0; row < infoHeader.biHeight; row++){
        pixels[row] = (struct PIXEL *)malloc(infoHeader.biWidth * sizeof(struct PIXEL));
        fread(pixels[row], sizeof(struct PIXEL), infoHeader.biWidth, image);
        for (int i = 0; i < paddingOffset; i++)
        {
            fread(&nullByte, sizeof(byte), 1, image);
        }

    }
    fclose(image);
    return pixels;
}

struct PIXEL interpolatePixel(struct INTERPOLATEDATA intData, struct PIXEL **smallerImagePixelData){
    //Position of the needed point as a float. 
    float xS, yS;
    //Nearest Bottom Left Pixel coordinates to the floating point; 
    int cLPx, cLPy;
    //Distance of floating point to Bottom left pixel.
    float dx, dy;

    xS = intData.largerImageX*(1.0*intData.smallerImageWidth/intData.largerImageWidth);
    yS = intData.largerImageY*(1.0*intData.smallerImageHeight/intData.largerImageHeight);
    
    cLPx = floor(xS);
    cLPy = floor(yS);

    dx = xS-cLPx;
    dy = yS-cLPy;

    struct PIXEL bLP, tLP, bRP, tRP, adjustedPixel;
    bLP = smallerImagePixelData[cLPy][cLPx];

    //First case is any of the corners, which then is simply composed of the corner pixel
    if(cLPx == 0 && cLPy == 0 || 
        cLPx == intData.smallerImageWidth-1 && cLPy == intData.smallerImageHeight-1 ||
        cLPx == 0 && cLPy == intData.smallerImageHeight-1 ||
        cLPx == intData.smallerImageWidth-1 && cLPy == 0){
            adjustedPixel = bLP;
    //Along the left or right most Column, not a corner.
    }else if(cLPx == 0 || cLPx == intData.smallerImageWidth-1){
        tLP = smallerImagePixelData[cLPy+1][cLPx];
        bLP = scalePixelValues(bLP, (1-dy));
        tLP = scalePixelValues(tLP, dy);
        adjustedPixel.red = bLP.red + tLP.red;
        adjustedPixel.green = bLP.green + tLP.green;
        adjustedPixel.blue = bLP.blue + tLP.blue;
    //Along the bottom most or top most row, not corner.
    }else if(cLPy == 0 || cLPy == intData.smallerImageHeight-1){
        bRP = smallerImagePixelData[cLPy][cLPx+1];
        bLP = scalePixelValues(bLP, (1-dx));
        bRP = scalePixelValues(bRP, dx);
        adjustedPixel.red = bLP.red + bRP.red;
        adjustedPixel.green = bLP.green + bRP.green;
        adjustedPixel.blue = bLP.blue + bRP.blue;
    //Not along edge or in corner
    }else{
        bRP = smallerImagePixelData[cLPy][cLPx+1];
        tLP = smallerImagePixelData[cLPy+1][cLPx];
        tRP = smallerImagePixelData[cLPy+1][cLPx+1];
        bLP = scalePixelValues(bLP, (1-dy));
        bRP = scalePixelValues(bRP, (1-dy));
        tLP = scalePixelValues(tLP, dy);
        tRP = scalePixelValues(tRP, dy);
        adjustedPixel.red = (bLP.red + tLP.red)*(1-dx) + (bRP.red + tRP.red)*(dx);
        adjustedPixel.green = (bLP.green + tLP.green)*(1-dx) + (bRP.green + tRP.green)*(dx);
        adjustedPixel.blue = (bLP.blue + tLP.blue)*(1-dx) + (bRP.blue + tRP.blue)*(dx);

    }
    return adjustedPixel;  
}

struct PIXEL scalePixelValues(struct PIXEL pixel, float scalar){
    struct PIXEL modifiedPixel;
    modifiedPixel.red = pixel.red * scalar;
    modifiedPixel.green = pixel.green * scalar;
    modifiedPixel.blue = pixel.blue * scalar;
    return modifiedPixel;
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
 
