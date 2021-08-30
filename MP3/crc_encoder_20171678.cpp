#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <limits.h>

typedef unsigned char byte;
typedef bool bit;

FILE *inputfile, *outputfile;
int lgen, ldat;
byte gen[9];
byte padding;
int fileSize;

void encode();
void getcode(int*, int*);

byte mod(int a, int b){
  return a != b;
}

int main (int argc, char *argv[]){
  
  
  if(argc != 5){
    fprintf(stderr, "usage: ./crc_encoder input_file output_file generator dataword_size\n");
    exit(1);
  }

  // file open
  inputfile = fopen(argv[1], "rb");
  if(!inputfile){
    fprintf(stderr, "input file open error.\n");
    exit(1);
  }

  outputfile = fopen(argv[2], "wb");
  if(!outputfile){
    fprintf(stderr, "output file open error.\n");
    exit(1);
  }
  
  if(strcmp(argv[4], "4") != 0 && strcmp(argv[4], "8")){   
    fprintf(stderr, "dataword size must be 4 or 8.\n");
    exit(1);
  }
  
  ldat = atoi(argv[4]);
  lgen = strlen(argv[3]);
  
  // get generator
  for(int i = 0; i < lgen; i++){
    gen[i] += argv[3][i] - '0';
  }

  // get padding
  fseek(inputfile, 0L, SEEK_END);
  fileSize = ftell(inputfile);
  rewind(inputfile);

  padding = 8 - ((fileSize * 8 / ldat) * (lgen - 1 + ldat) % 8);
  if (padding == 8) padding = 0;

  fwrite(&padding, 1, 1, outputfile);
  
  encode();
  
  //exit
  fclose(inputfile);
  fclose(outputfile);

}

void encode (){
  byte token;
  int dataword[8];
  int codeword[24];
  int bitswritten = padding;
  int count = 0;
  for (int i = 0; i < padding; i++){
    codeword[i] = 0;
  }

  for(count = 0; count < fileSize; count++){
    fread(&token, 1, 1, inputfile);

    for(int i = 0; i < 8; i++){
      dataword[7 - i] = token % 2;
      token = token >> 1;
    }

    for(int bitcount = 0; bitcount < 8; bitcount += ldat){
      getcode(dataword + bitcount, codeword + bitswritten);
      bitswritten += ldat + lgen - 1; 

      while(bitswritten >= 8){
        byte temp = 0;
        for(int i = 0; i < 8; i++){
          temp = temp << 1;
          temp += codeword[i];
        }
        for(int i = 0 ; i < bitswritten - 8; i++){
          codeword[i] = codeword[8 + i];
        }
        fwrite(&temp, 1, 1, outputfile);
        bitswritten -= 8;
      }
    }
      
  }  

}

void getcode(int* dataword, int* codeword){
  int temp[16];
  
  for(int i = 0; i < ldat; i++){
    *(codeword + i) = *(dataword + i);
    temp[i] = *(dataword + i);
  }

  for(int i = ldat; i < ldat + lgen - 1; i++){
    temp[i] = 0;
  }

  for(int i = 0; i < ldat; i++){
    if(temp[i] == 1){
      for(int j = 0; j < lgen; j++){
        temp[i + j] = mod(temp[i+j], gen[j]);
      }
    }
  }
  
  for(int i = 0; i < lgen - 1;i++){
    *(codeword + ldat + i) = temp[ldat + i];
  }
  /*for(int i = 0; i < ldat + lgen - 1; i++){
      printf("%d", *(codeword + i));
    }
    printf("\n");
    for(int i = 0; i < ldat + lgen - 1; i++){
      printf("%d", temp[i]);
    }
    printf("\n");*/
}
