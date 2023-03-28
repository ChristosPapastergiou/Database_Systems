#ifndef SHT_TABLE_H
#define SHT_TABLE_H

#include <record.h>
#include <ht_table.h>

// Return code emuration
typedef enum SHT_ErrorCode{
    SHT_OK = 0,
    SHT_ERROR = -1
}SHT_ErrorCode;

// SHT_info has informations about the secondary hastable file
typedef struct{
    int blockId;            // ID of the block
    int fileDesc;           // File ID
    int lastBlockId;        // ID of the last file's block
    int maxBlockRecs;       // Max amount of records a block can have
    long int numBuckets;    // Buckets of our hashtable
    int hashTable[];        // Hashtable array
}SHT_info;

// SHT_block_info has informations about the block
typedef struct{
    int recNumber;          // Number of records a block has
    int hashBucket;         // Storing the int value of another block (E.g. we will visit block 10 -> block 4 -> block 1 because their hash is the same)
}SHT_block_info;

// Create a file and proper initialization of an empty secondary hash 
// file with the name sfileName and filename for hash file
// It takes as input parameters the name of the file in which to
// build the heap and the number of buckets of the hash function
// Return 0 if successfull, -1 if failure
int SHT_CreateSecondaryIndex(char *sfileName, int buckets, char* fileName);

/* Η συνάρτηση SHT_OpenSecondaryIndex ανοίγει το αρχείο με όνομα sfileName
και διαβάζει από το πρώτο μπλοκ την πληροφορία που αφορά το δευτερεύον
ευρετήριο κατακερματισμού.*/

// Opens the file named sfilename and reads from the first  
// block the information about the secondary hashtable file
SHT_info* SHT_OpenSecondaryIndex(char *sfileName);

// Closes the file specified in in the header_info structure
// The function is is also responsible for freeing the memory occupied
// by the structure passed as a parameter, in case the closure was successfully performed
// Return 0 if successfull, -1 if failure
int SHT_CloseSecondaryIndex(SHT_info* header_info);

// Insert a entry into the hashtable file, the information about the file is in the
// header_info structure while the record to be inserted is specified by the record structure
// Return 0 if successfull, -1 if failure
int SHT_SecondaryInsertEntry(SHT_info* header_info, Record record, int block_id);

// Print all records that exist in the hashtable file that have a value in key field equal to name
// The first structure gives information about the hashtable and the second gives information about the secondary hashtable
// For each record that exists in the file and has a value in the id field equal to value, print it
// Also return the number of blocks that read until all records are found
// Return the number of readed blocks if successfull, -1 if failure
int SHT_SecondaryGetAllEntries(HT_info* ht_info, SHT_info* header_info, char* name);

#endif