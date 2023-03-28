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

/*
 * Η συνάρτηση BF_CreateFile δημιουργεί ένα αρχείο με όνομα filename το
 * οποίο αποτελείται από blocks. Αν το αρχείο υπάρχει ήδη τότε επιστρέφεται
 * κωδικός λάθους. Σε περίπτωση επιτυχούς εκτέλεσης της συνάρτησης επιστρέφεται
 * BF_OK, ενώ σε περίπτωση αποτυχίας επιστρέφεται κωδικός λάθους. Αν θέλετε να
 * δείτε το είδος του λάθους μπορείτε να καλέσετε τη συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_CreateFile(const char* filename);

/*
 * Η συνάρτηση BF_OpenFile ανοίγει ένα υπάρχον αρχείο από blocks με όνομα
 * filename και επιστρέφει το αναγνωριστικό του αρχείου στην μεταβλητή
 * file_desc. Σε περίπτωση επιτυχίας επιστρέφεται BF_OK ενώ σε περίπτωση
 * αποτυχίας, επιστρέφεται ένας κωδικός λάθους. Αν θέλετε να δείτε το είδος
 * του λάθους μπορείτε να καλέσετε τη συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_OpenFile(const char* filename, int *file_desc);

/*
 * Η συνάρτηση BF_CloseFile κλείνει το ανοιχτό αρχείο με αναγνωριστικό αριθμό
 * file_desc. Σε περίπτωση επιτυχίας επιστρέφεται BF_OK ενώ σε περίπτωση
 * αποτυχίας, επιστρέφεται ένας κωδικός λάθους. Αν θέλετε να δείτε το
 * είδος του λάθους μπορείτε να καλέσετε τη συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_CloseFile(const int file_desc);

/*
 * Η συνάρτηση Get_BlockCounter δέχεται ως όρισμα τον αναγνωριστικό αριθμό
 * file_desc ενός ανοιχτού αρχείου από block και βρίσκει τον αριθμό των
 * διαθέσιμων blocks του, τον οποίο και επιστρέφει στην μεταβλητή blocks_num.
 * Σε περίπτωση επιτυχίας επιστρέφεται BF_OK ενώ σε περίπτωση αποτυχίας,
 * επιστρέφεται ένας κωδικός λάθους. Αν θέλετε να δείτε το είδος του λάθους
 * μπορείτε να καλέσετε τη συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_GetBlockCounter(const int file_desc, int *blocks_num);

/*
 * Με τη συνάρτηση BF_AllocateBlock δεσμεύεται ένα καινούριο block για το
 * αρχείο με αναγνωριστικό αριθμό blockFile. Το νέο block δεσμεύεται πάντα
 * στο τέλος του αρχείου, οπότε ο αριθμός του block είναι
 * BF_getBlockCounter(file_desc) - 1. Το block που δεσμεύεται καρφιτσώνεται
 * στην μνήμη (pin) και επιστρέφεται στην μεταβλητή block. Όταν δεν το
 * χρειαζόμαστε άλλο αυτό το block τότε πρέπει να ενημερώσουμε τον επίπεδο
 * block καλώντας την συνάρτηση BF_UnpinBlock. Σε περίπτωση επιτυχίας
 * επιστρέφεται BF_OK ενώ σε περίπτωση αποτυχίας, επιστρέφεται ένας κωδικός
 * λάθους. Αν θέλετε να δείτε το είδος του λάθους μπορείτε να καλέσετε τη
 * συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_AllocateBlock(const int file_desc, BF_Block *block);


/*
 * Η συνάρτηση BF_GetBlock βρίσκει το block με αριθμό block_num του ανοιχτού
 * αρχείου file_desc και το επιστρέφει στην μεταβλητή block. Το block που
 * δεσμεύεται καρφιτσώνεται στην μνήμη (pin). Όταν δεν χρειαζόμαστε άλλο αυτό
 * το block τότε πρέπει να ενημερώσουμε τον επίπεδο block καλώντας την συνάρτηση
 * BF_UnpinBlock. Σε περίπτωση επιτυχίας επιστρέφεται BF_OK ενώ σε περίπτωση
 * αποτυχίας, επιστρέφεται ένας κωδικός λάθους. Αν θέλετε να δείτε το είδος του
 * λάθους μπορείτε να καλέσετε τη συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_GetBlock(const int file_desc, const int block_num, BF_Block *block);

/*
 * Η συνάρτηση BF_UnpinBlock αποδεσμεύει το block από το επίπεδο Block το
 * οποίο κάποια στηγμή θα το γράψει στο δίσκο. Σε περίπτωση επιτυχίας
 * επιστρέφεται BF_OK ενώ σε περίπτωση αποτυχίας, επιστρέφεται ένας κωδικός
 * λάθους. Αν θέλετε να δείτε το είδος του λάθους μπορείτε να καλέσετε τη
 * συνάρτηση BF_PrintError.
 */
BF_ErrorCode BF_UnpinBlock(BF_Block *block);

/*
 * Η συνάρτηση BF_PrintError βοηθά στην εκτύπωση των σφαλμάτων που δύναται να
 * υπάρξουν με την κλήση συναρτήσεων του επιπέδου αρχείου block. Εκτυπώνεται
 * στο stderr μια περιγραφή του πιο σφάλματος.
 */
void BF_PrintError(BF_ErrorCode err);

// Calls the Block layer by writing to disk any block had in memory
BF_ErrorCode BF_Close();

#ifdef __cplusplus
}
#endif
#endif
