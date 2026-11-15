#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

//#define TEXT_BUFFER_SIZE 4056
#define TEXT_BUFFER_SIZE 4088
#define EDITOR_VGA_OFFSET 0x0

#pragma pack(push, 1)
typedef struct _BUFFER_STREAM
{
  /*
  struct _BUFFER_STREAM *Next;
  struct _BUFFER_STREAM *Previous;
  struct _BUFFER_STREAM *Start;
  struct _BUFFER_STREAM *Last;
  */
  uint32_t Reserved1;
  uint16_t Length;
  uint16_t Reserved2;
  char Data[TEXT_BUFFER_SIZE];
} BUFFER_STREAM, *PBUFFER_STREAM;
#pragma pack(pop)

// use row and column make it less complicated
typedef struct _EDITOR_CURSOR
{
  uint16_t Index;
  uint16_t Row;
  uint16_t Column;
}EDITOR_CURSOR, *PEDITOR_CURSOR;

typedef enum _EDITOR_MODE
{
  NORMAL,
  INSERT
}EDITOR_MODE;

void SaveFile(PBUFFER_STREAM text_buffer, const char *FileName);

void EditorEngine(PBUFFER_STREAM text_buffer, const char *FileName);

#endif
