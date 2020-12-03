/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsStart.c
*
* Description: Sets up the file system's volume.
*
**************************************************************/
#include <stdlib.h>
#include "mfs.h"

int main (int argc, char* argv[]) 
{
    if (argc != 4) 
    {
        printf("Missing arguments. Try fsFormat volumeName volumeSize blockSize\n");
        return 0;
    }

    char volumeName[MAX_FILENAME_SIZE];
    uint64_t volumeSize;
    uint64_t blockSize;

    strcpy(volumeName, argv[1]);
    volumeSize = atoi(argv[2]);
    blockSize = atoi(argv[3]);

    int createVolumeCode = createVolume(volumeName, volumeSize, blockSize);
    if (createVolumeCode < 0)
    {
        printf("Call for createVolume function failed.");
        return 0;
    }

    openVolume(volumeName);
    closeVolume();
    return 0;
}