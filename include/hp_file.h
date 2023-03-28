#ifndef HP_FILE_H
#define HP_FILE_H

#include <record.h>

// Return code emuration
typedef enum HP_ErrorCode{
    HP_OK = 0, 
    HP_ERROR = -1
}HP_ErrorCode;    

// HP_info has informations about the heap file
typedef struct{
    int blockId;        // ID of the block
    int fileDesc;       // File ID
    int lastBlockId;    // ID of the last file's block 
    int maxBlockRecs;   // Max amount of records a block can have
}HP_info;

// HP_block_info has informations about the block
typedef struct{
    int recNumber;      // Number of records a block has 
    int nextBlock;      // Points to the next block_info
}HP_block_info;

// Create and properly initialize an empty heap file named fileName
// Return 0 if successfull, -1 if failure
int HP_CreateFile(char *fileName);

// Opens the file named filename and reads from the first block the information about the heap file
// Then, a structure is updated that holds as much information as deemed necessary 
// for this file in order to be able to edit then edit its records
HP_info* HP_OpenFile(char *fileName);

// Closes the file specified within the header_info structure
// The function is is also responsible for freeing the memory occupied
// by the structure passed as a parameter, in case the closure was successfully performed
// Return 0 if successfull, -1 if failure
int HP_CloseFile(HP_info* header_info);

// Insert a entry into the heap file, the information about the file is in the
// header_info structure while the record to be inserted is specified by the record structure
// Return the number of the block in which the insertion was made (blockId) if successfull, -1 if failure
int HP_InsertEntry(HP_info* header_info, Record record);

// Print all records that exist in the heap file that have a value in key field equal to id
// The first structure gives information about the heap, as it was returned by HP_OpenFile
// For each record that exists in the file and has a value in the id field equal to id, print it
// Also return the number of blocks that read until all records are found
// Return the number of readed blocks if successfull, -1 if failure
int HP_GetAllEntries(HP_info* header_info, int id);

#endif
