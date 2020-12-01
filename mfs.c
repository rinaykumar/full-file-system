/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Rinay Kumar
* Student ID: 913859133
* Project: Group Project - File System
*
* File: mfs.c
*
* Description: TDB
*
**************************************************************/

// All of these may not be needed
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include "mfs.h"
#include "fsInode.h"

// Current working directory path
char cwdPath[MAX_FILEPATH_SIZE];
char cwdPathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int cwdPathArraySize = 0;

// After parsing a path, holds each 'level' of the requested file's path
char requestFilePath[MAX_FILEPATH_SIZE];
char requestFilePathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int requestFilePathArraySize = 0;

void parsePath(const char *pathname)
{
    // Set old path as empty (null terminator)
    requestFilePath[0] = '\0';
    requestFilePathArraySize = 0;

    // Make a copy of path name for tokenization
    char pathCopy[MAX_FILEPATH_SIZE];
    strcpy(pathCopy, pathname);

    // Tokenize the path name; set first current token
    char* pathSavePtr;
    char* currentToken = strtok_r(pathCopy, "/", &pathSavePtr);

    // Check for each type of path name
    int isAbsolute = pathname[0] == '/';
    int isParentRelative = !strcmp(currentToken, "..");
    int isSelfRelative = !strcmp(currentToken, ".");

    if (currentToken && isAbsolute == 0)
    {
        int depth;
        if (isParentRelative)
        {
            depth = cwdPathArraySize - 1;
        }
        else
        {
            depth = cwdPathArraySize;
        }

        for (int i = 0; i < depth; i++)
        {
            strcpy(requestFilePathArray[i], cwdPathArray[i]);
            requestFilePathArraySize += 1;
        }
    }

    // Skip token if relative path
    if (isParentRelative == 1 || isSelfRelative == 1)
    {
        currentToken = strtok_r(0, "/", &pathSavePtr);
    }

    // Set the request file path to the pathname
    while (currentToken && requestFilePathArraySize < MAX_DIRECTORY_DEPTH)
    {
        strcpy(requestFilePathArray[requestFilePathArraySize], currentToken);
        requestFilePathArraySize++;
        currentToken = strtok_r(0, "/", &pathSavePtr);
    }
}

int setParent(fs_dir* parent, fs_dir* child)
{
    // Check parent's children if child already exists
    int childExists = 0;
    for (int i = 0; i < parent->numChildren; i++ ) 
    {
        if (!strcmp(parent->children[i], child->name)) 
        {
            childExists = 1;
        }
    }

    if (childExists = 1) 
    {
        printf("Directory '%s' already exists.\n", child->path);
        return 0;
    }

    // Check to see if the parent cannot hold more children
    if (parent->numChildren == MAX_NUMBER_OF_CHILDREN) 
    {
        printf("Directory '%s' is full.\n", parent->path);
        return 0;
    }

    //set the rest of the properties of the parent and child to correspond with each other
    strcpy(parent->children[parent->numChildren], child->name);
    parent->numChildren++;
    parent->lastAccessTime = time(0);
    parent->lastModificationTime = time(0);
    parent->sizeInBlocks += child->sizeInBlocks;
    parent->sizeInBytes += child->sizeInBytes;

    strcpy(child->parent, parent->path);
    sprintf(child->path, "%s/%s", parent->path, child->name);

    printf("Set parent of '%s' to '%s'.\n", child->path, child->parent);
    return 1;
}

int removeChild(fs_dir* parent, fs_dir* child) 
{
    
    for (int i = 0; i < parent->numChildren; i++) 
    {
        // If matching child; delete it
        if (!strcmp(parent->children[i], child->name)) 
        {
            strcpy(parent->children[i], "");
            parent->numChildren--;
            parent->sizeInBlocks -= child->sizeInBlocks;
            parent->sizeInBytes -= child->sizeInBytes;
            return 1;
        }
    }

    printf("Failed to find child '%s' in parent '%s'.\n", child->name, parent->path);
    return 0;
}

char* getParentPath(char* buf, const char* path)
{
    // Parse the path into a tokenized array of path levels
    parseFilePath(path);

    char parentPath[MAX_FILEPATH_SIZE] = "";

    // Loop till we reach the path level before the end
    for (int i = 0; i < requestFilePathArraySize - 1; i++) 
    {
        // Add a separator between each path level
        strcat(parentPath, "/");
        strcat(parentPath, requestFilePathArray[i]);
    }

    strcpy(buf, parentPath);
    printf("Input: %s, Parent Path: %s\n", path, buf);
    return buf;
}

int fs_mkdir(const char *pathname, mode_t mode)
{
    // If using inodes as directories:
    createInode(I_DIR, pathname);

    // Alternative way to create directories:
    // char newdir[] = pathname;
    // char path1[MAX_FILEPATH_SIZE], path2[MAX_FILEPATH_SIZE], *p;
    // mknod(newdir, S_IFDIR|mode);
    // strcpy(path1, newdir);
    // strcat(path1, "/.");
    // link(newdir, path1);
    // strcat(path1, ".");
    // strcpy(path2, newdir);
    // if ((p = strrchr(path2, '/')) == (char *)0) {
    //     link(".", path1);
    // } else {
    //     *p = '\0';
    //     link(path2, path1);
    // }

    return 0;
}

int fs_rmdir(const char *pathname)
{
    // Assuming inodes are used as directories

    // Get the inode
    fs_dir* inodeToRemove = getInode(pathname);

    // Remove inode from parent
    removeFromParent(inodeToRemove->parent, inodeToRemove); // Need to access inode's parent and pass into function

    // Free inode
    freeInode(inodeToRemove);

    return 0;
}

fs_dir* fs_opendir(const char *name) 
{
    // Call getInode 
    fs_dir* openedDir = getInode(name);

    return openedDir;
}

struct fs_dirEntry *fs_readdir(fs_dir *dirp) 
{

}

int fs_closedir(fs_dir *dirp)
{

}

char* fs_getcwd(char *buf, size_t size) 
{
    // Check to see if buffer is too small to fit current working directory name
    if (strlen(cwdPath) > size) 
    {
        errno = ERANGE;
        return NULL;
    }

    strcpy(buf, cwdPath);
    return buf;
}

// Linux chdir
int fs_setcwd(char *buf) 
{
    // Parse the path into a tokenized array of path levels
    parseFilePath(buf);

    // Check if inode exists
    fs_dir* inode = getInode(requestFilePath);
    if (inode == NULL) 
    {
        printf("Directory '%s' does not exist.\n", requestFilePath);
        return 1;
    }

    // Clear old path; '\0' is a null terminator
    cwdPath[0] = '\0';
    cwdPathArraySize = 0;

    // Copy the tokenized path to the current working directory path
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        strcpy(cwdPathArray[i], requestFilePathArray[i]);
        sprintf(cwdPath, "%s/%s", cwdPath, requestFilePathArray[i]);
        cwdPathArraySize++;
    }

    printf("Set cwd to '%s'.\n", cwdPath);
    return 0;
}  

// Return 1 if file, 0 otherwise
int fs_isFile(char * path)
{
    // Get the inode from path
    fs_dir* inode = getInode(path);

    // Check inode type
    if (inode->type == I_FILE) {
        return 1;
    } else {
        return 0;
    }

}

// Return 1 if directory, 0 otherwise
int fs_isDir(char * path) 
{
    // Get the inode from path
    fs_dir* inode = getInode(path);

    // Check inode type
    if (inode->type == I_DIR) {
        return 1;
    } else {
        return 0;
    }

}

// Removes a file
int fs_delete(char* filename) 
{

}	

void fs_init() 
{

}

void fs_close()
{

}

int fs_stat(const char *path, struct fs_stat *buf) 
{

}

// Not sure if this is supposed to be here, but it was in mfs.h 
char* getInodeTypeName(char* buf, InodeType type) 
{

}
