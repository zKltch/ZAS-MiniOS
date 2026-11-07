#include "script_engine.h"
#include "kalloc.h"
#include "fs.h"
#include "console.h"
#include "puts.h"

static inline void read_space(const char **s)
{
  while(**s == ' ')
  {
    (*s)++;
  }
}

static inline void filter_args(char *dst, const char *src, uint32_t *len)
{
  while(src[*len] != '\n' && src[*len] != '\r' && src[*len] != '\0')
  {
    dst[*len] = src[*len];
    (*len)++;
  }
  dst[*len] = '\0';
}

void cat_file(const char *FileName)
{
  read_space(&FileName);

  char file_name[24];
  uint32_t len = 0;

  filter_args(file_name, FileName, &len);

  if(len == 0)
  {
    puts("help : cat <filename>\n");
    return;
  }

  DIRECTORY_TREE dir_tree = fs_open(file_name);
  uint32_t FileID = dir_tree.FileID;
  if(!FileID)
  {
    puts("No Such File >> ");
    puts(file_name);
    puts("\n");
    return;
  }

  char IsDir = IsDirectory(FileID);
  switch (IsDir)
  {
    case -1 :
      puts("No Such File >> "); puts(file_name); puts("\n");
      return;
    case 1 :
      puts("ERROR : Invalid File Type!\n");
      return;
  }

  char *buf = kalloc();
  uint32_t offset = 0;

  while(1)
  {
    uint32_t read_size = fs_read_file(FileID, buf, PGSIZE - 1, offset);
    if(read_size == 0) break;

    buf[read_size] = '\0';
    puts(buf);
    offset += read_size;
  }
  puts("\n");
  kfree(buf);
  return;
}

void echo_write_file(const char *args)
{
  read_space(&args);

  uint32_t last_redirect = 0;
  const char *redirect = 0;
  for(const char *p = args; *p != '\0'; p++)
  {
    last_redirect++;
  }
  for(const char *p = &args[last_redirect]; p != args; p--)
  {
    if(*p == '>')
    {
      redirect = p;
      break;
    }
  }

  if(!redirect)
  {
    (*args == '\n') ? puts("help : echo [data] / echo [data] > [filename]\n") : puts(args);
    return;
  }

  char content[256];
  char filename[24];
  uint32_t file_len = 0;

  uint32_t content_len = redirect - args;

  while(content_len > 0 && args[content_len - 1] == ' ') content_len--;
  //memcpy(content, args, content_len);
  for(uint32_t i = 0; i < content_len; i++)
  {
    content[i] = args[i];
  }
  content[content_len] = '\0';

  const char *file_name = redirect + 1;
  read_space(&file_name);

  filter_args(filename, file_name, &file_len);

  if(file_len == 0)
  {
    puts("help : echo [data] > [filename]\n");
    return ;
  }

  DIRECTORY_TREE dir_tree = fs_open(filename);
  uint32_t file_id = dir_tree.FileID;
  if(!file_id)
  {
    fs_create_file_name(filename, CREATE_FILE_RESERVE_SIZE, PERM(7, 7), 0);
    dir_tree = fs_open(filename);
    file_id = dir_tree.FileID;
    if(!file_id)
    {
      puts("ERROR : Failed To Create File ["); puts(filename); puts("]\n");
      return;
    }
  }

  char IsDir = IsDirectory(file_id);
  switch (IsDir)
  {
    case -1 :
      puts("No Such File >> "); puts(filename); puts("\n");
      return;
    case 1 :
      puts("ERROR : Invalid File Type!\n");
      return;
  }

  content[content_len++] = '\n';
  if(fs_write_file(file_id, content, content_len))
  {
    puts("ERROR : Failed to Write ["); puts(filename); puts("]\n");
    return;
  }
  puts("Update ["); puts(filename); puts("] Successfully!\n");
}

void cmd_script(const char *script_name)
{
  read_space(&script_name);
  if(*script_name == '\0')
  {
    puts("help : sh <script.sh>\n");
    return;
  }

  char file_name[24];
  uint32_t length = 0;

  filter_args(file_name, script_name, &length);

  DIRECTORY_TREE dir_tree = fs_open(file_name);
  uint32_t file_id = dir_tree.FileID;
  if(!file_id)
  {
    puts("No Such File >> "); puts(file_name); puts("\n");
    return;
  }

  char IsDir = IsDirectory(file_id);
  switch (IsDir)
  {
    case -1 :
      puts("No Such File >> "); puts(file_name); puts("\n");
      return;
    case 1 :
      puts("ERROR : Invalid File Type!\n");
      return;
  }

  puts("===Run "); puts(file_name); puts("===\n");

  char *script_buffer = kalloc();
  uint32_t read_offset = 0;
  uint32_t script_size = 0;
  char line[256];
  uint32_t num = 0;
  while((script_size = fs_read_file(file_id, script_buffer, PGSIZE - 1, read_offset)) > 0)
  {
    for(uint32_t i = 0; i < script_size; i++)
    {
      char c = script_buffer[i];

      if(c != '\n' && c != '\r' && c != '\0')
      {
        if(num < sizeof(line) - 1)
	{
	  line[num++] = c;
	}
      }else
      {
        if(num != 0)
	{
	  line[num] = '\0';

	  char *s = line;
	  read_space(&s);
	  if(*s != '#' && *s != '\0')
	  {
	    execute_command(s);
	  }
	  num = 0;
	}
      }
    }
    read_offset += script_size;
  }

  if(num != 0)
  {
    line[num] = '\0';

    char *s = line;
    read_space(&s);
    if(*s != '#' && *s != '\0')
    {
      execute_command(s);
    }
    num = 0;
  }

  kfree(script_buffer);
  puts("===Script Completed===\n");
}

void create_dir(const char *DirName)
{
  char dirname[24];
  uint32_t len = 0;

  read_space(&DirName);

  filter_args(dirname, DirName, &len);

  if(len == 0)
  {
    puts("help : mkdir <dir>\n");
    return;
  }

  DIRECTORY_TREE dir_tree = fs_open(dirname);
  uint32_t dir_id = dir_tree.FileID;
  if(!dir_id)
  {
    fs_create_file_name(dirname, CREATE_DIR_RESERVE_SIZE, PERM(7, 7), DIR);
    dir_tree = fs_open(dirname);
    dir_id = dir_tree.FileID;
    if(!dir_id)
    {
      puts("ERROR : Failed To Create Dir ["); puts(dirname); puts("]\n");
      return;
    }
    puts("Create ["); puts(dirname); puts("] Successfully\n");
    return;
  }
  puts("WARNING : The Directory Had Been Created!\n");
  return;
}

void change_directory(const char *DirName)
{
  char dirname[24];
  uint32_t len = 0;

  read_space(&DirName);

  filter_args(dirname, DirName, &len);

  if(len == 0)
  {
    puts("help : cd <dir>\n");
    return;
  }

  if(dirname[0] == '.' && len == 1)
  {
    return; 
  }

  if(dirname[0] == '.' && dirname[1] == '.' && len == 2)
  {
    if(CurrentID == 0) return;
    uint32_t parent_id = GetParentID(CurrentID);
    CurrentID = parent_id;

    path_step--;
    path_ptr -= path_array[path_step];
    *path_ptr = '\0';

    return;
  }

  DIRECTORY_TREE dir_tree = fs_open(dirname);
  uint32_t dir_id = dir_tree.FileID;
  if(!dir_id)
  {
    puts("No Such Directory >> "); puts(dirname); puts("\n");
    return;
  }

  char IsDir = IsDirectory(dir_id);
  switch (IsDir)
  {
    case -1 :
      puts("No Such Directory >> "); puts(dirname); puts("\n");
      return;
    case 0 :
      puts("ERROR : Invalid File Type!\n");
      return;
  }
  
  memcpy(path_ptr, dirname, len);
  path_ptr += len;
  *path_ptr = '/';
  *(++path_ptr) = '\0';
  path_array[path_step] = len + 1;
  path_step++;

  CurrentID = dir_id;
  return;
}

void cmd_ls(void)
{
  list_line();
  return;
}
