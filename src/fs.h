#ifndef FS_H
#define FS_H

#include <stdint.h>

#define ZAS_TAG 0x235A4153

#define PERM_R 0x4
#define PERM_W 0x2
#define PERM_X 0x1

#define PERM(kernel_rwx, user_rwx) (((kernel_rwx) << 3) | (user_rwx))

#define DIR 1
#define FILE 0

#define TYPE(Type) (Type << 7)

#define FS_BLOCK_SIZE 512ULL
#define FS_BLOCK_MASK (FS_BLOCK_SIZE - 1ULL)
#define FS_BLOCK_ALIGNED (~FS_BLOCK_MASK)

#define FS_SUPER_BLOCK_LBA 0
#define FS_SUPER_BLOCK_SECTORS 1
#define FS_FILE_TABLE_START_LBA 1
#define FS_FILE_TABLE_SECTORS 8ULL
#define FS_MAX_FILES ((FS_FILE_TABLE_SECTORS) * FS_BLOCK_SIZE) / sizeof(FILE_STATUS)
#define FS_MAX_DIRS
#define FS_DATA_START_LBA (FS_FILE_TABLE_START_LBA + FS_FILE_TABLE_SECTORS)

#define CREATE_DIR_RESERVE_SIZE 1
#define CREATE_FILE_RESERVE_SIZE 5

#define DIV_ROUND_UP(Data, Size) (((Data) + ((Size) - 1)) / (Size))

#pragma pack(push, 1)
typedef struct _SUPER_BLOCK
{
  uint32_t Tag;         // ZAS_TAG, A Identifier
  uint32_t NextFreeLBA; // Reduce Query Cost
  uint32_t ActiveFiles;
  uint32_t ActiveDirectoriesFileID; // save file register
  //uint32_t ActiveDirectoriesDirID;
  uint32_t BlockSize;
  uint32_t Reserver[512 - 20];
} SUPER_BLOCK, *PSUPER_BLOCK;

typedef struct _FILE_STATUS
{
  uint32_t FileID;    // 0 is NULL
  uint8_t Perm;
  uint8_t State;      // 0 : Abandon, 1 : Using
  uint16_t Reserved1;

  uint32_t Date;
  uint32_t StartLBA;
  uint32_t DesiredCapacity; // Reserved Block Count
  uint32_t UsedBlocks;      // To Save the Current Data's Tail
  uint32_t BytesOffset;     // (UsedBlocks - 1) * 512 + BytesOffset
  uint32_t Reserved2;
} FILE_STATUS, *PFILE_STATUS;

// Only support Max Length 23
typedef struct _DIRECTORY_INFORMATION
{
  char FileName[24];
  uint32_t FileID;
  uint32_t ParentID;
} DIRECTORY_INFORMATION, *PDIRECTORY_INFORMATION;

typedef struct _DIRECTORY_TREE
{
  uint32_t FileID;
  uint32_t ParentID;
} DIRECTORY_TREE, *PDIRECTORY_TREE;
#pragma pack(pop)

//==================================================
// FileID Must Meet Index + 1 of g_FileTable !!!!! |
//==================================================

// delete file or expand size
// need to update caller file id
// or mark deleted

/*
 * @brief Mount File System
 * @param void
 * @return success(0), fail(1)
*/
uint8_t fs_mount(void);

/*
 * @brief Format SSD
 * @return success(0), fail(1)
*/
uint8_t fs_format(void);

/*
 * @brief Create File And Return File ID
 * @param DesiredCapacity, Pre-Allocate Additional Space (block)
 * @param perm, rwx
 * @return fail(0), File ID
*/
uint32_t fs_create_file(uint32_t DesiredCapacity, uint8_t perm);

/*
 * @brief Write Data into File Use FileID
 * @param FileID
 * @param Buffer Which You Want To Write
 * @param Size, The Data Size
 * @return success(0), fail(1)
*/
uint8_t fs_write_file(uint32_t FileID, const void *Buffer, uint32_t Size);


/*
 * @brief Read Data from File Use FileID
 * @param FileID
 * @param Buffer, To Get Data
 * @param BufferSize, The Size Which You Want To Read
 * @param Offset(byte), Avoid Reading The Same Data
 * @return Read Data Size
*/
uint32_t fs_read_file(uint32_t FileID, void *Buffer, uint32_t BufferSize, uint32_t Offset);

void fs_test(void);

/*
 * @brief Create File Name To Seek
 * @param Name, The File Name
 * @param Capacity, The DesiredCapacity
 * @param Perm
*/
void fs_create_file_name(const char *Name, uint32_t Capacity, uint8_t Perm, uint8_t Type);

/*
 * @brief Via File Name Get File ID // enumerate
 * @param File Name
 * @return {ParentID, FileID}
*/
DIRECTORY_TREE fs_open(const char *Name);

char IsDirectory(uint32_t FileID);

uint32_t GetParentID(uint32_t CurrentID);

void list_line(void);

#endif
