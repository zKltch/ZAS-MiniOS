#ifndef FS_H
#define FS_H

#include <stdint.h>

#define ZAS_TAG 0x235A4153

#define PERM_R 0x4
#define PERM_W 0x2
#define PERM_X 0x1

#define PERM(kernel_rwx, user_rwx) (((kernel_rwx) << 3) | (user_rwx))

#define FS_BLOCK_SIZE 512ULL
#define FS_BLOCK_MASK (FS_BLOCK_SIZE - 1ULL)
#define FS_BLOCK_ALIGNED (~FS_BLOCK_MASK)

#define FS_SUPER_BLOCK_LBA 0
#define FS_SUPER_BLOCK_SECTORS 1
#define FS_FILE_TABLE_START_LBA 1
#define FS_FILE_TABLE_SECTORS 8ULL
#define FS_MAX_FILES ((FS_FILE_TABLE_SECTORS) * FS_BLOCK_SIZE) / sizeof(FILE_STATUS)
#define FS_DATA_START_LBA (FS_FILE_TABLE_START_LBA + FS_FILE_TABLE_SECTORS)

#define DIV_ROUND_UP(Data, Size) (((Data) + ((Size) - 1)) / (Size))

#pragma pack(push, 1)
typedef struct _SUPER_BLOCK
{
  uint32_t Tag;         // ZAS_TAG, A Identifier
  uint32_t NextFreeLBA; // Reduce Query Cost
  uint32_t ActiveFiles;
  uint32_t BlockSize;
  uint8_t Reserved[512 - 16];
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
#pragma pack(pop)

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
*/
uint32_t fs_read_file(uint32_t FileID, void *Buffer, uint32_t BufferSize);

void fs_test(void);

#endif
