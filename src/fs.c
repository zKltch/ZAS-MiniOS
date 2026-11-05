#include "fs.h"
#include "kalloc.h"
#include "vmm.h"
#include "nvme.h"
#include "puts.h"

static SUPER_BLOCK g_SuperBlock;
static PFILE_STATUS g_FileTable = 0;

uint8_t fs_mount(void)
{
  if(!g_FileTable) g_FileTable = (PFILE_STATUS)kalloc();

  if(NvmeRead(FS_SUPER_BLOCK_LBA, FS_SUPER_BLOCK_SECTORS, v2p(&g_SuperBlock))) return 1;

  if(g_SuperBlock.Tag != ZAS_TAG) return fs_format();

  if(NvmeRead(FS_FILE_TABLE_START_LBA, FS_FILE_TABLE_SECTORS, v2p(g_FileTable))) return 1;

  return 0;
}

uint8_t fs_format(void)
{
  memset(&g_SuperBlock, 0, sizeof(SUPER_BLOCK));

  g_SuperBlock.Tag = ZAS_TAG;
  g_SuperBlock.NextFreeLBA = FS_DATA_START_LBA;
  g_SuperBlock.ActiveFiles = 0;
  g_SuperBlock.BlockSize = FS_BLOCK_SIZE;

  if(NvmeWrite(FS_SUPER_BLOCK_LBA, FS_SUPER_BLOCK_SECTORS, v2p(&g_SuperBlock))) return 1;

  for(uint8_t i = 0; i < ((FS_FILE_TABLE_SECTORS * FS_BLOCK_SIZE) - 1) / PGSIZE; i++) kalloc();
  memset(g_FileTable, 0, FS_BLOCK_SIZE * FS_FILE_TABLE_SECTORS);

  if(NvmeWrite(FS_FILE_TABLE_START_LBA, FS_FILE_TABLE_SECTORS, v2p(g_FileTable))) return 1;

  return 0;
}

uint32_t fs_create_file(uint32_t DesiredCapacity, uint8_t perm)
{
  uint32_t Index = ~0;

  if(!DesiredCapacity) return 0;

  for(uint32_t i = 0; i < (FS_BLOCK_SIZE * FS_FILE_TABLE_SECTORS) / sizeof(FILE_STATUS); i++)
  {
    if(!g_FileTable[i].FileID || !g_FileTable[i].State)
    {
      Index = i;
      break;
    }
  }

  if(Index == ~0) return 0;

  g_FileTable[Index].FileID = Index + 1;
  g_FileTable[Index].State = 1;
  g_FileTable[Index].Perm = perm;
  // g_FileTable[Index].Date = ;
  g_FileTable[Index].StartLBA = g_SuperBlock.NextFreeLBA;
  g_FileTable[Index].DesiredCapacity = DesiredCapacity;
  g_FileTable[Index].UsedBlocks = 0;
  g_FileTable[Index].BytesOffset = 0;

  g_SuperBlock.NextFreeLBA += DesiredCapacity;
  g_SuperBlock.ActiveFiles++;

  if(NvmeWrite(FS_SUPER_BLOCK_LBA, FS_SUPER_BLOCK_SECTORS, v2p(&g_SuperBlock))) return 0;

  void *aligned_buffer = (void*)((uintptr_t)&g_FileTable[Index] & FS_BLOCK_ALIGNED);
  uint64_t target_lba = FS_FILE_TABLE_START_LBA + ((sizeof(FILE_STATUS) * Index) / FS_BLOCK_SIZE);
  if(NvmeWrite(target_lba, 1, v2p(aligned_buffer))) return 0;

  return g_FileTable[Index].FileID;
}

uint8_t fs_write_file(uint32_t FileID, const void *Buffer, uint32_t Size)
{
  if(!FileID || FileID > FS_MAX_FILES || !Size || !Buffer) return 1;

  uint32_t Index = FileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 1;

  uint32_t TotalSize = g_FileTable[Index].UsedBlocks ?
	  (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset + Size :
	  Size;

  uint32_t needed_blocks = DIV_ROUND_UP(TotalSize, FS_BLOCK_SIZE);
  if(g_FileTable[Index].DesiredCapacity < needed_blocks) return 1;
  // return the pointer of fs_remapped(capacity) 

  void *DMA_Buffer = kalloc();
  char *Data_Buffer = (char*)Buffer;

  uint64_t target_lba = g_FileTable[Index].StartLBA;
  if(g_FileTable[Index].UsedBlocks > 0)
  {
    if(g_FileTable[Index].BytesOffset == FS_BLOCK_SIZE)
    {
      target_lba += g_FileTable[Index].UsedBlocks;
    }
    else
    {
      target_lba += g_FileTable[Index].UsedBlocks - 1;

      memset(DMA_Buffer, 0, PGSIZE);
      if(NvmeRead(target_lba, 1, v2p(DMA_Buffer)))
      {
        kfree((char*)DMA_Buffer);
	return 1;
      }
      uint32_t DataSize = (Size > FS_BLOCK_SIZE - g_FileTable[Index].BytesOffset) ?
	                   FS_BLOCK_SIZE - g_FileTable[Index].BytesOffset :
			   Size;
      memcpy((char*)DMA_Buffer + g_FileTable[Index].BytesOffset, Data_Buffer, DataSize);
      if(NvmeWrite(target_lba, 1, v2p(DMA_Buffer)))
      {
        kfree((char*)DMA_Buffer);
	return 1;
      }

      target_lba += 1;
      Data_Buffer += DataSize;
      Size -= DataSize;
    }
  }

  while(Size > 0)
  {
    uint32_t write_bytes = (Size > PGSIZE) ? PGSIZE : Size;
    memset(DMA_Buffer, 0, PGSIZE);
    memcpy(DMA_Buffer, Data_Buffer, write_bytes);

    if(NvmeWrite(target_lba, DIV_ROUND_UP(write_bytes, FS_BLOCK_SIZE), v2p(DMA_Buffer)))
    {
      kfree((char*)DMA_Buffer);
      return 1;
    }

    target_lba += DIV_ROUND_UP(write_bytes, FS_BLOCK_SIZE);
    Data_Buffer += write_bytes;
    Size -= write_bytes;
  }

  kfree((char*)DMA_Buffer);

  g_FileTable[Index].UsedBlocks = needed_blocks;
  g_FileTable[Index].BytesOffset = TotalSize % FS_BLOCK_SIZE;
  if(g_FileTable[Index].BytesOffset == 0) g_FileTable[Index].BytesOffset = FS_BLOCK_SIZE;

  void *aligned_buffer = (void*)((uintptr_t)&g_FileTable[Index] & FS_BLOCK_ALIGNED);
  uint64_t fs_table_lba = FS_FILE_TABLE_START_LBA + (sizeof(FILE_STATUS) * Index) / FS_BLOCK_SIZE;
  if(NvmeWrite(fs_table_lba, 1, v2p(aligned_buffer))) return 1;

  return 0;
}

uint32_t fs_read_file(uint32_t FileID, void *Buffer, uint32_t BufferSize)
{
  if(!FileID || FileID > FS_MAX_FILES || !Buffer || !BufferSize) return 0;

  uint32_t Index = FileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 0;

  if(!g_FileTable[Index].UsedBlocks) return 0;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  uint32_t read_buffer = (TotalSize > BufferSize) ? BufferSize : TotalSize;
  uint32_t read_size = read_buffer;

  void *DMA_Buffer = kalloc();
  char *dest = (char*)Buffer;
  uint64_t target_lba = g_FileTable[Index].StartLBA;

  while(read_buffer > 0)
  {
    uint32_t read_bytes = (read_buffer > PGSIZE) ? PGSIZE : read_buffer;
    uint32_t read_sectors = DIV_ROUND_UP(read_bytes, FS_BLOCK_SIZE);

    memset(DMA_Buffer, 0, PGSIZE);
    if(NvmeRead(target_lba, read_sectors, v2p(DMA_Buffer)))
    {
      kfree((char*)DMA_Buffer);
      return 0;
    }

    memcpy(dest, DMA_Buffer, read_bytes);

    target_lba += read_sectors;
    dest += read_bytes;
    read_buffer -= read_bytes;
  }

  kfree((char*)DMA_Buffer);
  return read_size;
}

void fs_test(void)
{
  if(fs_mount()) puts("ERROR : FileSystem Mount Failed!\n");

  uint32_t file_id = fs_create_file(10, PERM(7, 7));
  if(file_id == 0)
  {
    puts("ERROR : Create File Failed!\n");
    return;
  }
  puts("File ID : ");
  puts_hex(file_id);
  puts("\n");

  char msg1[] = "Hello FileSystem!";
  uint32_t length1 = sizeof(msg1) - 1;
  if(fs_write_file(file_id, msg1, length1))
  {
    puts("ERROR : Write File Failed\n");
    return;
  }

  char msg2[] = " >>> Appended";
  uint32_t length2 = sizeof(msg2) - 1;
  if(fs_write_file(file_id, msg2, length2))
  {
    puts("ERROR : Write File Failed\n");
    return;
  }

  char read_buf[512];
  memset(read_buf, 0, sizeof(read_buf));

  uint32_t read_size = fs_read_file(file_id, read_buf, sizeof(read_buf) - 1);
  if(read_size == 0)
  {
    puts("ERROR : Read File Failed\n");
    return;
  }

  read_buf[read_size] = '\0';
  puts("Data : ");
  puts(read_buf);
  puts("\n");
}
