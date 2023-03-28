#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"

#define RECORDS_NUM 200     // Number of records in database
#define FILE_NAME "data.db"

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
  HP_CreateFile(FILE_NAME);
  HP_info* info = HP_OpenFile(FILE_NAME);

  printf("The file has been created successfully. Time to insert some random records.\n");

  Record record;
  for (int i = 0; i < RECORDS_NUM; ++i) {
    record = randomRecord();
    HP_InsertEntry(info, record);
  }

  printf("Done with inserts. Time to find some records.\n");

  int id;

  /* Existing (or not) values */

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, id), id);

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, id), id);

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, id), id);

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, id), id);

  id = rand() % RECORDS_NUM;
  printf("\nSearching for: %d\n", id);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, id), id);

  /* 100% non existing */

  int noEntry = RECORDS_NUM * 2;
  printf("\nSearching for: %d\n", noEntry);
  printf("Visited : %d blocks to find record with id %d.\n", HP_GetAllEntries(info, noEntry), noEntry);

  printf("\nDone with reading. Time to close the file.\n");

  if(HP_CloseFile(info) == 0){
    printf("\nFile %s closed successfully\n", FILE_NAME);
  }
  BF_Close();

  remove(FILE_NAME);

  return 0;
}
