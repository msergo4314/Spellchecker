#define _POSIX_C_SOURCE 200809L // most likely won't need
#define MAX_WORD_LENGTH 100
#define MAX_FILE_NAME_LENGTH 120
#define MAX_SIZE_USERINPUT 90
#define MAX_THREADS 16
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

pthread_mutex_t lock;
const char *debugFile = "debug.txt";
const char *username = "username";
// pthread_mutex_t lock; // do not use global
bool firstWrite = true;
const bool debugOutput = true; // flag for extra prints to file
const bool detailedDebug = false;
const bool doSorting = true;

typedef struct mistake {
  unsigned int countErrors;
  char *misspelledString;
  char *dictionaryName;
  char *fileName;
  unsigned int threadID;
} spellingError;

typedef struct threadArgs {
  char *dictionaryFileName;
  char *spellcheckFileName;
  int threadStatus; // this will be set directly by the thread
  spellingError *errorArray; // array of mistakes
  unsigned int threadID;
} threadArguments;

enum stringTypes {ALPHA_ONLY, INTEGER, FLOAT, MIXED};

_Atomic int prevSize = 0;
_Atomic unsigned int numThreadsStarted; // atomic int can only be updated atomically (by a single thread)
_Atomic unsigned int numThreadsFinished;
_Atomic unsigned int numThreadsInUse;

int openFileForReading(char *filename);
unsigned char validateUserInput(char *inputString);
void printFlush(const char *string, ...);
void convertEntireStringToLower(char *string);
void removeNewline(char *string);
char *readEntireFileIntoStr(const char *fileName, int *sizeBytes);
char** splitStringOnWhiteSpace(const char* inputString, int* wordCount);
char **readFileArray(const char *fileName, int *wordCount);
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