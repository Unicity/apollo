/*    Copyright 2016 Unicity International
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
  int dateStringLength = 19;
  int currentIndex = 0;
  int currentNumberIndex = 0;
  int numbers[6] = {0};

  for(currentIndex = 0; currentIndex < dateStringLength; currentIndex++){
    char currentChar = *(dateCharStart + currentIndex);
    if(currentChar == '-' || currentChar == ' ' || currentChar == ':'){
      currentNumberIndex++;
      continue;
    }
    else{
      numbers[currentNumberIndex] *= 10;
      numbers[currentNumberIndex] += currentChar - '0';
    }
  }

  //Year is years from 1900 eg 2016 -> 116
  dateThing.tm_year = numbers[0] - 1900;
  //Month is the months zero based (0-11)
  dateThing.tm_mon  = numbers[1] - 1;
  dateThing.tm_mday = numbers[2];
  dateThing.tm_hour = numbers[3];
  dateThing.tm_min  = numbers[4];
  dateThing.tm_sec  = numbers[5];

  return (int)mktime(&dateThing);

}

void printBlock(char* fileString, block *blocks, int blockIndex){
  printf("%.*s\n", blocks[blockIndex].fileEnd - blocks[blockIndex].fileStart, fileString + blocks[blockIndex].fileStart);
}

int hash(char* string, int len){
  int hashAddress = 5381;
  for (int counter = 0; counter < len; counter++){
      hashAddress = ((hashAddress << 5) + hashAddress) + string[counter];
  }
  return hashAddress % TOTAL_BLOCK_STORAGE;
}

void markRequestPrinted(bool *table, char* id, int idLength){
  table[hash(id, idLength)] = 1;
}

bool isRequestPrinted(bool *table, char *id, int idLength){
  return table[hash(id, idLength)];
}

int main(int argc, char* argv[]){
  int numberOfStringsToSearch = argc - 2;
  char *filename = argv[1];
  char *searchPattern = argv[2];
  int searchPatternLength = strlen(searchPattern);
  char strang_;
  int blockCount = 0;
  FILE *file = fopen(filename, "r");
  block *blocks = (block*)malloc(sizeof(block) * TOTAL_BLOCK_STORAGE);

  if(argc < 3){
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
  bool requestsPrinted[TOTAL_BLOCK_STORAGE] = {0};
  
  for(int blockIndex = 0; blockIndex < blockCount; blockIndex++){
    for(int searchStringIndex = 0; searchStringIndex < numberOfStringsToSearch; searchStringIndex++){
      char *searchPattern = argv[searchStringIndex + 2];
      char *stringStart = &fileString[blocks[blockIndex].fileStart];
      int blockLength = blocks[blockIndex].fileEnd - blocks[blockIndex].fileStart;
      if(sstrnstr(stringStart, searchPattern, blockLength)){
        if(!isRequestPrinted(requestsPrinted, fileString + blocks[blockIndex].idStart, blocks[blockIndex].idLength)){
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
          markRequestPrinted(requestsPrinted, fileString + blocks[blockIndex].idStart, blocks[blockIndex].idLength);
        }
      }
    }
    
  }
  free(blocks);
  free(fileString);
  printf("Occurances of string found: %i", occurences);
  

}

