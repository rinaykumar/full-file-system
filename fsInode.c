/**************************************************************
* Class:  CSC-415-01 Fall 2020
* Name: Russell Azucenas
* Student ID: 917422693
* Project: Group Project - File System
*
* File: fsInode.c
*
* Description: Handles the Inode array. Holds information about each directory.
*
**************************************************************/
#include "fsInode.h"
#include "bitMap.h"

fs_dir* inodes;

char inodeTypeNames[3][64] = { "I_FILE", "I_DIR", "I_UNUSED" };

char* getInodeTypeName(char* buf, InodeType type)
{
    strcpy(buf, inodeTypeNames[type]);
    return buf;
}

fs_dir* createInode(InodeType type, const char* path)
{
    printf("--------------------------[createInode]---------------------------\n");

    fs_dir * inode;
    char parentPath[MAX_FILEPATH_SIZE];
    fs_dir* parentNode;

    /* Obtain current time. */
    time_t currentTime;
    currentTime = time(NULL);

    // call getFreeInode() to recieve the next available inode
    inode = getFreeInode();

    //find and assign the parent to the new inode
    getParentPath(parentPath, path);
    parentNode = getInode(parentPath);

    /* Set properties on inode. */
    inode->type = type;
    // Set inode name to file name
    // strcpy(inode->name , requestedFilePathArray[requestedFilePathArraySize - 1]);
    sprintf(inode->path, "%s/%s", parentPath, inode->name);
    inode->lastAccessTime = currentTime;
    inode->lastModificationTime = currentTime;

    /* Try to set the parent. If it fails, revert. */
    if (!setParent(parentNode, inode)) 
    {
        freeInode(inode);
        printf("Failed to set parent.\n");
        return NULL;
    }

    printf("Sucessfully created inode for path '%s'.\n", path);       
    return inode;
}

// Return first open inode in the array; return null if none found
fs_dir* getFreeInode()
{
    fs_dir* freeInode;

    for (size_t i = 0; i < getVCB()->totalInodes; i++) 
    {
        if (inodes[i].inUse == 0) 
        {
            inodes[i].inUse = 1;
            freeInode = &inodes[i];
            printf("Free inode found.\n");
            return freeInode;
        }
    }

    printf("No free inode found.\n");
    return NULL;
}

fs_dir* getInodeByIndex(int index) 
{
    if (index < getVCB()->totalInodes && index >= 0) 
    {
        return &inodes[index];
    } 
    else 
    {
        return NULL;
    }
}

int removeFromParent(fs_dir* parent, fs_dir* child) 
{
    /* Loop through parent's list of children until name match. */
    for (int i = 0; i < parent->numChildren; i++) 
    {
        if (!strcmp(parent->children[i], child->name)) 
        {

            /* Clear entry in parent's list of children. Decrement child count. */
            strcpy(parent->children[i], "");
            parent->numChildren--;
            parent->sizeInBlocks -= child->sizeInBlocks;
            parent->sizeInBytes -= child->sizeInBytes;
            return 1;
        }
    }

    printf("Could not find child '%s' in parent '%s'.\n", child->name, parent->path);
    return 0;

}

void writeInodes() 
{
    LBAwrite(inodes, getVCB()->totalInodeBlocks, getVCB()->inodeStartBlock);
}

int writeBufferToInode(fs_dir * inode, char* buffer, size_t bufSizeBytes, uint64_t blockNumber) 
{
    /* Check if dataBlockPointers is full. */
    int freeIndex = -1;
    for (int i = 0; i < MAX_DATABLOCK_POINTERS; i++) 
    {
        if (inode->directBlockPointers[i] == INVALID_DATABLOCK_POINTER) 
        {
            /* Record free dataBlockPointer index. */
            freeIndex = i;
            break;
        }
    }

    /* If there is no place to put the new dataBlock pointer. Return 0 blocks/bytes written. */
    if (freeIndex == -1) 
    {
        return 0;
    }

    /* Write buffered data to disk, update inode, write inodes to disk. */
    LBAwrite(buffer, 1, blockNumber);

    /* Record the block number in the inode, reserve the block in the freeMap and write the VCB. */
    inode->directBlockPointers[freeIndex] = blockNumber;
    setBit(getVCB()->freeMap, blockNumber);
    writeVCB();

    inode->numDirectBlockPointers++;
    inode->sizeInBlocks++;
    inode->sizeInBytes += bufSizeBytes;
    inode->lastAccessTime = time(0);
    inode->lastModificationTime = time(0);

    writeInodes();
    return 1;
}

void freeInode(fs_dir * node)
{
    printf("Freeing inode: '%s'\n", node->path);
    node->inUse = 0;
    node->type = I_UNUSED;
    node->name[0] = NULL;
    node->path[0] = NULL;
    node->parent[0] = NULL;
    node->sizeInBlocks = 0;
    node->sizeInBytes = 0;
    node->lastAccessTime = 0;
    node->lastModificationTime = 0;

    /* Free all data blocks from the file */
    if(node->type == I_FILE)
    {
        for (size_t i = 0; i < node->numDirectBlockPointers; i++) 
        {
            int blockPointer = node->directBlockPointers[i];
            clearBit(getVCB()->freeMap, blockPointer);
        }
    }

    /* Write inode updates to disk. */
    writeInodes();
}