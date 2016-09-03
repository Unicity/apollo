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
#include <stdint.h>

typedef uint64_t u64;
typedef int64_t s64;
//#include "strptime.h"
/*
  WHOO!
*/
//char *strptime(const char * __restrict, const char * __restrict, struct tm * __restrict);

#define TOTAL_BLOCK_STORAGE 700000
const u64 BUFF_SIZE = (u64)1024*1024*1024*4;
struct block {
  int fileStart;
  int fileEnd;
  int idStart;
  int idLength;
  int date;
  block * next;
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

void printBlock(char* fileString, block *b){
  printf("%.*s\n", b->fileEnd - b->fileStart, fileString + b->fileStart);
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
  int numberOfStringsToSearch = 0;
  char *searchPattern;
  int searchPatternLength = 0;
  char strang_;
  int blockCount = 0;
  char *fileName = 0;
  u64 filesize = 0;
  char *searchStrings[50];
  char *fileString = 0;
  for(int i=1; i<argc; i++){
    if(argv[i][0] == '-' && argv[i][1] == 'f'){
      fileName = argv[i+1];
      i++;
    }
    else{
      searchStrings[numberOfStringsToSearch] = argv[i];
      numberOfStringsToSearch++;
    }
  }
  if(!numberOfStringsToSearch){
    printf("no search pattern\n");
    return 1;
  }


  if(fileName){
    FILE *file = fopen(fileName, "r");
    fseek(file, 0, SEEK_END);
    filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    fileString = (char *)malloc(sizeof(char) * filesize);
    if(!fileString){
      printf("Couldnt allocate space for the file");
      return 0;
    }
    fread(fileString, sizeof(char), filesize, file);
  }
  else{
    fileString = (char*)malloc(sizeof(char) * BUFF_SIZE);
    if(!fileString){
      printf("couldnt allocate space for the file");
      return 1;
    }
    filesize = fread(fileString, sizeof(char), BUFF_SIZE, stdin);
    fileString = (char*)realloc(fileString, filesize);
    #if 0
    while(fread(buffer, sizeof(char), BUFF_SIZE, stdin)){
      char *old = fileString;
      u64 diff = contentSize/BUFF_SIZE;
      //printf("%d, %llu\n", contentSize, diff);
      int buffSize = strlen(buffer);
      contentSize += buffSize;
      printf("%d\n", contentSize);
      fileString = (char*)realloc(fileString, contentSize);
      if(!fileString){
        printf("couldnt allocate space for the file");
        return 1;
      }
      strcat(fileString, buffer);
    }
    #endif
  }
  int fileIndex = 0;
  int begin = 0;
  int end = 0;
  strang_ = fileString[fileIndex];
  block *requests[10] = {0};
  int requestIndex = 0;
  while(fileIndex < filesize){
    int tempIndex = fileIndex;
    if(strang_ == '[' && fileString[fileIndex+1] == ']' && fileString[fileIndex + 2] == '\n'){
      end = fileIndex + 2;
      while(begin < filesize && fileString[begin] != '['){
        begin++;
      }
      while(end < filesize && isWhitespace(fileString[end])){
        end++;
      }
      if(end - begin > 1){
        block *newBlock = (block *)malloc(sizeof(block));
        if(!newBlock){
          printf("bad alloc\n");
          return 1;
        }
        newBlock->next = 0;
        newBlock->fileStart = begin;
        newBlock->fileEnd = end;
        char *tempChar = fileString + begin;
        while(*tempChar != ']'){
          tempChar++;          
        }
        newBlock->date = getDateAsTimestamp(fileString + begin + 1);
        newBlock->idStart = tempChar - fileString;
        while(*tempChar != '.'){
          tempChar++;
        }
        newBlock->idLength = (tempChar - fileString) - newBlock->idStart;

        char *stringStart = &fileString[newBlock->fileStart];
        int blockLength = newBlock->fileEnd - newBlock->fileStart;
        if(!sstrnstr(stringStart, "Script start [] []", blockLength)){
          block *start;
          int startIndex;
          block *cur;
          //find the very start of the request
          for(int ri =0; ri<requestIndex; ri++){
            if(!requests[ri])break;
            if(strncmp(fileString + requests[ri]->idStart, fileString + newBlock->idStart, newBlock->idLength) == 0){
              start = requests[ri];
              startIndex = ri;
              break;
            }
          }
          //Add ourselves to the end of the list

          cur = start;
          while(cur->next){
            cur = cur->next;
          }
          cur->next = newBlock;

          if(sstrnstr(stringStart, "Script stop [] []", blockLength)){
            //Means we hit the end of a request
            bool match = false;
            //see if any block matches any of our search strings
            cur = start;
            while(cur){
              char *stringStart = &fileString[cur->fileStart];
              int blockLength = cur->fileEnd - cur->fileStart;
              for(int searchStringIndex = 0; searchStringIndex < numberOfStringsToSearch; searchStringIndex++){
                char *searchPattern = searchStrings[searchStringIndex];
                if(sstrnstr(stringStart, searchPattern, blockLength)){
                  match = true;
                  break;
                }
              }
              if(match){
                break;
              }
              cur = cur->next;
            }
            //if we have a match print the request
            if(match){
              printf("------ REQUEST --------\n");
              cur = start;
              while(cur){
                printBlock(fileString, cur);
                cur = cur->next;
              }
            }
            //Remove our request from the requests array
            #if 0
            while(start){
              block* temp = start;
              start = start->next;
              free(temp);
            }
            #endif  
            requests[startIndex] = 0;
            requestIndex--;
            block *temp = requests[requestIndex];
            requests[requestIndex] = 0;
            requests[startIndex] = temp;
          }
        }
        else{
          requests[requestIndex] = newBlock;
          requestIndex++;
        }
          
      }
      begin = end;
      fileIndex += 2;
    }
    fileIndex++;
    strang_ = fileString[fileIndex];

  }
  free(fileString);
  

}

