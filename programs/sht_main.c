#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "ht_table.h"
#include "sht_table.h"

#define RECORDS_NUM 200       // Number of records in database
#define FILE_NAME "data.db"
#define INDEX_NAME "index.db"

#define CALL_OR_DIE(call){  \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {      \
    BF_PrintError(code);    \
    exit(code);             \
  }                         \
}

int main(){
  srand(12569874);
  // srand(time(NULL));

  BF_Init(LRU);
  HT_CreateFile(FILE_NAME,10);
  SHT_CreateSecondaryIndex(INDEX_NAME, 10, FILE_NAME);

  HT_info* info = HT_OpenFile(FILE_NAME);
  SHT_info* index_info = SHT_OpenSecondaryIndex(INDEX_NAME);

  printf("The files have been created successfully. Time to insert some random records.\n");

  char searchName[15];
  Record record = randomRecord();
  strcpy(searchName, record.name);

  for(int i = 0; i < RECORDS_NUM; i++){
    record = randomRecord();
    int block_id = HT_InsertEntry(info, record);
    SHT_SecondaryInsertEntry(index_info, record, block_id);
  }

  printf("Done with inserts. Time to find some records.\n");

  char* name;

  /* Existing (or not) values */

  name = "Vagelis";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  name = "Christofos";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  name = "Marianna";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  name = "Konstantina";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  name = "Iosif";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  /* 100% non existing */

  name = "Christos";
  printf("\nSearching for: %s\n", name);
  printf("Visited : %d blocks to find record with name %s.\n", SHT_SecondaryGetAllEntries(info, index_info, name), name);

  printf("\nDone with reading. Time to close the file.\n");

  if(SHT_CloseSecondaryIndex(index_info) == 0){
    printf("\nFile %s closed successfully\n", INDEX_NAME);
  }
  if(HT_CloseFile(info) == 0){
    printf("File %s closed successfully\n", FILE_NAME);
  }
  BF_Close();

  remove(FILE_NAME);
  remove(INDEX_NAME);

  return 0;
}