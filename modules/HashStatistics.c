#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "record.h"
#include "ht_table.h"
#include "sht_table.h"
#include "HashStatistics.h"

#define CALL_OR_DIE(call){  \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}

/**** The two possible file we can have ****/

static const char* secondaryString = "SHash file";
static const char* hashtableString = "Hashtable file";

/**** Offset functions  ****/

static int HT_InfoOffset(void){
  return strlen(hashtableString) + 1;
}

static int HT_MetadataOffset(HT_info* ht_info){
  return ht_info->maxBlockRecs * sizeof(Record);
}

static int SHT_InfoOffset(void){
  return strlen(secondaryString) + 1;
}

static int SHT_MetadataOffset(SHT_info* sht_info){
  return sht_info->maxBlockRecs * (sizeof(char) * 15 + sizeof(unsigned int));
}

/**** Stats functions ****/

int HashStatistics(char* fileName){
  int file;
  void* data;
  BF_Block* block;

  BF_Block_Init(&block);
  CALL_OR_DIE(BF_OpenFile(fileName, &file));
  CALL_OR_DIE(BF_GetBlock(file, 0, block));
  data = BF_Block_GetData(block);

  int* hashTable;
  int fileDesc = 0;
  int numBuckets = 0;
  int lastBlockId = 0;
  int maxBlockRecs = 0;
  int metaDataOffset = 0;

  /**** Checking what kind of file this is and initialize the values ****/

  if(strcmp((char*) data, (char*) hashtableString) == 0){
    HT_info* ht_info = data + HT_InfoOffset();
    fileDesc = ht_info->fileDesc;
    numBuckets = ht_info->numBuckets;
    lastBlockId = ht_info->lastBlockId;
    maxBlockRecs = ht_info->maxBlockRecs;
    metaDataOffset = HT_MetadataOffset(ht_info);

    hashTable = (int*) malloc(sizeof(int) * numBuckets);
    for(int i = 0; i < numBuckets; i++){
      hashTable[i] = ht_info->hashTable[i];
    }

    CALL_OR_DIE(BF_UnpinBlock(block));
  }else if(strcmp((char*) data, (char*) secondaryString) == 0){
    SHT_info* sht_info = data + SHT_InfoOffset();
    fileDesc = sht_info->fileDesc;
    numBuckets = sht_info->numBuckets;
    lastBlockId = sht_info->lastBlockId;
    maxBlockRecs = sht_info->maxBlockRecs;
    metaDataOffset = SHT_MetadataOffset(sht_info);
    
    hashTable = (int*) malloc(sizeof(int) * numBuckets);
    for(int i = 0; i < numBuckets; i++){
      hashTable[i] = sht_info->hashTable[i];
    }

    CALL_OR_DIE(BF_UnpinBlock(block));
  }else{
    printf("There is no file with this name.\n");

    CALL_OR_DIE(BF_UnpinBlock(block));
    BF_Block_Destroy(&block);

    return -1;
  }

  CALL_OR_DIE(BF_UnpinBlock(block));  // For not having memory leaks

  int overFlow = 0;
  int totalOverFlow = 0;

  int maximumRecs = 0;
  double averageRecs = 0;
  int minimumRecs = maxBlockRecs * lastBlockId; // Taking a very high value for the first time

  int maximumTemp = 0;
  int minimumTemp = 0;
  
  double averageBlockNumber = 0;

  /**** Minimum-Maximum-Average records per bucket ****/

  for(int i = 0; i < numBuckets; i++){
    if(hashTable[i] != -1){   // The hashtable must have a value

      int temp = hashTable[i];
      CALL_OR_DIE(BF_GetBlock(file, temp, block));

      maximumTemp = 0;  // Set our temp variable  
      minimumTemp = 0;  // to 0 for every bucket

      while(1){
        data = BF_Block_GetData(block);

        HT_block_info* block_info = data + metaDataOffset;

        averageRecs += block_info->recNumber;
        minimumTemp += block_info->recNumber;
        maximumTemp += block_info->recNumber;
        
        CALL_OR_DIE(BF_UnpinBlock(block));

        if(block_info->hashBucket == -1){
          break;
        }

        temp = block_info->hashBucket;
        CALL_OR_DIE(BF_GetBlock(fileDesc, temp, block));
      }
    }

    if(maximumTemp > maximumRecs){
      maximumRecs = maximumTemp;  // Keeping the maximum 
    }
    if(minimumTemp < minimumRecs){
      minimumRecs = minimumTemp;  // Keeping the minimum
    }
  }
  averageRecs = (double) averageRecs/numBuckets;

  /**** Overflow calculation *****/

  for(int i = 0; i < numBuckets; i++){
    overFlow = 0;

    int temp = hashTable[i];

    if(temp == -1){
      continue;  // Need to check if the block exists
    }

    CALL_OR_DIE(BF_GetBlock(file, temp, block));
    data = BF_Block_GetData(block);

    HT_block_info* block_info = data + metaDataOffset;

    if(block_info->hashBucket != -1){
      totalOverFlow++;
    }

    /**** Overflow calculation for every block ****/

    while(1){
      data = BF_Block_GetData(block);

      HT_block_info* block_info = data + metaDataOffset;

      overFlow++;

      CALL_OR_DIE(BF_UnpinBlock(block));

      if(block_info->hashBucket == -1){
        break;
      }

      temp = block_info->hashBucket;
      CALL_OR_DIE(BF_GetBlock(fileDesc, temp, block));
    }
    printf("Bucket %d has overflown by %d blocks\n", i, overFlow - 1);
  }

  averageBlockNumber = (double) (lastBlockId - 1)/numBuckets;
  
  printf("This file has : %d blocks\n", lastBlockId);
  printf("The minimum amount of records a bucket has : %d\n", minimumRecs);
  printf("The maximum amount of records a bucket has : %d\n", maximumRecs);
  printf("The average amount of records a bucket has : %.2f\n", averageRecs);
  printf("The total amount of blocks with overflow   : %d\n", totalOverFlow);
  printf("The average amount of blocks a bucket has  : %.2f\n", averageBlockNumber);

  free(hashTable);
  BF_Block_Destroy(&block);

  return HT_OK;
}