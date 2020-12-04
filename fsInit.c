/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsInit.c
*
* Description: Responsible for calling fs_init and initializing inodes.
*
**************************************************************/
#include "mfs.h"
#include "fsVCB.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv[]) 
{
    // Needs volumeName in arguments
    if (argc < 2) 
    {
        printf("Missing arguments. Command: make run fsInit volumeName\n");
        return 0;
    }
    printf("OH YEAH\n");

    // Process the args into variables
    char volumeName[MAX_FILENAME_SIZE];
    strcpy(volumeName, argv[1]);
    printf("OH YEAH2\n");

    // Open the existing volume
    openVolume(volumeName);
    printf("OH YEAH3\n");

    // Initialize the file system
    fs_init();
    printf("OH YEAH4\n");
    fs_close();

    // Done with editing the volume
    closeVolume();
}