#include "fs.h"
#include "kalloc.h"
#include "vmm.h"
#include "nvme.h"
#include "console.h"
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
  g_SuperBlock.ActiveDirectoriesFileID = 0;
  //g_SuperBlock.ActiveDirectoriesDirID = 0;
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

  for(uint32_t i = 0; i < FS_MAX_FILES; i++)
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

uint8_t fs_write_file_offset(uint32_t FileID, const void *Buffer, uint32_t Size, uint32_t Offset)
{
  if(!FileID || FileID > FS_MAX_FILES || !Buffer || !Size) return 1;

  uint32_t Index = FileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 1;

  if(!g_FileTable[Index].UsedBlocks) return 1;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  if(Offset + Size > TotalSize) return 1;

  void *DMA_Buffer = kalloc();
  char *Data_Buffer = (char*)Buffer;

  uint32_t write_size = Size;
  uint32_t current_offset = Offset;

  while(write_size > 0)
  {
    uint32_t block_index = current_offset / FS_BLOCK_SIZE;
    uint32_t block_offset = current_offset % FS_BLOCK_SIZE;
    uint32_t current_lba = g_FileTable[Index].StartLBA + block_index;

    memset(DMA_Buffer, 0, PGSIZE);
    if(NvmeRead(current_lba, 1, v2p(DMA_Buffer)))
    {
      kfree((char*)DMA_Buffer);
      return 1;
    }

    uint32_t write_bytes = (write_size > FS_BLOCK_SIZE - block_offset) ? FS_BLOCK_SIZE - block_offset : write_size;

    memcpy((char*)DMA_Buffer + block_offset, Data_Buffer, write_bytes);

    if(NvmeWrite(current_lba, 1, v2p(DMA_Buffer)))
    {
      kfree((char*)DMA_Buffer);
      return 1;
    }

    write_size -= write_bytes;
    current_offset += write_bytes;
    Data_Buffer += write_bytes;
  }

  kfree((char*)DMA_Buffer);
  return 0;
}

uint32_t fs_read_file(uint32_t FileID, void *Buffer, uint32_t BufferSize, uint32_t Offset)
{
  if(!FileID || FileID > FS_MAX_FILES || !Buffer || !BufferSize) return 0;

  uint32_t Index = FileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 0;

  if(!g_FileTable[Index].UsedBlocks) return 0;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  if(Offset >= TotalSize) return 0;
  
  uint32_t read_buffer = (TotalSize > (Offset + BufferSize)) ? BufferSize : (TotalSize - Offset);
  uint32_t read_size = read_buffer;

  char *DMA_Buffer = kalloc();
  char *dest = (char*)Buffer;

  uint32_t block_init_offset = Offset / FS_BLOCK_SIZE;
  uint32_t block_bytes_offset = Offset % FS_BLOCK_SIZE;
  uint64_t target_lba = g_FileTable[Index].StartLBA + block_init_offset;

  while(read_buffer > 0)
  {
    uint32_t read_bytes = (read_buffer > PGSIZE - block_bytes_offset) ? PGSIZE : read_buffer;
    uint32_t read_sectors = DIV_ROUND_UP(read_bytes, FS_BLOCK_SIZE);

    memset(DMA_Buffer, 0, PGSIZE);
    if(NvmeRead(target_lba, read_sectors, v2p(DMA_Buffer)))
    {
      kfree(DMA_Buffer);
      return 0;
    }

    memcpy(dest, DMA_Buffer + block_bytes_offset, read_bytes);

    target_lba += read_sectors;
    dest += read_bytes;
    read_buffer -= read_bytes;
    block_bytes_offset = 0;
  }

  kfree((char*)DMA_Buffer);
  return read_size;
}

void fs_test(void)
{
  if(fs_mount())
  {
    puts("ERROR : FileSystem Mount Failed!\n");
    return;
  }
  //uint32_t file_id = fs_create_file(10, PERM(7, 7));
  DIRECTORY_TREE dir_tree = fs_open("Hello.txt");
  if(!dir_tree.FileID)
  {
    fs_create_file_name("Hello.txt", 10, PERM(7, 7), FILE);
    dir_tree = fs_open("Hello.txt");
    if(!dir_tree.FileID)
    {
      puts("ERROR : Can Not Find File!\n");
      return;
    }
  }
  puts("File ID : ");
  puts_hex(dir_tree.FileID);
  puts("\n");

  char msg1[] = "Hello FileSystem!";
  uint32_t length1 = sizeof(msg1) - 1;
  if(fs_write_file(dir_tree.FileID, msg1, length1))
  {
    puts("ERROR : Write File Failed\n");
    return;
  }

  char msg2[] = " >>> Appended";
  uint32_t length2 = sizeof(msg2) - 1;
  if(fs_write_file(dir_tree.FileID, msg2, length2))
  {
    puts("ERROR : Write File Failed\n");
    return;
  }

  char read_buf[512];
  memset(read_buf, 0, sizeof(read_buf));

  uint32_t read_size = fs_read_file(dir_tree.FileID, read_buf, sizeof(read_buf) - 1, 0);
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

void fs_create_file_name(const char *Name, uint32_t Capacity, uint8_t Perm, uint8_t Type)
{
  if(g_SuperBlock.ActiveDirectoriesFileID == 0)
  {
    g_SuperBlock.ActiveDirectoriesFileID = 
	    fs_create_file((FS_MAX_FILES * sizeof(DIRECTORY_INFORMATION) / FS_BLOCK_SIZE), PERM(7, 5));
    NvmeWrite(FS_SUPER_BLOCK_LBA, FS_SUPER_BLOCK_SECTORS, v2p(&g_SuperBlock));
  }
  /*if(g_SuperBlock.ActiveDirectoriesDirID == 0)
  {
    g_SuperBlock.ActiveDirectoriesDirID = 
	    fs_create_file((FS_MAX_DIRS * sizeof(DIRECTORY_INFORMATION) / FS_BLOCK_SIZE), PERM(7, 5));
    NvmeWrite(FS_SUPER_BLOCK_LBA, FS_SUPER_BLOCK_SECTORS, v2p(&g_SuperBlock));
  }*/

  // avoid the same file name
  DIRECTORY_TREE dir_tree = fs_open(Name);
  if(dir_tree.FileID != 0) return;

  uint32_t FileID = fs_create_file(Capacity, TYPE(Type) | Perm);
  if(!FileID) return;

  DIRECTORY_INFORMATION Entry = { 0 };
  Entry.ParentID = CurrentID;
  Entry.FileID = FileID;
  for(uint32_t i = 0; i < 23 && Name[i] != '\0'; i++) Entry.FileName[i] = Name[i];

  //Type ? fs_write_file(g_SuperBlock.ActiveDirectoriesDirID, &Entry, sizeof(DIRECTORY_INFORMATION)) :
    fs_write_file(g_SuperBlock.ActiveDirectoriesFileID, &Entry, sizeof(DIRECTORY_INFORMATION));
  
  return;
}

// The Code Block Below Has Plenty Of Repetitive Logic
// The Individual Differences Make It A Poor Candidate for Refactoring

DIRECTORY_TREE fs_open(const char *Name)
{
  DIRECTORY_TREE tree_data = { 0 };

  if(g_SuperBlock.ActiveDirectoriesFileID == 0) return tree_data;

  uint8_t Index = g_SuperBlock.ActiveDirectoriesFileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return tree_data;
  if(g_FileTable[Index].UsedBlocks == 0) return tree_data;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  uint32_t read_size = TotalSize;
  uint32_t entries = TotalSize / sizeof(DIRECTORY_INFORMATION);
  if(entries == 0) return tree_data;

  char *buffer = kalloc();
  PDIRECTORY_INFORMATION dir = (PDIRECTORY_INFORMATION)buffer;

  while(read_size > 0)
  {
    uint32_t read_bytes = (read_size > PGSIZE) ? PGSIZE : read_size;
  
    if(fs_read_file(g_SuperBlock.ActiveDirectoriesFileID, buffer, read_bytes, TotalSize - read_size) == 0) break;
    
    for(uint32_t i = 0; i < read_bytes / sizeof(DIRECTORY_INFORMATION); i++)
    {
      if(dir[i].FileID != 0 && dir[i].ParentID == CurrentID)
      {
        uint8_t isMatch = 1;
        for(uint32_t j = 0; Name[j] != '\0' || dir[i].FileName[j] != '\0'; j++)
        {
          if(Name[j] != dir[i].FileName[j])
          {
            isMatch = 0;
            break;
          }
        }
        if(isMatch)
        {
	  tree_data.FileID = dir[i].FileID;
	  tree_data.ParentID = CurrentID;
	  tree_data.FileOffset = (TotalSize - read_size) + i * sizeof(DIRECTORY_INFORMATION);
	  kfree(buffer);
          return tree_data;
        }
      }
    }
    read_size -= read_bytes;
  }

  kfree(buffer);
  return tree_data;
}

char IsDirectory(uint32_t FileID)
{
  if(FileID == 0 || !g_FileTable[FileID - 1].State) return -1;

  return g_FileTable[FileID - 1].Perm >> 7;
}

uint32_t GetParentID(uint32_t CurrentID)
{
  if (CurrentID == 0) return 0;
  if (g_SuperBlock.ActiveDirectoriesFileID == 0) return 0;
  
  uint32_t Index = g_SuperBlock.ActiveDirectoriesFileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 0;

  if(!g_FileTable[Index].UsedBlocks) return 0;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  if(TotalSize < sizeof(DIRECTORY_INFORMATION)) return 0;

  char *buffer = kalloc();

  PDIRECTORY_INFORMATION dir = (PDIRECTORY_INFORMATION)buffer;
  uint32_t read_size = TotalSize;
  uint32_t parent_id = 0;

  while(read_size > 0)
  {
    uint32_t read_bytes = (read_size > PGSIZE) ? PGSIZE : read_size;
    uint32_t offset = TotalSize - read_size;

    if(fs_read_file(g_SuperBlock.ActiveDirectoriesFileID, buffer, read_bytes, offset) == 0) break;

    uint32_t entries = read_bytes / sizeof(DIRECTORY_INFORMATION);
    for(uint32_t i = 0; i < entries; i++)
    {
      if(dir[i].FileID == CurrentID)
      {
        parent_id = dir[i].ParentID;
	break;
      }
    }

    if(parent_id != 0) break;
    read_size -= read_bytes;
  }
  kfree(buffer);
  return parent_id;
}

void list_file(void)
{
  if (g_SuperBlock.ActiveDirectoriesFileID == 0) return;
  
  uint32_t Index = g_SuperBlock.ActiveDirectoriesFileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return;

  if(!g_FileTable[Index].UsedBlocks) return;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  if(TotalSize < sizeof(DIRECTORY_INFORMATION)) return;

  char *buffer = kalloc();
  PDIRECTORY_INFORMATION dir = (PDIRECTORY_INFORMATION)buffer;
  uint32_t read_size = TotalSize;

  while(read_size > 0)
  {
    uint32_t read_bytes = (read_size > PGSIZE) ? PGSIZE : read_size;
    uint32_t offset = TotalSize - read_size;

    if(fs_read_file(g_SuperBlock.ActiveDirectoriesFileID, buffer, read_bytes, offset) == 0) break;

    uint32_t entries = read_bytes / sizeof(DIRECTORY_INFORMATION);
    for(uint32_t i = 0; i < entries; i++)
    {
      if(dir[i].FileID != 0 && dir[i].ParentID == CurrentID)
      {
        puts(dir[i].FileName);
	if(IsDirectory(dir[i].FileID)) puts("/");
	puts("  ");
      }
    }
    read_size -= read_bytes;
  }
  puts("\n");
  kfree(buffer);
  return;
}

uint8_t IsDirectoryEmpty(uint32_t DirID)
{
  if (g_SuperBlock.ActiveDirectoriesFileID == 0) return 1;
  
  uint32_t Index = g_SuperBlock.ActiveDirectoriesFileID - 1;
  if(!g_FileTable[Index].FileID || !g_FileTable[Index].State) return 1;

  if(!g_FileTable[Index].UsedBlocks) return 1;

  uint32_t TotalSize = (g_FileTable[Index].UsedBlocks - 1) * FS_BLOCK_SIZE + g_FileTable[Index].BytesOffset;
  if(TotalSize < sizeof(DIRECTORY_INFORMATION)) return 1;

  char *buffer = kalloc();
  PDIRECTORY_INFORMATION dir = (PDIRECTORY_INFORMATION)buffer;
  uint32_t read_size = TotalSize;
  uint8_t IsEmpty = 1;

  while(read_size > 0)
  {
    uint32_t read_bytes = (read_size > PGSIZE) ? PGSIZE : read_size;
    uint32_t offset = TotalSize - read_size;

    if(fs_read_file(g_SuperBlock.ActiveDirectoriesFileID, buffer, read_bytes, offset) == 0) break;

    uint32_t entries = read_bytes / sizeof(DIRECTORY_INFORMATION);
    for(uint32_t i = 0; i < entries; i++)
    {
      if(dir[i].FileID != 0 && dir[i].ParentID == DirID)
      {
        IsEmpty = 0;
	break;
      }
    }
    if(!IsEmpty) break;
    read_size -= read_bytes;
  }

  kfree(buffer);
  return IsEmpty;
}

void fs_init_dir_single_file(uint32_t offset)
{
  DIRECTORY_INFORMATION FormatName = { 0 };
  fs_write_file_offset(g_SuperBlock.ActiveDirectoriesFileID, &FormatName, sizeof(DIRECTORY_INFORMATION), offset);
  return ;
}

void fs_invalidate_file_table(uint32_t FileID)
{
  g_FileTable[FileID - 1].State = 0;
  NvmeWrite(FS_FILE_TABLE_START_LBA, FS_FILE_TABLE_SECTORS, v2p(g_FileTable));
  return;
}
