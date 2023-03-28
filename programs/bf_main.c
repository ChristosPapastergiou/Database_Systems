#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "record.h"

#define CALL_OR_DIE(call){  \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}

int main() {
  int fd1;
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_CreateFile("block_example.db"))
  CALL_OR_DIE(BF_OpenFile("block_example.db", &fd1));

  void* data;
  for(int i = 0; i < 10; ++i){
    CALL_OR_DIE(BF_AllocateBlock(fd1, block));  // Making a block
    data = BF_Block_GetData(block);             // data point to the begging of the block
    Record* rec = data;                         // rec points to the begging of data
    rec[0] = randomRecord();
    rec[1] = randomRecord();
    BF_Block_SetDirty(block);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }
  CALL_OR_DIE(BF_CloseFile(fd1)); // File close and save
  CALL_OR_DIE(BF_Close());

  CALL_OR_DIE(BF_Init(LRU));
  CALL_OR_DIE(BF_OpenFile("block_example.db", &fd1));
  int blocks_num;
  CALL_OR_DIE(BF_GetBlockCounter(fd1, &blocks_num));

  for(int i = 0; i < blocks_num; ++i){
    printf("Contents of Block %d\n\t",i);
    CALL_OR_DIE(BF_GetBlock(fd1, i, block));
    data = BF_Block_GetData(block);
    Record* rec= data;
    printRecord(rec[0]);
    printf("\t");
    printRecord(rec[1]);
    CALL_OR_DIE(BF_UnpinBlock(block));
  }

  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(fd1));
  CALL_OR_DIE(BF_Close());
}

