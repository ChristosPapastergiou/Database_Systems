#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "record.h"
#include "hp_file.h"

#define CALL_BF(call){      \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

/**** File "identifier" ****/

static const char* string = "Heap file";

/**** Offset functions  ****/

static int HP_InfoOffset(void){
  return strlen(string) + 1;
}

static int HP_BlockInfoOffset(HP_info* hp_info){
  return HP_InfoOffset() + sizeof(HP_info);
}

static int HP_MetadataOffset(HP_info* hp_info){
  return hp_info->maxBlockRecs * sizeof(Record);
}

static int HP_RecordOffset(HP_block_info* block_info){
  return block_info->recNumber * sizeof(Record);
}

/**** Heap File functions ****/

int HP_CreateFile(char *fileName){
  int file;
  void* data;

  BF_Block* block;

  BF_Block_Init(&block);
  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &file));

  CALL_BF(BF_AllocateBlock(file, block));
  data = BF_Block_GetData(block);

  memcpy(data, string, strlen(string));   // Copy to metadata block the string to identify this is a heap

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(file));

  return HP_OK;
}

HP_info* HP_OpenFile(char *fileName){
  int file;
  void* data;

  BF_Block* block;

  BF_Block_Init(&block);
  BF_PrintError(BF_OpenFile(fileName, &file));
  BF_PrintError(BF_GetBlock(file, 0, block));
  data = BF_Block_GetData(block);

  if(strcmp(data, string) != 0){            // This must be a heap file
    printf("This is not o heap file.\n");
    BF_PrintError(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    BF_PrintError(BF_CloseFile(file));
    return NULL;
  }

  // No need to memcopy to initializing, having pointer to our structs 
  HP_info* hp_info = data + HP_InfoOffset();    
  hp_info->blockId = 0;
  hp_info->fileDesc = file;
  hp_info->lastBlockId = 0;
  hp_info->maxBlockRecs = (BF_BLOCK_SIZE - sizeof(HP_block_info))/sizeof(Record);

  HP_block_info* block_info = data + HP_BlockInfoOffset(hp_info);
  block_info->recNumber = 0;
  block_info->nextBlock = 0;

  BF_Block_SetDirty(block);
  BF_PrintError(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return hp_info;
}

int HP_CloseFile(HP_info* hp_info){
  CALL_BF(BF_CloseFile(hp_info->fileDesc));

  return HP_OK;
}

int HP_InsertEntry(HP_info* hp_info, Record record){
  static int fileBlockSlot = 0;   // Know whenever we need to allocate a new block (initialize it's values)

  void* firstBlockData;
  void* currentBlockData;
  void* previousBlockData;  // "Temp" block data to find the previous block data

  BF_Block* firstBlock;
  BF_Block* currentBlock;
  BF_Block* previousBlock;  // "Temp" block to find the previous block so we can make it to point to current

  BF_Block_Init(&firstBlock);
  BF_Block_Init(&currentBlock);
  BF_Block_Init(&previousBlock);

  CALL_BF(BF_GetBlock(hp_info->fileDesc, 0, firstBlock));
  firstBlockData = BF_Block_GetData(firstBlock);
  hp_info = firstBlockData + HP_InfoOffset();
  
  if(fileBlockSlot == 0){    
    CALL_BF(BF_AllocateBlock(hp_info->fileDesc, currentBlock));                   // If the block has been full with records 
    hp_info->lastBlockId++;                                                       // allocate a new block, the same if there is no block 
  }else{                                                                          // (at the begging). If its not full just get the block
    CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->lastBlockId, currentBlock));
  }

  if(hp_info->lastBlockId - 1 == 0) {   // Updating what is the next block_info
    CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->lastBlockId - 1, previousBlock));     // If its the first block calculate different
    currentBlockData = BF_Block_GetData(currentBlock);                                    // offset because block 0 has only metadata and 0 records
    previousBlockData = BF_Block_GetData(previousBlock);
    HP_block_info* previousBlock_info = previousBlockData + HP_BlockInfoOffset(hp_info);
    previousBlock_info->nextBlock = hp_info->lastBlockId;
  }else{
    CALL_BF(BF_GetBlock(hp_info->fileDesc, hp_info->lastBlockId - 1, previousBlock));     
    currentBlockData = BF_Block_GetData(currentBlock);
    previousBlockData = BF_Block_GetData(previousBlock);
    HP_block_info* previousBlock_info = previousBlockData + HP_MetadataOffset(hp_info);
    previousBlock_info->nextBlock = hp_info->lastBlockId;
  }

  HP_block_info* block_info = currentBlockData + HP_MetadataOffset(hp_info);
  if(fileBlockSlot == 0){
    block_info->recNumber = 0;
    block_info->nextBlock = 0;
  }

  memcpy(currentBlockData + HP_RecordOffset(block_info), &record, sizeof(Record));  // Memcpy with offset to write it to the right "position"
  block_info->recNumber++;

  BF_Block_SetDirty(firstBlock);
  BF_Block_SetDirty(currentBlock);
  BF_Block_SetDirty(previousBlock);
  CALL_BF(BF_UnpinBlock(firstBlock));
  CALL_BF(BF_UnpinBlock(currentBlock));
  CALL_BF(BF_UnpinBlock(previousBlock));
  BF_Block_Destroy(&firstBlock);
  BF_Block_Destroy(&currentBlock);
  BF_Block_Destroy(&previousBlock);

  fileBlockSlot++;
  if(hp_info->maxBlockRecs == block_info->recNumber){
    fileBlockSlot = 0;
  }

  return hp_info->lastBlockId;
}

int HP_GetAllEntries(HP_info* hp_info, int value){
  int total = 0;
  int noEntry = 0;

  void* data;
  BF_Block* block;

  BF_Block_Init(&block);

  int temp = 1; // Just a temp to use to get a block (at the end of the loop this will change)
  while(1){
    CALL_BF(BF_UnpinBlock(block));
    CALL_BF(BF_GetBlock(hp_info->fileDesc, temp, block));
    data = BF_Block_GetData(block);

    Record* record = data;
    HP_block_info* block_info = data + HP_MetadataOffset(hp_info);

    for(int i = 0; i < block_info->recNumber; i++){
      if(record->id == value){
        printRecord(*record);
        noEntry++;
      }
      record = data + sizeof(Record) * (i + 1); // Going to the next record of the block
    }
    total++;

    CALL_BF(BF_UnpinBlock(block));  // Unpin for not having memory leaks

    if(block_info->nextBlock == 0) break;

    temp = block_info->nextBlock;   // Going to the next block each time
  }

  if(noEntry == 0) printf("There is no entry with this id.\n");

  BF_Block_Destroy(&block);
  
  return total;
}