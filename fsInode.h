/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsInode.h
*
* Description: Header file for fsInode.c
*
**************************************************************/
#include "mfs.h"
#include "fsVCB.c"

#define MAX_FILEPATH_SIZE 225
#define	MAX_FILENAME_SIZE 20
#define MAX_DIRECTORY_DEPTH 10
#define MAX_NUMBER_OF_CHILDREN 64
#define MAX_DATABLOCK_POINTERS	64

typedef struct
{
	char name[MAX_FILENAME_SIZE];  // File name
	char path[MAX_FILEPATH_SIZE];  // File path

	int inUse;
	uint64_t inodeIndex;
	InodeType type;

	char parent[MAX_FILEPATH_SIZE]; // Path name to parent
	char children[MAX_NUMBER_OF_CHILDREN][MAX_FILENAME_SIZE]; // Array of children names
	int numChildren; // Number of children in a DIR

	time_t lastAccessTime;
	time_t lastModificationTime;
	blkcnt_t sizeInBlocks; // 512 for each block
	off_t sizeInBytes; // File size
	int directBlockPointers[MAX_DATABLOCK_POINTERS]; // Array of pointers to data blocks
	int numDirectBlockPointers;
} fs_dir;

char * getInodeTypeName(char* buf, InodeType type);
fs_dir* createInode(InodeType type, const char* path);
fs_dir* getFreeInode();
fs_dir* getInodeByIndex(int index);
int removeFromParent(fs_dir* parent, fs_dir* child);
void writeInodes();
int writeBufferToInode(fs_dir * inode, char* buffer, size_t bufSizeBytes, uint64_t blockNumber);
void freeInode(fs_dir * node);