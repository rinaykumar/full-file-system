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

}

// Linux chdir
int fs_setcwd(char *buf) 
{

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
