#include "fsVCB.h"

char header[16] = "[File System]";
uint64_t volumeSize;
uint64_t blockSize;
uint64_t diskSizeBlocks;
uint32_t vcbStartBlock;
uint32_t totalVCBBlocks;
uint32_t inodeStartBlock;
uint32_t totalInodes;
uint32_t totalInodeBlocks;
uint32_t freeMapSize;

int initialized = 0;
fs_VCB* openVCB;

void initialize(uint64_t _volumeSize, uint64_t _blockSize) 
{
    volumeSize = _volumeSize;
    blockSize = _blockSize;
    diskSizeBlocks = ceilDiv(volumeSize, blockSize);
    freeMapSize = diskSizeBlocks <= sizeof(uint32_t) * 8 ? 1 : diskSizeBlocks / sizeof(uint32_t) / 8;
    totalVCBBlocks = ceilDiv(sizeof(fs_VCB) + sizeof(uint32_t[freeMapSize]), blockSize);
    inodeStartBlock = VCB_START_BLOCK + totalVCBBlocks;
    totalInodes = (diskSizeBlocks - inodeStartBlock) / (DATA_BLOCKS_PER_INODE + ceilDiv(sizeof(fs_dir), blockSize));
    totalInodeBlocks = ceilDiv(totalInodes * sizeof(fs_dir), blockSize);

    /* Allocate memory for the VCB. */
    int vcbSize = allocateVCB(&openVCB);
    printf("Allocated %d blocks for VCB.\n", vcbSize);

    initialized = 1;
}

int allocateVCB(fs_VCB** vcb) 
{
    *vcb = calloc(totalVCBBlocks, blockSize);
    return totalVCBBlocks;
}

void initializeVCB() 
{
    if (!initialized) 
    {
        printf("VCB system not initialized.\n");
        return;
    }

    sprintf(openVCB->header, "%s", header); 

    /* Set information on volume sizes and block locations. */
    openVCB->volumeSize = volumeSize;
    openVCB->blockSize = blockSize;
    openVCB->diskSizeBlocks = diskSizeBlocks;
    openVCB->vcbStartBlock = VCB_START_BLOCK;
    openVCB->totalVCBBlocks = totalVCBBlocks;
    openVCB->inodeStartBlock = inodeStartBlock;
    openVCB->totalInodes = totalInodes;
    openVCB->totalInodeBlocks = totalInodeBlocks;
    printf("initVCB: totalInodeBlocks %ld", openVCB->totalInodeBlocks);

    openVCB->freeMapSize = freeMapSize;

    /* Initialize freeBlockMap to all 0's. */
    for (int i=0; i<freeMapSize; i++) 
    {
        openVCB->freeMap[i] = 0;
    }

    /* Set bits in freeMap for VCB and inodes. */
    for (int i=0; i<inodeStartBlock+totalInodeBlocks; i++) 
    {
        setBit(openVCB->freeMap, i);
    }

    printVCB();
    writeVCB();
}

fs_VCB* getVCB() 
{
    return openVCB;
}

uint64_t readVCB()
{
    if (!initialized) 
    {
        printf("VCB system not initialized.\n");
        return 0;
    }

    /* Read VCB from disk to openVCB */
    uint64_t blocksRead = LBAread(openVCB, totalVCBBlocks, VCB_START_BLOCK);
    printf("Read VCB in %d blocks starting at block %d.\n", totalVCBBlocks, VCB_START_BLOCK);
    return blocksRead;
}

uint64_t writeVCB() 
{
    if (!initialized) 
    {
        printf("writeVCB: System not initialized.\n");
        return 0;
    }

    /* Write openVCB_p to disk. */
    uint64_t blocksWritten = LBAwrite(openVCB, totalVCBBlocks, VCB_START_BLOCK);
    printf("Wrote VCB in %d blocks starting at block %d.\n", totalVCBBlocks, VCB_START_BLOCK);
    return blocksWritten;
}

uint64_t getFreeBlock()
{
    for (int index = 0; index < diskSizeBlocks; index++)
    {
        if (findBit(openVCB->freeMap, index) == 0) 
        {
            return index; //The position in the VolumeSpaceArray
        }
    }
    return -1;
}

void printVCB() 
{
    int size = openVCB->totalVCBBlocks * (openVCB->blockSize);
    int width = 16;
    char* char_p = (char*)openVCB;
    char ascii[width+1];
    sprintf(ascii, "%s", "................");
    for (int i = 0; i<size; i++) 
    {
        printf("%02x ", char_p[i] & 0xff);
        if (char_p[i]) 
        {
            ascii[i%width] = char_p[i];
        }
        if ((i+1)%width==0&&i>0) 
        {
            ascii[i%width+1] = '\0';
            printf("%s\n", ascii);
            sprintf(ascii, "%s", "................");
        } 
        else if (i==size-1) 
        {
            for (int j=0; j<width-(i%(width-1)); j++) 
            {
                printf("   ");
            }
            ascii[i%width+1] = '\0';
            printf("%s\n", ascii);
            sprintf(ascii, "%s", "................");
        }
    }
    printf("VCB Size: %d bytes\n", size);
}

int createVolume(char* volumeName, uint64_t _volumeSize, uint64_t _blockSize) 
{
    /* Check whether volume exists already. */
    if (access(volumeName, F_OK) != -1) 
    {
        printf("Cannot create volume '%s'. Volume already exists.\n", volumeName);
        return -3;
    }

    uint64_t existingVolumeSize = _volumeSize;
    uint64_t existingBlockSize = _blockSize;

    /* Initialize the volume with the fsLow library. */
    int retVal = startPartitionSystem (volumeName, &existingVolumeSize, &existingBlockSize);

    printf("Opened %s, Volume Size: %llu;  BlockSize: %llu; Return %d\n", volumeName, (ull_t)existingVolumeSize, (ull_t)existingBlockSize, retVal);

    /* Format the disk if the volume was opened properly. */
    if (!retVal) 
    {
        init(_volumeSize, _blockSize);
        initializeVCB();
        initializeInodes();
    }

    closeVolume();
    return retVal;
}

void openVolume(char* volumeName) 
{
    if (!initialized) 
    {
        uint64_t volumeSize;
        uint64_t blockSize;

        int retVal =  startPartitionSystem(volumeName, &volumeSize, &blockSize);
        if (!retVal) 
        {
            init(volumeSize, blockSize);
            readVCB();
            printVCB();
        }
    }
    else 
    {
        printf("Failed to open volume '%s'. Another volume is already open.\n", volumeName);
    }
}

void closeVolume() 
{
    if (initialized) 
    {
        closePartitionSystem();
        free(openVCB);
        initialized = 0;
    } 
    else 
    {
        printf("Can't close volume. Volume not open.\n");
    }
}

