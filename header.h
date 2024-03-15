#define _POSIX_C_SOURCE 200809L // most likely won't need
#define MAX_WORD_LENGTH 60
#define MAX_FILE_NAME_LENGTH 60
#define MAX_SIZE_USERINPUT 90
#define MAX_THREADS 16
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
#include <limits.h> // needed to check chartype (possibly not necessary)

pthread_mutex_t lock;
const char *debugFile = "debug.txt";
const char *username = "username";
// pthread_mutex_t lock; // do not use global
bool firstWrite = true;
const bool debugOutput = true; // flag for extra prints to file
const bool detailedDebug = true;
const bool verySlowAndVeryDetailedDebug = true;
const bool doSorting = false;

typedef struct mistake {
  unsigned int countErrors;
  char *misspelledString;
  char *dictionaryName;
  char *fileName;
  unsigned int threadID;
} spellingError;

// functionally both input and output for thread functions
typedef struct threadArgs {
  char *dictionaryFileName;
  char *spellcheckFileName;
  int threadStatus; // this will be set directly by the thread
  spellingError *errorArray; // array of mistakes appended by each thread
  // unsigned int threadID;
  unsigned int prevSize; // previous size of mistakes array (needed to realloc)
  unsigned int numThreadsStarted;
  unsigned int numThreadsFinished;
  unsigned int numThreadsInUse;
} threadArguments;

enum stringTypes {ALPHA_ONLY, INTEGER, FLOAT, MIXED};

// _Atomic unsigned int numThreadsStarted; // atomic varaibles are also an option if you don't want to use mutexes
// _Atomic unsigned int numThreadsFinished;
// _Atomic unsigned int numThreadsInUse;

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
void *threadFunction(void *vargp);
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
int partitionStr(char **arr, int start, int end);
void quickSortStr(char ** arr, int start, int end);
char* getNonAlphabeticalCharsString();
unsigned int numStringMismatchesInStrings(const char *dictionaryString, const char *target);
char* getOutputString (threadArguments threadArgsPtr);
unsigned int countMistakesForThread(spellingError *errorArr, unsigned int numEntriesInArr, int index);
const char *getFileNameFromThreadID(spellingError *arr, int index, unsigned int numElements);
const char *getMistakeAtIndex(spellingError *arr, int threadNum, int index, unsigned int numElements);
char *generateSummary(spellingError *errorArr, unsigned int numThreads, unsigned int arrayLength, char *inputString);
