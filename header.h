#define _POSIX_C_SOURCE 200809L // most likely won't need
#define MAX_WORD_LENGTH 50
#define MAX_FILE_NAME_LENGTH 50
#define MAX_SIZE_USERINPUT 100
#define MAX_THREADS 16 // depends on machine so should be sure to check your machine
#define MAX_OUTPUT_MESSAGE_SIZE 5000 // in case the filenames are really damn big
#include <ctype.h> // tolower
#include <pthread.h> // multithreading
#include <stdatomic.h> // for atomic values
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // string functions (strdup, strtoui, etc.)
#include <string.h>
#include <fcntl.h> // for file open/read
#include <unistd.h>
#include <stdarg.h> // for variable # of function inputs
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

const bool debugOutput = true; // flag for extra prints to file
const bool detailedDebug = false;
const bool doSorting = false; // not very useful but it's here. Enables/disables sorting of file array (could be useful for a binary search instead of brute force)

typedef struct mistake {
  unsigned int countErrors;
  char *misspelledString;
  char *dictionaryName;
  char *fileName;
  unsigned int threadID;
} spellingError;

// functionally the struct is both input and output for thread functions
typedef struct threadArgs {
  char *dictionaryFileName;
  char *spellcheckFileName;
  unsigned int threadSucccessCount; // this will be set directly by the threads to track success/failure
  spellingError *errorArray; // array of mistakes appended by each thread
  unsigned int prevSize; // previous size of mistakes array (needed to realloc)
  unsigned int numThreadsStarted;
  unsigned int numThreadsFinished;
  unsigned int numThreadsInUse;
} threadArguments;

enum stringTypes {ALPHA_ONLY, INTEGER, FLOAT, MIXED};

// _Atomic unsigned int numThreadsStarted; // atomic varaibles are also an option if you don't want to use mutexes (but can't be used for everything anyway)
// _Atomic unsigned int numThreadsFinished;
// _Atomic unsigned int numThreadsInUse;

void *threadFunction(void *vargp);
int openFileForReading(char *filename);
unsigned char validateUserInput(char *inputString);
void printFlush(const char *string, ...);
void convertEntireStringToLower(char *string);
void removeNewline(char *string);
char *readEntireFileIntoStr(const char *fileName, unsigned int *sizeBytes);
char** splitStringOnWhiteSpace(const char* inputString,unsigned int* wordCount);
char **readFileArray(const char *fileName, unsigned int *wordCount);
void free2DArray(void ***addressOfGenericPointer, int numberOfInnerElements);
void freePointer(void **addressOfGenericPointer);
unsigned int min(unsigned int a, unsigned int b);
spellingError *compareFileData(const char **dictionaryData, const char ** fileData, const unsigned int numEntriesDictionary,
const unsigned int numEntriesInFile, unsigned int *countTotalMistakes, unsigned int *countInArr);
void freeArrayOfSpellingErrors(spellingError **arrayOfMistakes, unsigned int countInArr);
unsigned int numStringMismatchesInArrayOfStrings(const char **arrayOfDictionaryStrings, const char **arrayOfFileStrings,
                                              unsigned int sizeDictionary, unsigned int sizeFile,  const char *target);
bool verifySortedStr(const char ** sortedArrayOfStrings, const unsigned int numStrings);
bool verifySortedSpellingErrors(const spellingError *arrayOfSpellingErrors, const int numElements);
int partitionSpellingErrorArr(spellingError *arr, int start, int end);
void quickSortSpellingErrorArr(spellingError *arr, int start, int end);
int printToLog(const char *debugFile, const char *stringLiteral, ...);
char* getNonAlphabeticalCharsString(void);
unsigned int max(unsigned int a, unsigned int b);
unsigned int numStringMismatchesInStrings(const char *dictionaryString, const char *target);
char* getOutputString (threadArguments threadArgsPtr);
unsigned int countMistakesForThread(spellingError *errorArr, unsigned int numEntriesInArr, int index);
const char *getFileNameFromThreadID(spellingError *arr, int index, unsigned int numElements);
const char *getMistakeAtIndex(spellingError *arr, int threadNum, int index, unsigned int numElements);
char *generateSummary(spellingError *errorArr, unsigned int numThreads, unsigned int arrayLength, char *inputString, unsigned int numSuccessfulThreads);
int writeThreadToFile(const char *fileName, spellingError *listOfMistakes, unsigned int numElements);
void printToOutputFile(const char *fileName, const char* stringToPrint);
unsigned int getNumDigitsInNumber(int number, char base);
void quicksortStrings(char **arrayofStrings, int left, int right);