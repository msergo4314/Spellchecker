// #ifndef "header.h"
#define _POSIX_C_SOURCE 200809L
#define MAX_WORD_LENGTH 40 // experimentally it's actually 32
#define MAX_FILE_NAME_LENGTH 50
#define MAX_SIZE_USERINPUT 100
#include <ctype.h> // tolower, isalpha, etc
#include <unistd.h>
#include <pthread.h> // multithreading
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // malloc and free
#include <string.h> // string functions (strdup, strtoui, etc.)
#include <fcntl.h> // for file open/read
#include <stdarg.h> // for variable # of function inputs
// #include <stdatomic.h> // for atomic values
// #include <omp.h> // only needed to check how many threads you have
#include <limits.h> // needed to check chartype
#include <time.h> // optional (for timing)

// global variables -- many of them can be placed in the struct
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t updatedVariables = PTHREAD_COND_INITIALIZER; // global variable to track when struct updates

const char *debugFile = "debug.txt";
const char *threadOutputFile = "username.out";
bool firstWriteDebug = true;
bool firstWriteThreadOutput = true;

const bool debugOutput = false; // flag for extra prints to file
const bool detailedDebug = false;
const char* dictionaryAbsPath = "/mnt/c/global C/dictionary.txt";

typedef unsigned int uint; // slightly easier to type

typedef struct mistake {
  uint countErrors;
  char *misspelledString;
  char *dictionaryName;
  char *fileName;
  uint threadID;
} spellingError;

// functionally this struct is both input and output for thread functions
typedef struct threadArgs {
  char *dictionaryFileName;
  char *spellcheckFileName;
  uint threadSucccessCount; // this will be set directly by the threads to track success/failure
  spellingError *errorArray; // array of mistakes appended by each thread
  uint prevSize; // previous size of mistakes array (needed to realloc)
  uint numThreadsStarted;
  uint numThreadsFinished;
  uint numThreadsInUse;
} threadArguments;

enum StringTypes {ALPHA_ONLY, UINTEGER, FLOAT, MIXED};
enum ReturnTypes {FAILURE = EXIT_FAILURE, SUCCESS = EXIT_SUCCESS, ALTERNATE_SUCCESS = -1};

// _Atomic uint numThreadsStarted; // atomic varaibles are also an option if you don't want to use mutexes (but can't be used for everything anyway)
// _Atomic uint numThreadsFinished;
// _Atomic uint numThreadsInUse;

typedef struct {
  char *key;
  uint occurrences;
} StringEntry;

typedef struct HashNode {
  StringEntry entry;
  struct HashNode *next;
} HashNode;

typedef struct {
  HashNode **buckets;
  size_t size;
} HashMap;

void *threadFunction(void *vargp);
unsigned char validateUserInput(char *inputString);
void printFlush(const char *string, ...);
void convertEntireStringToLower(char *string);
void removeNewline(char *string);
char *readEntireFileIntoStr(const char *fileName, uint *sizeBytes);
char** splitStringOnWhiteSpace(const char* inputString,uint* wordCount);
char **readFileArray(const char *fileName, uint *wordCount);
void free2DArray(void ***addressOfGenericPointer, int numberOfInnerElements);
void freePointer(void **addressOfGenericPointer);
spellingError *compareFileData(char **dictionaryData, char **fileData, const uint numEntriesDictionary,
const uint numEntriesFile, uint *countTotalMistakes, uint *countInArr);
void freeArrayOfSpellingErrors(spellingError **arrayOfMistakes, uint countInArr);
uint numStringMismatchesInArrayOfStrings(const char **arrayOfDictionaryStrings, const char **arrayOfFileStrings,
uint sizeDictionary, uint sizeFile,  const char *target);
int partitionSpellingErrorArr(spellingError *arr, int start, int end);
void quickSortSpellingErrorArr(spellingError *arr, int start, int end);
int printToLog(const char *debugFile, const char *stringLiteral, ...);
void getNonAlphabeticalCharsString(char *buffer);
int max(int a, int b);
char* getOutputString (threadArguments threadArgsPtr);
uint countMistakesForThread(spellingError *errorArr, uint numEntriesInArr, int index);
const char *getFileNameFromThreadID(spellingError *arr, int index, uint numElements);
const char *getMistakeAtIndex(spellingError *arr, uint threadNum, uint index, uint numElements);
char *generateSummary(spellingError *errorArr, uint numThreads, uint arrayLength, char *inputString, uint numSuccessfulThreads);
int writeThreadToFile(const char *fileName, spellingError *listOfMistakes, uint numElements);
void printToOutputFile(const char *fileName, const char* stringToPrint);
uint getNumDigitsInPositiveNumber(uint number, unsigned char base);
void quicksortStrings(char **arrayofStrings, int left, int right);
uint numberOfStringMatchesInArrayOfStrings(const char **arrayOfStrings, uint numStrings, const char *target);
