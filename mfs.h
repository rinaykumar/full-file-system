/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: fsLow.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"
#include "fsVCB.h"
#include "fsInode.h"

#define MAX_FILEPATH_SIZE 225
#define	MAX_FILENAME_SIZE 20
#define MAX_DIRECTORY_DEPTH 10
#define MAX_NUMBER_OF_CHILDREN 64
#define MAX_DATABLOCK_POINTERS	64
#define INVALID_DATABLOCK_POINTER -1
#define INVALID_INODE_NAME	"unused_inode"

#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif


struct fs_dirEntry
{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	ino_t          d_ino;       /* inode number */
    off_t          d_off;       /* offset to the next directory entry*/
};

typedef enum { I_FILE, I_DIR, I_UNUSED } InodeType;
char* getInodeTypeName(char* buf, InodeType type);

typedef struct
{
	char name[MAX_FILENAME_SIZE];  // File name
	char path[MAX_FILEPATH_SIZE];  // File path

	int inUse;
	uint64_t inodeIndex;
	InodeType type;

	char parent[MAX_FILEPATH_SIZE]; // Path name to parent
	char children[MAX_NUMBER_OF_CHILDREN][MAX_FILENAME_SIZE]; // Array of children names
	int numChildren; // Number of children in a directory

	time_t lastAccessTime;
	time_t lastModificationTime;
	blkcnt_t sizeInBlocks; // 512 for each block
	off_t sizeInBytes; // File size
	int directBlockPointers[MAX_DATABLOCK_POINTERS]; // Array of pointers to data blocks
	int numDirectBlockPointers;
} fs_dir;


int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fs_dir* fs_opendir(const char *name);
struct fs_dirEntry *fs_readdir(fs_dir *dirp);
int fs_closedir(fs_dir *dirp);

char* fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);   //linux chdir
int fs_isFile(char * path);	//return 1 if file, 0 otherwise
int fs_isDir(char * path);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

/* Other functions */
void fs_init();
void fs_close();

struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	dev_t     st_dev;     /* ID of device containing file */
	ino_t     st_ino;     /* inode index number */
	mode_t    st_mode;    /* protection mode */
	nlink_t   st_nlink;   /* number of hard links */
	uid_t     st_uid;     /* owner's user ID */
	gid_t     st_gid;     /* owner's group ID */
	dev_t     st_rdev;    /* device ID (if special file) */
	};

int fs_stat(const char *path, struct fs_stat *buf);

#endif

