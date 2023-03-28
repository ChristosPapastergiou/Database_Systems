#ifndef BF_H
#define BF_H

#ifdef __cplusplus
extern "C" {
#endif

#define BF_BLOCK_SIZE 512      // Block size in bytes
#define BF_BUFFER_SIZE 100     // Maximum block number in memory
#define BF_MAX_OPEN_FILES 100  // Maximum number of open files

typedef enum BF_ErrorCode{
  BF_OK,
  BF_OPEN_FILES_LIMIT_ERROR,     // Already exists BF_MAX_OPEN_FILES files open
  BF_INVALID_FILE_ERROR,         // File ID number does not correspond to an open file
  BF_ACTIVE_ERROR,               // The BF level is active and it cant be initialized
  BF_FILE_ALREADY_EXISTS,        // File cant be created because it already exists
  BF_FULL_MEMORY_ERROR,          // Memory is full with active block
  BF_INVALID_BLOCK_NUMBER_ERROR, // Requested block doesnt exist in the file
  BF_AVAILABLE_PIN_BLOCKS_ERROR, // File cant close cause of active block in the memory
  BF_ERROR
} BF_ErrorCode;

typedef enum ReplacementAlgorithm{
  LRU,
  MRU
}ReplacementAlgorithm;

// Block struct
typedef struct BF_Block BF_Block;

// Initialize and allocate memory for BF_BLOCK 
void BF_Block_Init(BF_Block **block);

// Deallocate memory used by BF_BLOCK 
void BF_Block_Destroy(BF_Block **block);

// Changing the state of a block to dirty. The block data has  
// been changed and the BF layer will write the block back to disk when needed
// In case we simply read the data without changing it then there is no need to call the function.
void BF_Block_SetDirty(BF_Block *block);

// Returns a pointer to the Block data. If we change the data we have to 
// make the block dirty by calling of the BF_Block_GetData function
char* BF_Block_GetData(const BF_Block *block);

// Initialize the BF layer. We can choose between two Block replacement policies (LRU, MRU)
BF_ErrorCode BF_Init(const ReplacementAlgorithm repl_alg);

// Creates a file named filename which consists of blocks. If the file already exists then an error code is returned
// Returns BF_OK if successfull or an error code if failed 
// Call the BF_PrintError function to see the error
BF_ErrorCode BF_CreateFile(const char* filename);

// Opens an existing file from blocks named filename and returns the file ID in the file_desc variable
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error
BF_ErrorCode BF_OpenFile(const char* filename, int *file_desc);

// Close the open file with file ID file_desc
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error
BF_ErrorCode BF_CloseFile(const int file_desc);

// Taking file ID of an open file and finds the number of available block, and return it to block_num variable
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error
BF_ErrorCode BF_GetBlockCounter(const int file_desc, int *blocks_num);

// Allocate a new block for the file with ID number blockFile. The new block is always bound at the end of the file,
// so the block number is BF_getBlockCounter(file_desc) - 1. The bound block is pinned to memory 
// (pin) and returned to the block variable. When we no longer need this block 
// then we need to update the block level by calling the function BF_UnpinBlock
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error 
BF_ErrorCode BF_AllocateBlock(const int file_desc, BF_Block *block);

// Finds the block with block_num number of the open file_desc and returns it in the block variable
// The block bound is pinned to memory (pin). When we no longer need this block 
// then we need to update the block level by calling the function BF_UnpinBlock
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error 
BF_ErrorCode BF_GetBlock(const int file_desc, const int block_num, BF_Block *block);

// Releases the block from the Block layer which some arrow will write to disk
// Returns BF_OK if successfull or an error code if failed
// Call the BF_PrintError function to see the error 
BF_ErrorCode BF_UnpinBlock(BF_Block *block);

// Helps to print the errors that may occur when calling block level functions
// A description of the most error is printed to stderr 
void BF_PrintError(BF_ErrorCode err);

// Calls the Block layer by writing to disk any block had in memory
BF_ErrorCode BF_Close();

#ifdef __cplusplus
}
#endif
#endif
