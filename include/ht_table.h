#ifndef HT_TABLE_H
#define HT_TABLE_H

#include <record.h>

// Return code emuration
typedef enum HT_ErrorCode{
    HT_OK = 0,
    HT_ERROR = -1
}HT_ErrorCode;

// HT_info has informations about the hastable file
typedef struct{
    int blockId;            // ID of the block
    int fileDesc;           // File ID
    int lastBlockId;        // ID of the last file's block
    int maxBlockRecs;       // Max amount of records a block can have
    long int numBuckets;    // Buckets of our hashtable
    int hashTable[];        // Hashtable array
}HT_info;

// HT_block_info has informations about the block
typedef struct{
    int recNumber;          // Number of records a block has
    int hashBucket;         // Storing the int value of another block (E.g. we will visit block 10 -> block 4 -> block 1 because their hash is the same)
}HT_block_info;

// Create a file and proper initialization of an empty hash file with the name fileName
// It takes as input parameters the name of the file in which to
// build the heap and the number of buckets of the hash function
// Return 0 if successfull, -1 if failure
int HT_CreateFile(char *fileName, int buckets);

// Opens the file named filename and reads from the first block the information about the hashtable file
// Then, a structure is updated that holds as much information as deemed necessary 
// for this file in order to be able to edit then edit its records
// In case of an error then it returns NULL
HT_info* HT_OpenFile(char *fileName);

// Closes the file specified in in the header_info structure
// The function is is also responsible for freeing the memory occupied
// by the structure passed as a parameter, in case the closure was successfully performed
// Return 0 if successfull, -1 if failure
int HT_CloseFile(HT_info* header_info);

// Insert a entry into the hashtable file, the information about the file is in the
// header_info structure while the record to be inserted is specified by the record structure
// Return the number of the block in which the insertion was made (blockId) if successfull, -1 if failure
int HT_InsertEntry(HT_info* header_info, Record record);

// Print all records that exist in the hashtable file that have a value in key field equal to value
// The first structure gives information about the hashtable, as it was returned by HT_OpenFile
// For each record that exists in the file and has a value in the id field equal to value, print it
// Also return the number of blocks that read until all records are found
// Return the number of readed blocks if successfull, -1 if failure
int HT_GetAllEntries(HT_info* header_info, int value);

#endif
