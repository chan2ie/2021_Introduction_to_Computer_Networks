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

FILE *inputfile, *outputfile, *resultfile;
int lgen, ldat;
byte gen[9];
byte padding;
int fileSize;
int errorcount = 0;
int codewordcount = 0;

void decode();
int checkcode(int*);

byte mod(int a, int b){
  return a != b;
}

int main (int argc, char *argv[]){
  
  
  if(argc != 6){
    fprintf(stderr, "usage: ./crc_decoder input_file output_file result_file generator dataword_size\n");
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

  resultfile = fopen(argv[3], "w");
  if(!resultfile){
    fprintf(stderr, "result file open error.\n");
  }
  
  if(strcmp(argv[5], "4") != 0 && strcmp(argv[5], "8")){   
    fprintf(stderr, "dataword size must be 4 or 8.\n");
    exit(1);
  }
  
  ldat = atoi(argv[5]);
  lgen = strlen(argv[4]);
  
  // get generator
  for(int i = 0; i < lgen; i++){
    gen[i] += argv[4][i] - '0';
  }
  
  fseek(inputfile, 0L, SEEK_END);
  fileSize = ftell(inputfile);
  rewind(inputfile);
  
  fread(&padding, 1, 1, inputfile);
  decode();
  
  fprintf(resultfile, "%d %d", codewordcount, errorcount);

  //exit
  fclose(inputfile);
  fclose(outputfile);
  fclose(resultfile);
}

void decode (){
  byte token;
  int dataword[8];
  int codeword[24];
  int bitsread = 8 - padding;
  int count = 0;
  int bitswritten = 0;

  if(bitsread == 8) bitsread = 0;

  fread(&token, 1, 1, inputfile);

  for (int i = 0; i < 8 - padding; i++){
    codeword[7-padding-i] = token % 2;
    token = token >> 1;
  }

  for(count = 2; count < fileSize; count++){
    fread(&token, 1, 1, inputfile);

    for(int i = 0; i < 8; i++){
      codeword[bitsread + 7 - i] = token % 2;
      token = token >> 1;
    }
    
    bitsread += 8;

    while(bitsread >= ldat + lgen - 1){
      errorcount += checkcode(codeword);
      codewordcount += 1;
      
      for(int i = 0; i < ldat; i++){
        dataword[bitswritten + i] = codeword[i];
      }

      for(int i = 0; i < bitsread - (ldat + lgen - 1); i++){
        codeword[i] = codeword[i + ldat + lgen - 1];
      }

      bitsread -= ldat + lgen - 1;

      bitswritten += ldat;
      if(bitswritten == 8){
        byte temp = 0;
        for(int i = 0; i < 8; i++){
          temp = temp << 1;
          temp += dataword[i];
        }
        fwrite(&temp, 1, 1, outputfile);
        bitswritten = 0;
      }
    }
      
  }  

}

int checkcode(int* codeword){
  int temp[16];
  bool flag = false;
  
  for(int i = 0; i < ldat; i++){
    temp[i] = *(codeword + i);
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
    if(*(codeword + ldat + i) != temp[ldat + i]) flag = true;
  }

  if(flag){
    /*for(int i = 0; i < ldat + lgen - 1; i++){
      printf("%d", *(codeword + i));
    }
    printf("\n");
    for(int i = 0; i < ldat + lgen - 1; i++){
      printf("%d", temp[i]);
    }
    printf("\n");*/
    return 1;
  }
  else return 0;
}
