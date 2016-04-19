#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h>

//#include "strptime.h"
/*
  WHOO!
*/
//char *strptime(const char * __restrict, const char * __restrict, struct tm * __restrict);

#define TOTAL_BLOCK_STORAGE 500000
struct block {
  int fileStart;
  int fileEnd;
  int idStart;
  int idLength;
  int date;
};

bool isWhitespace(char ch){
  bool result  = false;
  if(ch == ' ' || ch == '\n'){
    result = true;
  }
  return result;
}

int *kmp_borders(char *needle, size_t nlen){
    if (!needle) return NULL;
    int i, j, *borders = (int *)malloc((nlen+1)*sizeof(*borders));
    if (!borders) return NULL;
    i = 0;
    j = -1;
    borders[i] = j;
    while((size_t)i < nlen){
        while(j >= 0 && needle[i] != needle[j]){
            j = borders[j];
        }
        ++i;
        ++j;
        borders[i] = j;
    }
    return borders;
}

char *kmp_search(char *haystack, size_t haylen, char *needle, size_t nlen, int *borders){
    size_t max_index = haylen-nlen, i = 0, j = 0;
    while(i <= max_index){
        while(j < nlen && *haystack && needle[j] == *haystack){
            ++j;
            ++haystack;
        }
        if (j == nlen){
            return haystack-nlen;
        }
        if (!(*haystack)){
            return NULL;
        }
        if (j == 0){
            ++haystack;
            ++i;
        } else {
            do{
                i += j - (size_t)borders[j];
                j = borders[j];
            }while(j > 0 && needle[j] != *haystack);
        }
    }
    return NULL;
}

char *sstrnstr(char *haystack, char *needle, size_t haylen){
    if (!haystack || !needle){
        return NULL;
    }
    size_t nlen = strlen(needle);
    if (haylen < nlen){
        return NULL;
    }
    int *borders = kmp_borders(needle, nlen);
    if (!borders){
        return NULL;
    }
    char *match = kmp_search(haystack, haylen, needle, nlen, borders);
    free(borders);
    return match;
}

int getDateAsTimestamp(char *dateCharStart){
  tm dateThing;
  dateThing.tm_year = 0;
  dateThing.tm_mon = 0;
  dateThing.tm_mday = 0;
  dateThing.tm_hour = 0;
  dateThing.tm_min = 0;
  dateThing.tm_sec = 0;

  dateThing.tm_year += 1000 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_year += 100 * (*(dateCharStart) - '0');
  dateCharStart++;
  dateThing.tm_year += 10 * (*(dateCharStart) - '0');
  dateCharStart++;
  dateThing.tm_year += 1 * (*(dateCharStart) - '0');
  dateCharStart++;
  //Skip past the '-'
  dateCharStart++;

  dateThing.tm_mon += 10 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_mon += 1 * (*dateCharStart - '0');
  dateCharStart++;
  dateCharStart++;

  dateThing.tm_mday += 10 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_mday += 1 * (*dateCharStart - '0');
  dateCharStart++;
  dateCharStart++;

  dateThing.tm_hour += 10 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_hour += 1 * (*dateCharStart - '0');
  dateCharStart++;
  dateCharStart++;

  dateThing.tm_min += 10 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_min += 1 * (*dateCharStart - '0');
  dateCharStart++;
  dateCharStart++;

  dateThing.tm_sec += 10 * (*dateCharStart - '0');
  dateCharStart++;
  dateThing.tm_sec += 1 * (*dateCharStart - '0');
  dateCharStart++;
  dateCharStart++;

  dateThing.tm_year = dateThing.tm_year - 1900;
  return (int)mktime(&dateThing);

}

void printBlock(char* fileString, block *blocks, int blockIndex){
  printf("%.*s\n", blocks[blockIndex].fileEnd - blocks[blockIndex].fileStart, fileString + blocks[blockIndex].fileStart);
}


int main(int argc, char* argv[]){

  char *filename = argv[1];
  char *searchPattern = argv[2];
  int searchPatternLength = strlen(searchPattern);
  char strang_;
  int blockCount = 0;
  FILE *file = fopen(filename, "r");
  block *blocks = (block*)malloc(sizeof(block) * TOTAL_BLOCK_STORAGE);

  if(argc != 3){
    printf("Not enough arguments\n");
    return 0;
  }

  if(!blocks){
    printf("couldn't get block storage\n");
    return 0;
  }

  if(!file){
    printf("couldn't open file");
    return 0;
  }

  
  fseek(file, 0, SEEK_END);
  int filesize = ftell(file);
  fseek(file, 0, SEEK_SET);
  int begin = ftell(file);
  int end = 0;
  char* fileString = (char *)malloc(sizeof(char) * filesize);
  if(!fileString){
    printf("Couldnt allocate space for the file");
    return 0;
  }
  fread(fileString, sizeof(char), filesize, file);
  int fileIndex = 0;
  strang_ = fileString[fileIndex];
  begin = 0;
  while(fileIndex < filesize && blockCount < TOTAL_BLOCK_STORAGE){
    int tempIndex = fileIndex;
    if(strang_ == '[' && fileString[fileIndex+1] == ']' && fileString[fileIndex + 2] == '\n'){ 
      end = fileIndex + 2;
      while(isWhitespace(fileString[end])){
        end++;
      }
      if(end - begin > 1){
        blocks[blockCount].fileStart = begin;
        blocks[blockCount].fileEnd = end;
        char *tempChar = fileString + begin;
        while(*tempChar != ']'){
          tempChar++;          
        }
        
        blocks[blockCount].date = getDateAsTimestamp(fileString + begin + 1);
        blocks[blockCount].idStart = tempChar - fileString;
        while(*tempChar != '.'){
          tempChar++;
        }
        blocks[blockCount].idLength = (tempChar - fileString) - blocks[blockCount].idStart;

        blockCount++;
      }
             

      begin = end;
      fileIndex += 2;
    }
    fileIndex++;
    strang_ = fileString[fileIndex];

  }
  int occurences = 0;
  for(int blockIndex = 0; blockIndex < blockCount; blockIndex++){
    char *stringStart = &fileString[blocks[blockIndex].fileStart];
    int blockLength = blocks[blockIndex].fileEnd - blocks[blockIndex].fileStart;
    if(sstrnstr(stringStart, searchPattern, blockLength)){
      printf("------ REQUEST --------\n");

      int startIndex = blockIndex - 100;
      int endIndex = blockIndex + 100;
      occurences++;
      if(startIndex < 0){
        startIndex = 0;
      }
      if(endIndex == blockCount){
        endIndex = blockCount;
      }
      for(int bIndex = startIndex; bIndex < endIndex; bIndex++){
        if(bIndex != blockIndex){
          if(strncmp(fileString + blocks[bIndex].idStart, fileString + blocks[blockIndex].idStart, blocks[blockIndex].idLength) == 0){
            printBlock(fileString, blocks, bIndex);
          }
        }
        else{
          printBlock(fileString, blocks, bIndex);
        }
      }
    }
  }
  free(blocks);
  free(fileString);
  printf("Occurances of string found: %i", occurences);
  

}

