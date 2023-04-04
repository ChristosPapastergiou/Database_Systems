#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "record.h"

#define CALL_OR_DIE(call){  \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}

/**** File "identifier" ****/

static const char* string = "Hashtable file";

/**** Offset functions  ****/

static int HT_InfoOffset(void){
  return strlen(string) + 1;
}

static int HT_BlockInfoOffset(HT_info* ht_info){
  return HT_InfoOffset() + sizeof(HT_info) + (ht_info->numBuckets * sizeof(int));
}

static int HT_MetadataOffset(HT_info* ht_info){
  return ht_info->maxBlockRecs * sizeof(Record);
}

static int HT_RecordOffset(HT_block_info* block_info){
  return block_info->recNumber * sizeof(Record);
}

/**** Hash function ****/

static int HT_Function(int ID, int buckets){
  return ID % buckets;
}

/**** Initialize block_info ****/

static HT_block_info* HT_MetadataBlockInitialize(HT_info* ht_info, BF_Block* block){
  void* data;

  CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->lastBlockId, block));
  data = BF_Block_GetData(block);

  // No need to memcopy to initializing, having pointer to our struct 
  HT_block_info* block_info = data + HT_MetadataOffset(ht_info);
  block_info->recNumber = 0;
  block_info->hashBucket = -1;

  return block_info;
}

/**** HashTable functions ****/

int HT_CreateFile(char *fileName,  int buckets){
  int file;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);
  CALL_OR_DIE(BF_CreateFile(fileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &file));

  CALL_OR_DIE(BF_AllocateBlock(file, block));
  data = BF_Block_GetData(block);

  memcpy(data, string, strlen(string) + 1);

  // No need to memcopy to initializing, having pointer to our structs 
  HT_info* ht_info = data + HT_InfoOffset();
  ht_info->blockId = 0;
  ht_info->lastBlockId = 0;
  ht_info->fileDesc = file;
  ht_info->numBuckets = buckets;
  ht_info->maxBlockRecs = (BF_BLOCK_SIZE - sizeof(HT_block_info))/sizeof(Record);
  ht_info->hashTable[buckets];
  for(int i = 0; i < buckets; i++){
    ht_info->hashTable[i] = -1;
  }

  HT_block_info* block_info = data + HT_BlockInfoOffset(ht_info);
  block_info->recNumber = 0;
  block_info->hashBucket = -1;    

  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(file));

  return HT_OK;
}

HT_info* HT_OpenFile(char *fileName){
  int file;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);
  BF_PrintError(BF_OpenFile(fileName, &file));
  BF_PrintError(BF_GetBlock(file, 0, block));
  data = BF_Block_GetData(block);

  if(strcmp(data, string) != 0){              // This must be a hashtable file
    printf("This is not a Hashtable file.\n");
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_CloseFile(file));
    return NULL;
  }

  HT_info* ht_info = data + HT_InfoOffset();

  BF_PrintError(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return ht_info;
}

int HT_CloseFile(HT_info* ht_info){
  CALL_OR_DIE(BF_CloseFile(ht_info->fileDesc));
  
  return HT_OK;
}

int HT_InsertEntry(HT_info* ht_info, Record record){
  void* data;
  void* firstData;

  BF_Block* block;
  BF_Block* firstBlock;
  
  BF_Block_Init(&block);
  BF_Block_Init(&firstBlock);
  
  CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, 0, firstBlock));
  firstData = BF_Block_GetData(firstBlock);

  int hash = HT_Function(record.id, ht_info->numBuckets);

  /**** Allocate block if the hashtable[hash] is empty or if it is full of records. Get the block if we can copy the record ****/

  if(ht_info->hashTable[hash] == -1){
    CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
    ht_info->lastBlockId++;

    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->lastBlockId, block));
    data = BF_Block_GetData(block);

    HT_block_info* block_info = HT_MetadataBlockInitialize(ht_info, block); // Initialize via function

    ht_info->hashTable[hash] = ht_info->lastBlockId;
    memcpy(data + HT_RecordOffset(block_info), &record, sizeof(Record));

    block_info->recNumber++;
  }else{
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->hashTable[hash], block));
    data = BF_Block_GetData(block);

    HT_block_info* block_info = data + HT_MetadataOffset(ht_info);
    
    if(block_info->recNumber == ht_info->maxBlockRecs){
      BF_Block_SetDirty(block);
      CALL_OR_DIE(BF_UnpinBlock(block));

      CALL_OR_DIE(BF_AllocateBlock(ht_info->fileDesc, block));
      ht_info->lastBlockId++;

      CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, ht_info->lastBlockId, block));
      data = BF_Block_GetData(block);

      block_info = HT_MetadataBlockInitialize(ht_info, block);

      data = BF_Block_GetData(block);
      block_info->hashBucket = ht_info->hashTable[hash];  // Update hasbucket before hashtable[hash] change it's value
      ht_info->hashTable[hash] = ht_info->lastBlockId;    // And then update the hashtable[hash] to the last block we are

      memcpy(data + HT_RecordOffset(block_info), &record, sizeof(Record));
      block_info->recNumber++;
    }else{
      memcpy(data + HT_RecordOffset(block_info), &record, sizeof(Record));
      block_info->recNumber++;
    }
  }

  BF_Block_SetDirty(block);
  BF_Block_SetDirty(firstBlock);
  CALL_OR_DIE(BF_UnpinBlock(block));
  CALL_OR_DIE(BF_UnpinBlock(firstBlock));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&firstBlock);

  return ht_info->hashTable[hash];
}

int HT_GetAllEntries(HT_info* ht_info, int value){
  int total = 0;
  int noEntry = 0;

  void* data;
  
  BF_Block* block;

  BF_Block_Init(&block);

  int hash = HT_Function(value, ht_info->numBuckets);

  if(ht_info->hashTable[hash] == -1){             // Check before get_block if a block exists
    printf("There is no entry with this id.\n");
    return -1;
  }

  int temp = ht_info->hashTable[hash];  // To go from block to block need to take a temporary because we cant change hashTable value at the end of the loop
  while(1){
    CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, temp, block));
    data = BF_Block_GetData(block);

    Record* record = data;
    HT_block_info* block_info = data + HT_MetadataOffset(ht_info);

    for(int i = 0; i < block_info->recNumber; i++){
      if(record->id == value){
        printRecord(*record);

        CALL_OR_DIE(BF_UnpinBlock(block));  // Unpin for not having memory leaks
        BF_Block_Destroy(&block);

        return total;

        noEntry++;
      }
      record = data + sizeof(Record) * (i + 1); // Going to the next record of the block
    }
    total++;

    CALL_OR_DIE(BF_UnpinBlock(block));

    if(block_info->hashBucket == -1){
      break;
    }
    
    temp = block_info->hashBucket;    // Going from hashbucket to hashbucket until its over (hashbucket == -1)
  }

  if(noEntry == 0){
    printf("There is no entry with this id.\n");
  }
  
  BF_Block_Destroy(&block);

  return total;
}