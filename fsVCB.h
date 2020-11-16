/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsVolume.h
*
* Description: Header file for fsVolume.c
*
**************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"

#define DATA_BLOCKS_PER_INODE 4 // Allocated blocks for each Inode
#define VCB_START_BLOCK 0

// Volume control block
typedef struct 
{
  char header[16];
  uint64_t volumeSize;
  uint64_t blockSize;
  uint64_t diskSizeBlocks;
  uint64_t vcbStartBlock;
  uint64_t totalVCBBlocks;

  uint64_t inodeStartBlock;
  uint64_t totalInodes;
  uint64_t totalInodeBlocks;
  uint64_t freeMapSize;
  uint32_t freeMap[];
} fs_VCB;

void initialize(uint64_t _volumeSize, uint64_t _blockSize);
int allocateVCB(fs_VCB** vcb);
void initializeVCB();
fs_VCB* getVCB();
uint64_t readVCB();
uint64_t writeVCB();
uint64_t getFreeBlock();
void printVCB();

int createVolume(char* volumeName, uint64_t _volumeSize, uint64_t _blockSize);
void openVolume(char* volumeName);
void closeVolume();