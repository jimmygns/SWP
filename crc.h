#ifndef __CRC_H__
#define __CRC_H__

#include <stdlib.h>
#include <string.h>
#include "util.h"

char get_bit(char byte, int pos);
char crc8(char* array, int byte_len);
void append_crc(char* array, int array_len);
int is_corrupted(char* array, int array_len);

#endif
