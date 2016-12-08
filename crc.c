#include "crc.h"


char get_bit(char byte, int pos){
    if (((byte >> pos) & 0x01)==0x01){
       return 0x01;
   }
   return 0x00;
} 

char crc8(char* array, int byte_len){
    char poly = 0x07;
    char crc = array[0];
    int i,j;
    for(i = 1; i < byte_len; i++){
       char next_byte = array[i];
       for(j=7; j>=0; j--){
           if(get_bit(crc, 7) == '0'){
              crc = crc << 1;
              crc = crc | get_bit(next_byte, j);
          }
          else{
              crc = crc << 1;
              crc = crc | get_bit(next_byte, j);
              crc = crc ^ poly;
          }
      }
  }
  return crc;
}


void append_crc(char* array, int array_len){
    char crc = crc8(array, array_len);
    array[array_len-1] = crc;
}

int is_corrupted(char* array, int array_len){
    char crc = crc8(array, array_len);
    if (crc == '\0'){
        return 0;
    }
    return 1;

}
