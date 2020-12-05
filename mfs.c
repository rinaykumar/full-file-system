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
#include "fsVCB.h"
#include "b_io.h"

// Current working directory path
char cwdPath[MAX_FILEPATH_SIZE];
char cwdPathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int cwdPathArraySize = 0;

// After parsing a path, holds each 'level' of the requested file's path
char requestFilePathArray[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
int requestFilePathArraySize = 0;

void parseFilePath(const char *pathname)
{
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
        printf("Path is relative\n");
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
    // printf("Input: %s, Parent Path: %s\n", path, buf);
    return buf;
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

    if (childExists == 1) 
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

    // Set other attributes of parent
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

fs_dir* createInode(InodeType type, const char* path)
{
    fs_dir * inode;
    char parentPath[MAX_FILEPATH_SIZE];
    fs_dir* parentNode;

    // Set the current creation time
    time_t currentTime;
    currentTime = time(NULL);

    // Find and assign the parent to the new inode
    inode = getFreeInode();
    getParentPath(parentPath, path);
    parentNode = getInode(parentPath);

    // Set inode info
    inode->type = type;
    strcpy(inode->name , requestFilePathArray[requestFilePathArraySize - 1]);
    sprintf(inode->path, "%s/%s", parentPath, inode->name);
    inode->lastAccessTime = currentTime;
    inode->lastModificationTime = currentTime;

    // Set the inode's parent
    if (!setParent(parentNode, inode)) 
    {
        freeInode(inode);
        printf("Failed to set parent.\n");
        return NULL;
    }

    printf("Sucessfully created inode for path '%s'.\n", path);       
    return inode;
}

fs_dir* inodes;
void fs_init() 
{
    printf("totalInodeBlocks %ld, blockSize %ld\n", getVCB()->totalInodeBlocks, getVCB()->blockSize);
    inodes = calloc(getVCB()->totalInodeBlocks, getVCB()->blockSize);
    printf("Inodes allocated at %p.\n", inodes);

    uint64_t blocksRead = LBAread(inodes, getVCB()->totalInodeBlocks, getVCB()->inodeStartBlock);
    printf("%ld inode blocks were read.\n", blocksRead);

    // Return failed if not enough blocks read
    if (blocksRead != getVCB()->totalInodeBlocks)
    {
        printf("fs_init: Failed to read all inode blocks.\n");
        fs_close();
        exit(0);
    }

    // Initialize the root directory
    fs_setcwd("/root");
}

void fs_close()
{
    free(inodes);
}

int fs_mkdir(const char *pathname, mode_t mode)
{
    // Parse the path into a tokenized array of path levels
    parseFilePath(pathname);

    // Combine tokens into a single char string; gets the parent of the called level
    char parentPath[256] = "";
    for (int i = 0; i < requestFilePathArraySize - 1; i++)
    {
        // Append a '/' before each token
        strcat(parentPath, "/");
        strcat(parentPath, requestFilePathArray[i]);
    }

    // Return failure if directory already exists or if parent does not exist
    fs_dir* parentDir = getInode(parentPath);
    if (parentDir)
    {
        for (int i = 0; i < parentDir->numChildren; i++)
        {
            // Compare current child's name to the request file path level
            int dirExists = strcmp(parentDir->children[i], requestFilePathArray[requestFilePathArraySize - 1]);
            if (dirExists == 0)
            {
                printf("mkdir: Directory %s already exists.\n", parentDir->children[i]);
                return -1;
            }
        }
    }
    else
    {
        printf("mkdir: Parent does not exist.");
        return -1;
    }

    // Can finally create the directory
    fs_dir* newDir = createInode(I_DIR, pathname);
    if (newDir != NULL)
    {
        LBAwrite(inodes, getVCB()->totalInodeBlocks, getVCB()->inodeStartBlock);
        return 0;
    }

    printf("mkdir: Failed to create inode. '%s'.\n", pathname);
    return -1;
}

int fs_rmdir(const char *pathname)
{
    // Parse the path into a tokenized array of path levels
    parseFilePath(pathname);

    // Piece together the full file path
    char fullPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        // Add a separator between each path level
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    // Check if inode exists
    fs_dir* inodeToRemove = getInode(fullPath);
    if (!inodeToRemove) 
    {
        printf("Directory '%s' does not exist.\n", fullPath);
        return 1;
    }

    char parentPath[MAX_FILEPATH_SIZE];
    getParentPath(parentPath, pathname);
    fs_dir* parentInode = getInode(parentPath);
    if (!parentInode) 
    {
        printf("Directory '%s' does not exist.\n", fullPath);
        return 1;
    }

    // Remove inode from parent
    removeFromParent(parentInode, inodeToRemove); // Need to access inode's parent and pass into function

    // Free inode
    freeInode(inodeToRemove);
    return 0;
}

fs_dir* fs_opendir(char *pathname) 
{
    int openCode = b_open(pathname, 0);
    if (openCode < 0)
    {
        printf("Open failed\n");
        return NULL;
    }

    // Parse the path into a tokenized array of path levels
    parseFilePath(pathname);

    // Piece together the full file path
    char fullPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        // Add a separator between each path level
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    fs_dir* inode = getInode(fullPath);
    inode->fd = openCode;
    return inode;
}

struct fs_dirEntry dirEntry;
int childIndex = 0;
struct fs_dirEntry *fs_readdir(fs_dir *dirp) 
{
    // Check if index is at the end
    if (childIndex == dirp->numChildren)
    {
        childIndex = 0;
        return NULL;
    }

    // Based on the following: "readdir returns a pointer to a dirent structure representing the next directory entry"
    // Get inode
    fs_dir* inode = getInode(dirp->path);

    // Set inode properties to directory entry
    strcpy(dirEntry.d_name, inode->children[childIndex]);
    dirEntry.d_ino = inode->inodeIndex;
    dirEntry.fileType = inode->type;

    // Increment child index
    childIndex++;
    
    // Retun directory entry
    return &dirEntry;
}

int fs_closedir(fs_dir *dirp)
{   
    int fd = dirp->fd;
    b_close(fd);
    return 0;
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

    // Piece together the full file path
    char fullPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        // Add a separator between each path level
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    // Check if inode exists
    fs_dir* inode = getInode(fullPath);
    if (!inode) 
    {
        printf("Directory '%s' does not exist.\n", fullPath);
        return 1;
    }

    // Check if already in root directory
    if (strcmp(buf, "..") == 0 && strcmp(cwdPath, "/root") == 0) 
    {
        printf("Already in root directory.\n");
        return 1;
    }

    // Check if path is a file
    if (inode->type != I_DIR) 
    {
        printf("Path not a directory. (%s)\n", fullPath);
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
    // Parse the path into a tokenized array of path levels
    parseFilePath(path);

    // Piece together the full file path
    char fullPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        // Add a separator between each path level
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    // Get the inode from path
    fs_dir* inode = getInode(fullPath);

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
    // Parse the path into a tokenized array of path levels
    parseFilePath(path);

    // Piece together the full file path
    char fullPath[MAX_FILEPATH_SIZE] = "";
    for (int i = 0; i < requestFilePathArraySize; i++) 
    {
        // Add a separator between each path level
        strcat(fullPath, "/");
        strcat(fullPath, requestFilePathArray[i]);
    }

    // Get the inode from path
    fs_dir* inode = getInode(fullPath);

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
    // First way:

    // Get all inodes, store into allInodes
    // allInodes = all inodes
    // Loop through inodes and check for filename match in children
    // for (int i = 0; i < allInodes->numChildren; i++) {
    //     if (strcmp(filename, allInodes->children[i])) {
    //         // Delete child/file 

    //         return 1;
    //     }
    // }

    // Second way:
    
    // Get inode
    fs_dir* inode = getInode(filename);
    fs_dir* parentInode = getInode(inode->parent);

    // Remove indoe from parent
    removeFromParent(parentInode, inode);

    // Free inode
    freeInode(inode);

    return 0;
}	

int fs_stat(const char *path, struct fs_stat *buf) 
{
    // get inode for path
    fs_dir* inode = getInode(path);

    // if inode exists
    if (inode) {
        // set info for buf
        buf->st_size = inode->sizeInBytes;
        
        // st_blksize = getVCB()->blockSize
        buf->st_blksize = getVCB()->blockSize;
        
        // st_blocks = 2
        buf->st_blocks = 2;
        
        // Access and modification times
        buf->st_accesstime = inode->lastAccessTime;
        buf->st_modtime = inode->lastModificationTime;

        // buf->st_createtime = ?

        return 1;
    }

    return 0;
}