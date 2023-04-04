#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "record.h"
#include "ht_table.h"
#include "sht_table.h"

#define CALL_OR_DIE(call){  \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}

/**** File "identifier" ****/

static const char* string = "SHash file";

/**** Offset functions  ****/

static int SHT_InfoOffset(void){
  return strlen(string) + 1;
}

static int SHT_BlockInfoOffset(SHT_info* sht_info){
  return SHT_InfoOffset() + sizeof(HT_info) + (sht_info->numBuckets * sizeof(int));
}

static int SHT_MetadataOffset(SHT_info* sht_info){
  return sht_info->maxBlockRecs * (sizeof(char) * 15 + sizeof(unsigned int));
}

static int SHT_RecordOffset(SHT_block_info* block_info){
  return block_info->recNumber * (sizeof(char) * 15 + sizeof(unsigned int));
}

static int SHT_RecordNameOffset(void){
  return sizeof(char) * 15;
}

/**** String hash function ****/

static int SHT_Function(unsigned char *str, int buckets){
  int c;
  int hash = 0;

  while (c = *str++){
    hash = hash + c;
  }

  return (int) hash % buckets;
}

/**** Initialize block_info ****/

static SHT_block_info* SHT_MetadataBlockInitialize(SHT_info* sht_info, BF_Block* block){
  void* data;

  CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->lastBlockId, block));
  data = BF_Block_GetData(block);

  // No need to memcopy to initializing, having pointer to our struct
  SHT_block_info* block_info = data + SHT_MetadataOffset(sht_info);
  block_info->recNumber = 0;
  block_info->hashBucket = -1;

  return block_info;
}

/**** Secondary HashTable functions ****/

int SHT_CreateSecondaryIndex(char *sfileName,  int buckets, char* fileName){
  int file;
  int sfile;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);
  CALL_OR_DIE(BF_CreateFile(sfileName));
  CALL_OR_DIE(BF_OpenFile(fileName, &file));    // Open both files with "filename"
  CALL_OR_DIE(BF_OpenFile(sfileName, &sfile));  // so they can get correct filedesc

  CALL_OR_DIE(BF_AllocateBlock(sfile, block));
  data = BF_Block_GetData(block);

  memcpy(data, string, strlen(string) + 1);

  // No need to memcopy to initializing, having pointer to our structs 
  SHT_info* sht_info = data + SHT_InfoOffset();
  sht_info->blockId = 0;
  sht_info->lastBlockId = 0;
  sht_info->fileDesc = sfile;
  sht_info->numBuckets = buckets;
  sht_info->maxBlockRecs = (BF_BLOCK_SIZE - sizeof(SHT_block_info))/(sizeof(char) * 15 + sizeof(unsigned int));
  sht_info->hashTable[buckets];
  for(int i = 0; i < buckets; i++){
    sht_info->hashTable[i] = -1;
  }

  SHT_block_info* block_info = data + SHT_BlockInfoOffset(sht_info);
  block_info->recNumber = 0;
  block_info->hashBucket = -1;    

  BF_Block_SetDirty(block);
  CALL_OR_DIE(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_OR_DIE(BF_CloseFile(file));
  CALL_OR_DIE(BF_CloseFile(sfile));

  return HT_OK;
}

SHT_info* SHT_OpenSecondaryIndex(char *indexName){
  int file;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);
  BF_PrintError(BF_OpenFile(indexName, &file));
  BF_PrintError(BF_GetBlock(file, 0, block));
  data = BF_Block_GetData(block);

  if(strcmp(data, string) != 0){                          // This must be a secondary hashtable file
    printf("This is not a Secondary Hashtable file.\n");
    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);
    CALL_OR_DIE(BF_CloseFile(file));
    return NULL;
  }

  SHT_info* sht_info = data + SHT_InfoOffset();

  BF_PrintError(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);

  return sht_info;
}

int SHT_CloseSecondaryIndex(SHT_info* sht_info){
  CALL_OR_DIE(BF_CloseFile(sht_info->fileDesc));
  
  return HT_OK;
}

int SHT_SecondaryInsertEntry(SHT_info* sht_info, Record record, int block_id){
  void* data;
  void* firstData;

  BF_Block* block;
  BF_Block* firstBlock;
  
  BF_Block_Init(&block);
  BF_Block_Init(&firstBlock);
  
  CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, 0, firstBlock));
  firstData = BF_Block_GetData(firstBlock);

  int hash = SHT_Function(record.name, sht_info->numBuckets);

  /**** Allocate block if the hashtable[hash] is empty or if it is full of records. Get the block if we can copy the record ****/

  if(sht_info->hashTable[hash] == -1){
    CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, block));
    sht_info->lastBlockId++;

    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->lastBlockId, block));
    data = BF_Block_GetData(block);

    SHT_block_info* block_info = SHT_MetadataBlockInitialize(sht_info, block);  // Initialize via function

    sht_info->hashTable[hash] = sht_info->lastBlockId;

    memcpy(data + SHT_RecordOffset(block_info), &record.name , sizeof(record.name));                            // Need to copy a name and an int to memory so
    memcpy(data + SHT_RecordOffset(block_info) + sizeof(record.name), (void*) &block_id, sizeof(unsigned int)); // 2 memcpy and the offsets for this
    block_info->recNumber++;
  }else{
    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->hashTable[hash], block));
    data = BF_Block_GetData(block);

    SHT_block_info* block_info = data + SHT_MetadataOffset(sht_info);
    
    if(block_info->recNumber == sht_info->maxBlockRecs){
      BF_Block_SetDirty(block);
      CALL_OR_DIE(BF_UnpinBlock(block));

      CALL_OR_DIE(BF_AllocateBlock(sht_info->fileDesc, block));
      sht_info->lastBlockId++;

      CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, sht_info->lastBlockId, block));
      data = BF_Block_GetData(block);

      block_info = SHT_MetadataBlockInitialize(sht_info, block);

      data = BF_Block_GetData(block);
      block_info->hashBucket = sht_info->hashTable[hash];   // Update hasbucket before hashtable[hash] change it's value
      sht_info->hashTable[hash] = sht_info->lastBlockId;    // And then update the hashtable[hash] to the last block we are

      memcpy(data + SHT_RecordOffset(block_info), &record.name , sizeof(record.name));                            // Need to copy a name and an int to memory so 
      memcpy(data + SHT_RecordOffset(block_info) + sizeof(record.name), (void*) &block_id, sizeof(unsigned int)); // 2 memcpy and the offsets for this
      block_info->recNumber++;
    }else{
      memcpy(data + SHT_RecordOffset(block_info), &record.name , sizeof(record.name));                            // Need to copy a name and an int to memory so
      memcpy(data + SHT_RecordOffset(block_info) + sizeof(record.name), (void*) &block_id, sizeof(unsigned int)); // 2 memcpy and the offsets for this
      block_info->recNumber++;
    }
  }

  BF_Block_SetDirty(block);
  BF_Block_SetDirty(firstBlock);
  CALL_OR_DIE(BF_UnpinBlock(block));
  CALL_OR_DIE(BF_UnpinBlock(firstBlock));
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&firstBlock);

  return 0;
}

int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* sht_info, char* name){
  int total = 0;
  int noEntry = 0;

  void* data;
  void* dataHT;
  BF_Block* block;
  BF_Block* blockHT;

  BF_Block_Init(&block);
  BF_Block_Init(&blockHT);

  int hash = SHT_Function(name, sht_info->numBuckets);

  if(sht_info->hashTable[hash] == -1){            // Check before get_block if a block exists
    printf("There is no entry with this name!\n");
    return 0;
  }

  int* array = (int*) malloc(sizeof(int) * (ht_info->lastBlockId + 1));
  for(int i = 0; i <= ht_info->lastBlockId; i++){
    array[i] = 0;
  }

  int temp = sht_info->hashTable[hash]; // To go from block to block need to take a temporary because we cant change hashTable value at the end of the loop
  while(1){
    CALL_OR_DIE(BF_GetBlock(sht_info->fileDesc, temp, block));
    data = BF_Block_GetData(block);

    void* record = data;
    void* blockId = data + SHT_RecordNameOffset();
    SHT_block_info* block_info = data + SHT_MetadataOffset(sht_info);

    for(int i = 0; i < block_info->recNumber; i++){
      if((strcmp((char*) record, (char*) name) == 0) && (array[*(int*) blockId] != 1)){ // Check if this is the name but also if we visited that block previously
        CALL_OR_DIE(BF_GetBlock(ht_info->fileDesc, *(int*) blockId, blockHT));
        dataHT = BF_Block_GetData(blockHT);

        Record* rec = dataHT;
        HT_block_info* ht_block_info = dataHT + ht_info->maxBlockRecs * sizeof(Record);

        for(int j = 0; j < ht_block_info->recNumber; j++){
          if(strcmp(rec->name, (char*) record) == 0){
            printRecord(*rec);
            noEntry++;
          }
          rec = dataHT + sizeof(Record) * (j + 1);  // Going to the next record of the block
        }
        total++;

        CALL_OR_DIE(BF_UnpinBlock(blockHT));

        array[*(int*) blockId] = 1;   // Block visited so change the value 
      }
      record = data + (SHT_RecordNameOffset() + sizeof(unsigned int)) * (i + 1);   // Going to the next record & block id of the block
      blockId = record + SHT_RecordNameOffset();
    }
    total++;

    CALL_OR_DIE(BF_UnpinBlock(block));

    if(block_info->hashBucket == -1){
      break;
    }
    
    temp = block_info->hashBucket;  // Giving the next we point
  }

  if(noEntry == 0){
    printf("There is no entry with this name.\n");
  }
  
  free(array);
  BF_Block_Destroy(&block);
  BF_Block_Destroy(&blockHT);

  return total;
}