#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct _BUFFER_STREAM
{
  struct _BUFFER_STREAM *Next;
  uint16_t Length;
  uint16_t Reserved1;
  uint32_t Reserved2;
  char Data[4080];
} BUFFER_STREAM, *PBUFFER_STREAM;
#pragma pack(pop)

#endif
