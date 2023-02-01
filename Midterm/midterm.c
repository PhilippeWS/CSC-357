#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>


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


typedef struct col{
    int r,g,b; //red green blue in order.
} col;

struct PIXEL{
    byte blue;
    byte green;
    byte red;
};

typedef struct compressedFormat{
    int width, height; //Width and height of image, with one byte for each color, blue green and red
    int rowbyte_quarter[4]; // For parallel algorithms! the location in bytes which exactly splits the result image after decompression into 4 equal parts.
    int palettecolors; //How many colors the picture has
    col *colors; //All participating colors of the image
} compressedFormat;

typedef struct chunk{
    byte color_index; //Which color in color pallette
    short count; //How many ouxek if the same 
} chunk;

//31895

void readCompressedFileHeader(char fileName[], struct compressedFormat *compressedFileHeader);
struct chunk* readCompressedFileData(char fileName[], compressedFormat compressedFileHeader, int numberOfCompressedChunks);
void createFileHeaders(struct tagBITMAPFILEHEADER *fileHeader, struct tagBITMAPINFOHEADER *infoHeader);

int * splitChunkArray(compressedFormat readCompressedFileHeader);
int getChunkCount(char fileName[], compressedFormat compressedFileHeader);

void injectPixelData(void *pixelArrayPointer, chunk *chunks, compressedFormat compressedFileHeader, int imageWidth , int begginingChunkArrayIndex, int endingChunkArrayIndex, int begginingRowIndex);
void writeFileHeaders(FILE *outputFile, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader);
void writeImageToFile(FILE *outputFile, struct PIXEL **pixels, int imageHeight, int imageWidth);

int main(int argc, char *argv[]){
    //Setup
    char *inputFileName = "compressed.bin"; //argv[1];
    char *outputFileName = "output.bmp";    //argv[3];

    //Read in compressed file header
    compressedFormat compressedFileHeader;
    readCompressedFileHeader(inputFileName, &compressedFileHeader);
    int *chunkArrayIndices = splitChunkArray(compressedFileHeader);

    //Read in chunk data and array size
    int numberOfCompressedChunks = getChunkCount(inputFileName, compressedFileHeader);
    chunk *chunks = readCompressedFileData(inputFileName, compressedFileHeader, numberOfCompressedChunks);

    //Set up output File
    struct tagBITMAPFILEHEADER outFileH;
    struct tagBITMAPINFOHEADER outInfoH;
    createFileHeaders(&outFileH, &outInfoH);

    //Create empty image pixel array
    struct PIXEL **imagePixels = (struct PIXEL **)mmap(NULL, outInfoH.biHeight * sizeof(void *), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    for (int row = 0; row < outInfoH.biHeight; row++){
        imagePixels[row] = (struct PIXEL *)mmap(NULL, outInfoH.biWidth * sizeof(struct PIXEL), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    }

    void *pixelArrayPointer = &imagePixels[0];

    //Inside Great-Grandparent
    int pID1 = fork();
    if (pID1 == 0)
    {
        //Inside Grand-Parent
        int pID2 = fork();
        if (pID2 == 0)
        {
            //Inside Parent
            int pID3 = fork();
            if (pID3 == 0)
            {
                //Inside Child
                injectPixelData(imagePixels, chunks, compressedFileHeader, outInfoH.biWidth, 0, chunkArrayIndices[0], 0);
                exit(0);
            }
            else if (pID3 > 0)
            {
                //Inside Parent
                injectPixelData(imagePixels, chunks, compressedFileHeader, outInfoH.biWidth, chunkArrayIndices[0], chunkArrayIndices[1], outInfoH.biHeight / 4);
                wait(0);
            }
            exit(0);
        }
        else if (pID2 > 0)
        {
            //Inside Grand-Parent
            injectPixelData(imagePixels, chunks, compressedFileHeader, outInfoH.biWidth, chunkArrayIndices[1], chunkArrayIndices[2], outInfoH.biHeight / 2);
            wait(0);
        }
        exit(0);
    }
    else if (pID1 > 0)
    {
        //Inside Great-Grandparent
        injectPixelData(imagePixels, chunks, compressedFileHeader, outInfoH.biWidth, chunkArrayIndices[2], chunkArrayIndices[3], (outInfoH.biHeight * 3) / 4);
        wait(0);
    }

    FILE *outputImage = fopen(outputFileName, "w");
    writeFileHeaders(outputImage, outFileH, outInfoH);
    writeImageToFile(outputImage, imagePixels, outInfoH.biHeight, outInfoH.biWidth);

    //cleanup
    munmap(compressedFileHeader.colors, sizeof(col) * compressedFileHeader.palettecolors);
    munmap(chunks,sizeof(chunk) * numberOfCompressedChunks);
    munmap(chunkArrayIndices,sizeof(int) * 4);

    for (int row = 0; row < outInfoH.biHeight; row++)
        munmap(imagePixels[row], sizeof(outInfoH.biWidth * sizeof(struct PIXEL)));
    munmap(imagePixels, sizeof(outInfoH.biHeight * sizeof(void *)));

    fclose(outputImage);
}

void createFileHeaders(struct tagBITMAPFILEHEADER* fileHeader, struct tagBITMAPINFOHEADER* infoHeader){
    struct tagBITMAPFILEHEADER *fH = fileHeader;
    struct tagBITMAPINFOHEADER *fI = infoHeader;
    //File Header create
    fH->bfType = 19778;
    fH->bfSize = 4320054;
    fH->bfReserved1 = fH->bfReserved2 = 0;
    fH->bfOffBits = 54;
    //Info Header create
    fI->biSize = 40; 
    fI->biWidth = fI->biHeight = 1200;
    fI->biPlanes = 1;
    fI->biBitCount = 24;
    fI->biCompression = fI->biClrUsed = fI->biClrImportant = 0;
    fI->biSizeImage = 4320000;
    fI->biXPelsPerMeter = fI->biYPelsPerMeter = 3780;
}

void readCompressedFileHeader(char fileName[], struct compressedFormat *compressedFileHeader){
    FILE *compressedFile = fopen(fileName,"rb");
    struct compressedFormat *cFH = compressedFileHeader;
    fread(&cFH->width, sizeof(int), 1, compressedFile);
    fread(&cFH->height, sizeof(int), 1, compressedFile);
    fread(&cFH->rowbyte_quarter, 4 * sizeof(int), 1, compressedFile);
    fread(&cFH->palettecolors, sizeof(int), 1, compressedFile);
    cFH->colors = (col *)mmap(NULL, sizeof(col) * compressedFileHeader->palettecolors, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    for(int index = 0; index<compressedFileHeader->palettecolors; index++)
        fread(&cFH->colors[index], sizeof(col), 1, compressedFile);

    fclose(compressedFile);
}

struct chunk* readCompressedFileData(char fileName[], struct compressedFormat compressedFileHeader, int numberOfCompressedChunks){
    FILE *compressedFile = fopen(fileName,"rb");
    chunk *chunks = (chunk *)mmap(NULL, sizeof(chunk) * numberOfCompressedChunks, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    fseek(compressedFile, sizeof(int)*3 + sizeof(compressedFileHeader.rowbyte_quarter) + sizeof(col) * compressedFileHeader.palettecolors, SEEK_SET);

    chunk currentChunk;
    int index = 0;
    while(!feof(compressedFile)){
        fread(&currentChunk.color_index, sizeof(byte), 1, compressedFile);
        fread(&currentChunk.count, sizeof(short), 1, compressedFile);
        chunks[index] = currentChunk;
        index++;
    }
    fclose(compressedFile);
    return chunks;
}

int getChunkCount(char fileName[], compressedFormat compressedFileHeader){
    FILE *compressedFile = fopen(fileName,"rb");

    //Hard coded-----------------------
    fseek(compressedFile, sizeof(int)*7 + sizeof(col) * compressedFileHeader.palettecolors, SEEK_SET);
    chunk currentChunk;
    int numberOfCompressedChunks = 0;
    while(!feof(compressedFile)){
        fread(&currentChunk.color_index, sizeof(byte), 1, compressedFile);
        fread(&currentChunk.count, sizeof(short), 1, compressedFile);
        numberOfCompressedChunks++;
    }
    fclose(compressedFile);
    return numberOfCompressedChunks;
}  

int * splitChunkArray(compressedFormat readCompressedFileHeader){
    int *splitIndeces = (int *)mmap(NULL, sizeof(int) * 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    for(int i = 0; i < 4; i++){
        splitIndeces[i] = readCompressedFileHeader.rowbyte_quarter[i]/(sizeof(short) + sizeof(byte));
    }
    return splitIndeces;
}

void injectPixelData(void *pixelArrayPointer, chunk *chunks, compressedFormat compressedFileHeader, int imageWidth , int begginingChunkArrayIndex, int endingChunkArrayIndex, int begginingRowIndex){
    struct PIXEL **pixels = (struct PIXEL**)(pixelArrayPointer);
    int row = begginingRowIndex;
    int col = 0;

    for (int chunkIndex = begginingChunkArrayIndex; chunkIndex <= endingChunkArrayIndex-1; chunkIndex++){
        chunk currentChunk = chunks[chunkIndex];
        for (int pixelIndex = 0; pixelIndex < currentChunk.count; pixelIndex++){
            pixels[row][col + pixelIndex].blue = (byte)(compressedFileHeader.colors[currentChunk.color_index].b);
            pixels[row][col + pixelIndex].green = (byte)(compressedFileHeader.colors[currentChunk.color_index].g);
            pixels[row][col + pixelIndex].red = (byte)(compressedFileHeader.colors[currentChunk.color_index].r);
        }

        col += currentChunk.count;
        if (col == imageWidth){
            col = 0;
            row++;
        }
    }
}

void writeFileHeaders(FILE *outputFile, struct tagBITMAPFILEHEADER fileHeader, struct tagBITMAPINFOHEADER infoHeader){
    //Write File Header
    fwrite(&fileHeader.bfType, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfSize, sizeof(DWORD), 1, outputFile);
    fwrite(&fileHeader.bfReserved1, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfReserved2, sizeof(WORD), 1, outputFile);
    fwrite(&fileHeader.bfOffBits, sizeof(DWORD), 1, outputFile);
    //Write Info Header
    fwrite(&infoHeader, sizeof(struct tagBITMAPINFOHEADER), 1, outputFile);

    //Prep to write pixels
    fseek(outputFile, fileHeader.bfOffBits, SEEK_SET);
}

void writeImageToFile(FILE *outputFile, struct PIXEL **pixels, int imageHeight, int imageWidth){
    for (int row = 0; row < imageHeight; row++)
    {
        for (int col = 0; col < imageWidth; col++)
        {
            fwrite(&pixels[row][col], sizeof(struct PIXEL), 1, outputFile);
        }
    }
}

