#include "editor.h"
#include "fs.h"
#include "console.h"
#include "kalloc.h"
#include "puts.h"


// Unlimited buffer is too complicated to finish
// so use array temporarily

static void text_buffer_refresh(volatile PBUFFER_STREAM text_buffer, EDITOR_CURSOR cursor_index)
{
  volatile uint8_t *vga_ptr = (uint8_t *)VGA_ADDRESS;
  uint16_t refresh_position = 0;

  for (uint16_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
  {
    vga_ptr[i << 1] = ' ';
    vga_ptr[(i << 1) + 1] = 0x07;
  }

  for(uint16_t i = 0; i < text_buffer->Length; i++)
  {
    if (text_buffer->Data[i] == '\n')
    {
      refresh_position = (refresh_position / (VGA_WIDTH << 1) + 1) * (VGA_WIDTH << 1);
      continue;
    }
    vga_ptr[refresh_position] = text_buffer->Data[i];
    vga_ptr[refresh_position + 1] = 0x0f;
    refresh_position += 2;
  }
  vga_ptr[refresh_position] = ' ';
  vga_ptr[refresh_position + 1] = 0x07;

  StringAddress = cursor_index.Row * (VGA_WIDTH << 1) + (cursor_index.Column << 1);
  update_cursor(0);
}

static void text_buffer_shl(volatile PBUFFER_STREAM text_buffer, EDITOR_CURSOR cursor_index)
{
  for(uint16_t i = cursor_index.Index - 1; i < text_buffer->Length - 1; i++)
  {
    text_buffer->Data[i] = text_buffer->Data[i + 1];
  }
}

static void text_buffer_shr(volatile PBUFFER_STREAM text_buffer, EDITOR_CURSOR cursor_index)
{
  for(uint16_t i = text_buffer->Length; i > cursor_index.Index; i--)
  {
    text_buffer->Data[i] = text_buffer->Data[i - 1];
  }
}

static void cursor_up(volatile PBUFFER_STREAM text_buffer, PEDITOR_CURSOR cursor_index)
{
  if (cursor_index->Row == 0) return;
  uint16_t index = cursor_index->Index;

  while (index > 0 && text_buffer->Data[index - 1] != '\n') index--;
  if (index == 0) return;

  index--;

  while (index > 0 && text_buffer->Data[index - 1] != '\n') index--;

  uint16_t end = index;
  while (end < text_buffer->Length && text_buffer->Data[end] != '\n') end++;

  uint16_t len = end - index;

  cursor_index->Index = index + (cursor_index->Column < len ? cursor_index->Column : len);
  cursor_index->Column = cursor_index->Column < len ? cursor_index->Column : len;
  cursor_index->Row--;
}

static void cursor_down(volatile PBUFFER_STREAM text_buffer, PEDITOR_CURSOR cursor_index)
{
  if (cursor_index->Index >= text_buffer->Length) return;
  uint16_t index = cursor_index->Index;
  while (index < text_buffer->Length && text_buffer->Data[index] != '\n') index++;
  
  index++;

  uint16_t end = index;
  while (end < text_buffer->Length && text_buffer->Data[end] != '\n') end++;
  uint16_t len = end - index;

  cursor_index->Index = index + (cursor_index->Column < len ? cursor_index->Column : len);
  cursor_index->Column = cursor_index->Column < len ? cursor_index->Column : len;
  cursor_index->Row++;
}

static void cursor_left(volatile PBUFFER_STREAM text_buffer, PEDITOR_CURSOR cursor_index)
{
  if (cursor_index->Index == 0) return;

  cursor_index->Index--;

  if (cursor_index->Column > 0) cursor_index->Column--;
  else
  {
    cursor_index->Row--;
    uint16_t index = cursor_index->Index;
  
    while(index > 0 && text_buffer->Data[index - 1] != '\n') index--;

    cursor_index->Column = cursor_index->Index - index;
  }
}

static void cursor_right(volatile PBUFFER_STREAM text_buffer, PEDITOR_CURSOR cursor_index)
{
  if (cursor_index->Index >= text_buffer->Length) return;

  if (text_buffer->Data[cursor_index->Index] == '\n')
  {
    cursor_index->Row++;
    cursor_index->Column = 0;
  } else {
    cursor_index->Column++;
  }

  cursor_index->Index++;
}

void SaveFile(PBUFFER_STREAM text_buffer, const char *FileName)
{
  DIRECTORY_TREE dir_tree = fs_open(FileName);
  if (!dir_tree.FileID)
  {
    puts("ERROR : Failed to Save File!\n");
    return;
  }

  PFILE_STATUS file = fs_get_file(dir_tree.FileID);

  file->UsedBlocks = 0;
  file->BytesOffset = 0;

  if (fs_write_file(dir_tree.FileID, text_buffer->Data, text_buffer->Length))
  {
    puts("ERROR : Failed to Save File!\n");
    return;
  }
}

void EditorEngine(PBUFFER_STREAM text_buffer, const char *FileName)
{
  uint8_t c = 0;
  EDITOR_CURSOR cursor_index = { 0 };
  EDITOR_MODE mode = INSERT; 

  text_buffer_refresh(text_buffer, cursor_index);
  while (1)
  {
    c = buffer_getc();

    if (!c)
    {
      sti();
      hlt();
    }

    if (mode == INSERT) 
    { 
      switch (c)
      {
        case backspace_key :
          if (cursor_index.Index == 0) continue;
	  text_buffer_shl(text_buffer, cursor_index);
	  text_buffer->Length--;
	  if (text_buffer->Data[--cursor_index.Index] == '\n')
	  {
            uint16_t index = cursor_index.Index;
            while (index > 0 && text_buffer->Data[index - 1] != '\n') index--;
	    cursor_index.Row--;
	    cursor_index.Column = cursor_index.Index - index;
	  } else cursor_index.Column--;
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
        case escape_key :
	  mode = NORMAL;
	  continue;
        case up_key :
	  cursor_up(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
	  continue;
        case down_key :
	  cursor_down(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
        case left_key :
	  cursor_left(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
        case right_key :
	  cursor_right(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
      }

      if (c) 
      {
        if (cursor_index.Index != text_buffer->Length) text_buffer_shr(text_buffer, cursor_index);

        if (c == '\n')
        {
          cursor_index.Row++;
          cursor_index.Column = 0;
        } else cursor_index.Column++;

        text_buffer->Data[cursor_index.Index++] = c;
        text_buffer->Length++;
        text_buffer_refresh(text_buffer, cursor_index);    
      }
    }

    if (mode == NORMAL)
    {
      if (c == ':')
      {
        char buf[5] = { 0 };
	char *ptr = buf;
        while(c != '\n')
	{
	  c = buffer_getc();
	  if (!c)
	  {
	    sti();
	    hlt();
	    continue;
	  }
	  *ptr = c;
	  ptr++;
	}
	if (!memcmp(buf, "wq", 2))
	{
          SaveFile(text_buffer, FileName);
	  clear();
	  return;
	}
	else if (!memcmp(buf, "w", 1)) SaveFile(text_buffer, FileName);
	else if (!memcmp(buf, "q", 1))
	{
          clear();
	  return;
	}
      }

      switch (c)
      {
        case backspace_key :
        case escape_key :
	  continue;
	case 'i' :
	  mode = INSERT;
	  continue;
        case up_key :
	  cursor_up(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
	  continue;
        case down_key :
	  cursor_down(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
        case left_key :
	  cursor_left(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
        case right_key :
	  cursor_right(text_buffer, &cursor_index);
	  text_buffer_refresh(text_buffer, cursor_index);
          continue;
      }
    }
  }
}
