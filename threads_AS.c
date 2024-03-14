#include "headers.h"

// pthread_t *threadIDs = NULL;

int main(int argc,char **argv) {

  spellingError *allMistakes = NULL;
  if (pthread_mutex_init(&lock, NULL) != 0) { 
    printf("\n mutex init has failed\n"); 
    return 1; 
  } 
  char userInput[MAX_SIZE_USERINPUT + 1] = "";
  unsigned char x = 0;
  pthread_t *threadIDs = NULL;

  main_menu:
  printf("\n1. Start a new spellchecking task\n2. Exit\n\n");
  printf("Select the mode of operation (1 or 2): ");
  fgets(userInput, sizeof(userInput), stdin);
  while ((x = validateUserInput(userInput)) != INTEGER) {
  input_loop:
    // printf("Invalid. You entered a string of type %s. Try again: ", returnTypeStrings[x]);
    printf("Invalid. Enter integer between 1 and 2: ");
    fgets(userInput, sizeof(userInput), stdin);
  }
  unsigned char selection = (unsigned char)strtoul(userInput, NULL, 10);
  switch(selection) {
    case 1:
      if (numThreadsInUse >= MAX_THREADS) {
        printf("too many threads in use\n");
        goto case_2;
      }
      // get dictionary to process
      pthread_mutex_lock(&lock);
      char fileNameString[MAX_FILE_NAME_LENGTH + 1];
      printf("Enter the filename of the dictionary (or 'Q' to quit): ");
      fgets(fileNameString, sizeof(fileNameString), stdin);
      pthread_mutex_unlock(&lock);
      removeNewline(fileNameString);
      char qString[] = {'q', 0x0};
      convertEntireStringToLower(fileNameString);
      if (!strcmp(fileNameString, qString)) {
        printf("Exiting...\n");
        goto main_menu;
      }
      // struct for thread arguments
      threadArguments args;
      pthread_mutex_lock(&lock);
      args.dictionaryFileName = strdup(fileNameString);
      pthread_mutex_unlock(&lock);
      // get file to process
      printf("Enter the filename of the input file (or 'Q' to quit): ");
      fgets(fileNameString, sizeof(fileNameString), stdin);
      removeNewline(fileNameString);
      convertEntireStringToLower(fileNameString);
      if (!strcmp(fileNameString, qString)) {
        free(args.dictionaryFileName);
        printf("Exiting...\n");
        goto main_menu;
      }
      pthread_mutex_lock(&lock);
      args.spellcheckFileName = strdup(fileNameString);
      args.errorArray = NULL;
      pthread_mutex_unlock(&lock);
      threadIDs = (pthread_t *)realloc(threadIDs, (sizeof(pthread_t) * (numThreadsStarted + 1)));
      for (int i = 0; i < numThreadsStarted + 1; i++) {
        threadIDs[i] = 0;
      }
      if (!threadIDs) {
        printf("failed to realloc for threadIDs\n");
        free(args.dictionaryFileName);
        free(args.spellcheckFileName);
        pthread_mutex_destroy(&lock);
        return(EXIT_FAILURE);
      }
      printf("making thread %d\n", numThreadsStarted + 1);
      int error;
      error = pthread_create(&(threadIDs[numThreadsStarted]), NULL, threadFunction, (void *)&args);
      if (error != 0) {
        printf("Thread can't be created: %s\n", strerror(error));
        free(args.dictionaryFileName);
        free(args.spellcheckFileName);
        free(threadIDs);
        pthread_mutex_destroy(&lock);
        return(EXIT_FAILURE);
      }
      goto main_menu;
    case 2:
      case_2:
      // int status;
      printf("exiting program\n");
      allMistakes = args.errorArray;
      if (threadIDs) {
        for(int i = 0; i < numThreadsStarted; i++) {
          printf("joining thread %d\n", i+1);
          pthread_join(threadIDs[i], NULL);
        }
        free(allMistakes);
        free(threadIDs);
      }
      pthread_mutex_destroy(&lock);
      return(EXIT_SUCCESS); // do not need break since end of main
    default:
      printf("not valid. Try again\n");
      goto input_loop;
  }
}

void removeNewline(char *string) {
  const unsigned int length = strlen(string);
  if (string[length - 1] == '\n') {
    string[length - 1] = '\0';
  }
}

char *readEntireFileIntoStr(const char *fileName, int *sizeBytes) {
  // assume fd is valid
  int fd;
  if ((fd = open(fileName, O_RDONLY)) == -1) {
    printf("bad file descriptor. File most likely does not exist\n");
    return NULL;
  }
  int size = lseek(fd, 0, SEEK_END);                    // sizeof file in bytes
  char *readString = malloc(sizeof(char) * (size + 1)); // one byte extra for \0
  if (readString == NULL) {
    close(fd);
    perror("fatal error, malloc failed!\n");
    return NULL;
  }
  if (lseek(fd, 0, SEEK_SET) == -1) {
    free(readString);
    close(fd);
    perror("lseek failed!\n");
    return NULL;
  }
  int numBytesRead = read(fd, readString, size);
  if (numBytesRead != size) {
    free(readString);
    close(fd);
    fprintf(stderr, "read failed! Expected %d bytes, received %d\n", size,
            numBytesRead);
    return NULL;
  }
  readString[size] = '\0'; // manually add the null terminator
  *sizeBytes = size;
  close(fd);
  return readString;
}

unsigned char validateUserInput(char *inputString) {
  removeNewline(inputString);
  int digit = 0, chars = 0;
  bool hasDecimal = false;
  for (int i = 0; i < strlen(inputString); i++) {
    if (isdigit(inputString[i])) {
      digit++;
    }
    else if (inputString[i] == '.') {
      hasDecimal = true;
      continue;
    }
    else {
      chars++;  
    }
  }
  if (hasDecimal && (digit >= 2) && !chars) {
    return FLOAT;
  }
  if (digit && !chars && !hasDecimal) {
    return INTEGER;
  }
  if (chars && !digit) {
    return ALPHA_ONLY;
  }
  return MIXED;
}

void printFlush(const char *string, ...) {
    va_list args;
    va_start(args, string);
    vfprintf(stdout, string, args);
    va_end(args);
    fflush(stdout);
}

char **readFileArray(const char *fileName, int *wordCount) {
  int sizeBytes, numWords;
  char **stringArrToReturn = NULL;
  char *singleFileString;
  if (!(singleFileString = readEntireFileIntoStr(fileName, &sizeBytes))) {
    return NULL;
  }
  // printf("there are %d bytes in the text file %s\n", sizeBytes, fileName);
  convertEntireStringToLower(singleFileString);
  stringArrToReturn = splitStringOnWhiteSpace(singleFileString, &numWords);
  if (stringArrToReturn == NULL) {
    free(singleFileString);
    return NULL;
  }
  free(singleFileString);
  *wordCount = numWords;
  return stringArrToReturn;
}

void convertEntireStringToLower(char *string) {
  for (long long unsigned int index = 0; string[index]; index++) {
    if (isalpha(string[index])) {
      string[index] = tolower(string[index]);
    }
  }
}

char* getNonAlphabeticalCharsString() {
    char* nonAlphaString = (char*)malloc(128 * sizeof(char)); // Assuming ASCII
    if (!nonAlphaString) {
      return NULL;
    }
    if (nonAlphaString == NULL) {
        perror("Memory allocation failed");
        return NULL;
    }
    nonAlphaString[0] = '\0'; // Initialize as empty string
    
    for (int i = 0; i < 128; i++) { // Assuming ASCII
        if (!isalpha(i)) {
            char temp[2] = {i, '\0'}; // Convert character to string
            strcat(nonAlphaString, temp); // Concatenate to result string
        }
    }
    return nonAlphaString;
}

char** splitStringOnWhiteSpace(const char* inputString, int* wordCount) {
    // Count the number of words
    if (!inputString || !wordCount) {
      printf("passed in NULL\n");
      return NULL;
    }
    *wordCount = 0;
    bool prevWasNotAlpha = true;
    const char* ptr;
    for (ptr = inputString; *ptr != '\0'; ptr += sizeof(char)) {
        // chars that seperate words: whitespace, special chars, punctuation, digits
        // strchr(" \t\r\n\v\f\xE2\x80\xA8\xE2\x80\xA9\xA0", *ptr)
        if (!isalpha(*ptr)) {
          // strchr will return an address if it finds the char in the string
          if (prevWasNotAlpha) {
            continue;
          }
          prevWasNotAlpha = true;
          // printf("new word: %x\n", *ptr);
          (*wordCount)++;
        }
        else {
          prevWasNotAlpha = false;
        }
    }
    if (!prevWasNotAlpha) {
      (*wordCount)++; // Account for the last word at EOF
    }
    // printf("number of words is: %d\n", *wordCount);
    if (*wordCount == 0) {
      printf("no words in file. Returning early...\n");
      return NULL;
    }
    // Allocate memory for array of strings
    char **words = (char **)malloc((*wordCount) * sizeof(char *));
    if (words == NULL) {
        printToLog(debugFile, "Memory allocation failed for words arr\n");
        return NULL;
    }

    char *delimiters = getNonAlphabeticalCharsString();
    if (!delimiters) {
      free2DArray((void***)&words, *wordCount);
      return NULL;
    }
    // printf("delims: %s\n", delimiters);
    char* token = strtok((char *)inputString, delimiters); // cast to char *
    int index = 0;
    while (token != NULL) {
        words[index] = strdup(token); // Allocate memory for each word
        if (words[index] == NULL) {
            printToLog(debugFile, "Memory allocation failed for strdup inside splitOnWhitespace");
            free2DArray((void ***)&words, index);
            return NULL;
        }
        index++;
        token = strtok(NULL, delimiters);
    }
    free(delimiters);
    return words;
}

void *threadFunction(void *vargp) {
  threadArguments *data = (threadArguments *)vargp;
  pthread_mutex_lock(&lock);
  numThreadsInUse++;
  numThreadsStarted++;
  if (numThreadsStarted == 1) {
    prevSize = 0;
  }
  char *dictionaryFileName = data->dictionaryFileName;
  char *spellcheckFileName = data->spellcheckFileName;
  data->threadID = numThreadsStarted;
  data->threadStatus = 0;
  pthread_mutex_unlock(&lock);
  // if (debugOutput) {
  //   printToLog(debugFile, "attempting to read file %s\n", dictionaryFileName);
  //   if (detailedDebug) {
  //     printToLog(debugFile, "split file is:\n\n");
  //   }
  // }
  int wordCountDictionary = 0;
  char **dictionaryArrayOfStrings = readFileArray(dictionaryFileName, &wordCountDictionary);
  if (!dictionaryArrayOfStrings) {
    printToLog(debugFile, "Reading words array from dictionary file failed!\n");
    goto exit_failure;
  }
  // if (debugOutput) {
  //   printToLog(debugFile, "wordcount is: %d\n", wordCountDictionary);
  //   if (detailedDebug) {
  //     for(int i = 0; i < wordCountDictionary; i++) {
  //       printToLog(debugFile, "%s\n", dictionaryArrayOfStrings[i]);
  //     }
  //   }
  // }

  /*****************************SECOND FILE******************************/

  // if (debugOutput) {
  //   printToLog(debugFile, "\nattempting to read file %s\n", spellcheckFileName);
  //   if (detailedDebug) {
  //     printToLog(debugFile, "split file is:\n\n");
  //   }
  // }
  int wordCountFile = 0;
  char **fileArrayOfStrings = readFileArray(spellcheckFileName, &wordCountFile);
  if (fileArrayOfStrings == NULL) {
    printToLog(debugFile, "Reading words array from file failed!\n");
    prevSize = 0;
    free(data->dictionaryFileName);
    free(data->spellcheckFileName);
    numThreadsInUse--;
    numThreadsFinished++;
    pthread_mutex_unlock(&lock);
    return NULL;
  }
  // if (debugOutput) { // this is very slow so prepare to wait
  //   printToLog(debugFile, "word count is: %d\n", wordCountFile);
  //   if (detailedDebug) {
  //     for(int i = 0; i < wordCountFile; i++) {
  //       printToLog(debugFile, "%s\n", fileArrayOfStrings[i]);
  //     }
  //   }
  // }
  if (wordCountDictionary < 1 || wordCountFile < 1) {
    if (dictionaryArrayOfStrings) {
      free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
    }
    if (fileArrayOfStrings) {
      free2DArray((void ***)&fileArrayOfStrings, wordCountFile);
    }
    exit_failure:
    pthread_mutex_lock(&lock);
    free(data->dictionaryFileName);
    free(data->spellcheckFileName);
    free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
    data->threadStatus = -1;
    numThreadsFinished ++;
    numThreadsInUse--;
    data->errorArray = NULL; // no error arrays
    pthread_mutex_unlock(&lock);
    return NULL;
  }

  unsigned int countTotalMistakes, countInArr;
  spellingError *mistakes = compareFileData((const char **)dictionaryArrayOfStrings, (const char **)fileArrayOfStrings,
  wordCountDictionary, wordCountFile, &countTotalMistakes, &countInArr);
  free2DArray((void ***)&fileArrayOfStrings, wordCountFile);
  if (!mistakes) {
    goto exit_failure;
  }
  if (debugOutput) {
    printToLog(debugFile, "\nExited comparison\n");
  }
  pthread_mutex_lock(&lock);
  for(int i = 0; i < countInArr; i++) {
    printf("word: %s count: %d\n", mistakes[i].misspelledString, mistakes[i].countErrors);
  }
  freeArrayOfSpellingErrors(&mistakes, countInArr);
  free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
  printf("\nThere are %d mistakes in total\n", countTotalMistakes);

  // printf("prevSize = %d\n", prevSize);
  // printf("new array size will be: %d\n", countInArr + prevSize);
  // printf("address of data error array: %p\n", (void *)data->errorArray);
  // fflush(stdout);
  
  data->errorArray = realloc(data->errorArray, sizeof(spellingError) * (countInArr + prevSize));
  // printf("number of mistakes: %d\n", countTotalMistakes);
  // printf("number of elements in array: %d\n", countInArr);
  // printf("dictionary fileName: %s\n", dictionaryFileName);
  // printf("checked fileName: %s\n", spellcheckFileName);
  // printf("address of mistakes array: %p\n", (void *)mistakes);
  // fflush(stdout);
  for(int i = 0; i < countInArr; i++) {
    if (mistakes) {
      mistakes[i].dictionaryName = strdup(dictionaryFileName);
      mistakes[i].fileName = strdup(spellcheckFileName); 
    }
  }
  // printf("past first loop\n");
  if (mistakes) {
    for (int i = 0; i < countTotalMistakes; i++) {
      data->errorArray[i + prevSize] = mistakes[i];
    }
  }
  // exit success
  prevSize = countInArr;
  free(data->dictionaryFileName);
  free(data->spellcheckFileName);
  numThreadsInUse--;
  numThreadsFinished++;
  pthread_mutex_unlock(&lock);
  // thread status already set to 0
  return NULL;
}

void free2DArray(void ***addressOfGenericPointer, int numberOfInnerElements) {
  void **genericPointer = (void **)*addressOfGenericPointer;
  if (genericPointer == NULL || *genericPointer == NULL) {
    return;
  }
  for (int i = 0; i < numberOfInnerElements; i++) {
    if (genericPointer[i]) {
      free(genericPointer[i]);
      genericPointer[i] = NULL;
    }
  }
  free(genericPointer);
  *addressOfGenericPointer = NULL;
  return;
}

void freePointer(void **addressOfGenericPointer) {
  if (!addressOfGenericPointer) {
    return;
  }
  void * genericPointer = *addressOfGenericPointer;
  if (!genericPointer) {
    return;
  }
  free(genericPointer);
  *addressOfGenericPointer = NULL; // set to NULL since freed
  return;
}

int partitionSpellingErrorArr(spellingError *arr, int start, int end) {
    unsigned int pivot = arr[end].countErrors;
    int i = start - 1;

    for (int j = start; j < end; j++) {
      if (arr[j].countErrors <= pivot) {
        i++;
        // Swap elements individually
        spellingError temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
      }
    }
    // Swap pivot element with the element at i + 1
    spellingError temp = arr[i + 1];
    arr[i + 1] = arr[end];
    arr[end] = temp;
    return i + 1;
}

void quickSortSpellingErrorArr(spellingError *arr, int start, int end) {
  if (end <= start) {
      return;
  }
  int pivotIndex = partitionSpellingErrorArr(arr, start, end);
  quickSortSpellingErrorArr(arr, start, pivotIndex - 1);
  quickSortSpellingErrorArr(arr, pivotIndex + 1, end);
  return;
}

spellingError *compareFileData(const char **dictionaryData, const char ** fileData, const unsigned int numEntriesDictionary,
const unsigned int numEntriesInFile, unsigned int *countTotalMistakes, unsigned int *countInArr) {
  *countTotalMistakes = 0;
  *countInArr = 0;
  spellingError *mistakesArr = NULL; // no entries yet
  unsigned int countMistakesForCurrentWord = 0;
  char **dictionaryCopySorted = NULL;
  if (doSorting) {
    dictionaryCopySorted = (char **)malloc(sizeof(char *) * numEntriesDictionary);
    if (dictionaryCopySorted == NULL) {
      printToLog(debugFile, "error with mallocing copy...\n");
      return NULL;
    }
    for(int i = 0; i < numEntriesDictionary; i++) {
      // printf("%s\n", dictionaryData[i]);
      if(!dictionaryData) {
        dictionaryCopySorted[i] = strdup(dictionaryData[i]); //strdup will malloc...be sure to free later
      }
      if (dictionaryCopySorted && dictionaryCopySorted[i] == NULL) {
        printToLog(debugFile, "error with mallocing copy inner string...\n");
        while (i > 0) {
          freePointer((void **)&dictionaryCopySorted[--i]);
        }
        freePointer((void **)&dictionaryCopySorted);
        return NULL;
      }
    }
    quickSortStr(dictionaryCopySorted, 0, numEntriesDictionary - 1); // use quicksort to make sure the dictionary is in alphabetical order
  }
  if (debugOutput && doSorting && !verifySortedStr((const char **)dictionaryCopySorted, numEntriesDictionary)) {
    printf("sort failded\n");
    free2DArray((void ***)&dictionaryCopySorted, numEntriesDictionary);
    return NULL;
  }
  bool flag = false;
  char **seenWords = malloc(sizeof(char *) * numEntriesInFile);
  if (!seenWords) {
    printToLog(debugFile, "failed malloc\n");
    return NULL;
  }
  for (int i = 0; i < numEntriesInFile; i++) {
    seenWords[i] = NULL;
  }
  for(int i = 0; i < numEntriesInFile; i++) { // for each word in the file...
    flag = false;
    for(int j = 0; j < i; j++) {
      if (seenWords[j] != NULL && strcmp(fileData[i], seenWords[j]) == 0) {
        flag = true; // Word already seen
        break;
      }
    }
    if (flag) {
      continue;
    }
    seenWords[i] = strdup(fileData[i]);
    // printf("currently i is: %d\n", i);
    if (!doSorting) {
      dictionaryCopySorted = (char **)dictionaryData;
    }
    if ((countMistakesForCurrentWord = numStringMismatchesInArrayOfStrings((const char **)dictionaryCopySorted, (const char **)fileData,
    numEntriesDictionary, numEntriesInFile, fileData[i]))) {
      // this is when a match is NOT found...
      // printf("Found %d misspellings of the word %s\n", countMistakesForCurrentWord, fileData[i]);
      (*countTotalMistakes) += countMistakesForCurrentWord;
      (*countInArr)++;
      mistakesArr = (spellingError *)realloc(mistakesArr, ((*countInArr) * sizeof(spellingError)));
      // printf("size of mistakes array is now %d\n", (int)*countInArr);
      mistakesArr[(*countInArr) - 1].countErrors = countMistakesForCurrentWord;
      mistakesArr[(*countInArr) - 1].misspelledString = (char *)strdup(fileData[i]);
      mistakesArr[(*countInArr) - 1].dictionaryName = NULL;
      mistakesArr[(*countInArr) - 1].fileName = NULL;
      if (mistakesArr[(*countInArr) - 1].misspelledString == NULL) {
        printToLog(debugFile, "error with malloc for strdup\n");
        return NULL;
      }
    }
  }
  free2DArray((void ***)&seenWords, numEntriesInFile);
  if (doSorting) {
    free2DArray((void ***)&dictionaryCopySorted, numEntriesDictionary);
  }
  // quickSortSpellingErrorArr(mistakesArr, 0, (*countInArr) - 1); // arrange from lowest frequency to highest
  if (!mistakesArr) { // if there are no mistakes create the array and populate it
    mistakesArr = (spellingError *)malloc(sizeof(spellingError));
    mistakesArr[0].countErrors = 0;
    mistakesArr[0].misspelledString = NULL;
    mistakesArr[0].dictionaryName = NULL;
    mistakesArr[0].fileName = NULL;
    (*countInArr) = 1;
  }
  // printf("\nreturning an address %p\n\n");
  return mistakesArr;
}

unsigned int numStringMismatchesInArrayOfStrings(const char **arrayOfDictionaryStrings, const char **arrayOfFileStrings,
                                              unsigned int sizeDictionary, unsigned int sizeFile,  const char *target) {
  // retuns number of matches found
  // assumes that arrayOfDictionaryStrings is valid (no checks!)
  for (int i = 0; i < sizeDictionary; i++) {
    if (!strcmp(arrayOfDictionaryStrings[i], target)) {
      return 0; // if a match does exist in the dictionary, the word is valid/correct
    }
  }
  unsigned int count = 0;
  for (int i = 0; i < sizeFile; i++) {
    if (!strcmp(arrayOfFileStrings[i], target)) {
      count++;
    }
  }
  return count;  // Target string not found
}

int partitionStr(char **arr, int start, int end) {
    char *temp;
    char *pivot = arr[end];
    int i = start - 1;

    for (int j = start; j < end; j++) {
        if (strcmp(arr[j], pivot) <= 0) {
            i++;
            temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }
    temp = arr[i + 1];
    arr[i + 1] = arr[end];
    arr[end] = temp;
    return i + 1;
}

void quickSortStr(char **arr, int start, int end) {
    if (end <= start) {
        return;
    }
    int pivotIndex = partitionStr(arr, start, end);
    quickSortStr(arr, start, pivotIndex-1);
    quickSortStr(arr, pivotIndex + 1, end);
}

void freeArrayOfSpellingErrors(spellingError **arrayOfMistakes, unsigned int countInArr) {
  if (arrayOfMistakes == NULL || !(*arrayOfMistakes)) {
    return;
  }
  spellingError *array = *arrayOfMistakes; // safe as long as user does not pass in NULL
  for(int i = 0; i < countInArr; i++) {
      // Free the misspelledString for each spellingError struct
      if (array[i].misspelledString) {
        free(array[i].misspelledString);
        array[i].misspelledString = NULL;
      }
      if (array[i].dictionaryName) {
        free(array[i].dictionaryName);
        array[i].dictionaryName = NULL;
      }
      if (array[i].fileName) {
        free(array[i].fileName);
        array[i].fileName = NULL;
      }
  }
  free(array);
  *arrayOfMistakes = NULL; // Set the pointer to NULL to avoid dangling pointers
  return;
}

bool verifySortedStr(const char ** sortedArrayOfStrings, const unsigned int numStrings) {
  const char *min = sortedArrayOfStrings[0];

  for(unsigned int i = 0; i < numStrings; i++) {
    if (strcmp(sortedArrayOfStrings[i], min) < 0) {
      printf("string %s is less than %s\n", sortedArrayOfStrings[i], min);
      return false;
    }
  }
  return true;
}

bool verifySortedSpellingErrors(const spellingError *arrayOfSpellingErrors, const int numElements) {
  const unsigned int min = arrayOfSpellingErrors[0].countErrors;

for(unsigned int i = 0; i < numElements; i++) {
  if (arrayOfSpellingErrors[i].countErrors < min) {
    printf("int %d is less than %d\n", arrayOfSpellingErrors[i].countErrors, min);
    return false;
  }
}
return true; 
}

int printToLog(const char *debugFile, const char *stringLiteral, ...) {
  va_list args;
  va_start(args, stringLiteral);
  va_end(args);

  // Open the debug file
  int fd;
  if (firstWrite) {
    if ((fd = open(debugFile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
      perror("Error opening debug file");
      return -1;
    }
    firstWrite = false;
  }
  else {
    if ((fd = open(debugFile, O_CREAT | O_WRONLY | O_APPEND, 0644)) == -1) {
      perror("Error opening debug file");
      return -1;
    }
  }
  // Write the formatted string to the debug file
  if (vdprintf(fd, stringLiteral, args) == -1) {
    perror("Error writing to debug file");
    close(fd);
    return -1;
  }
  // Close the file descriptor
  if (close(fd) == -1) {
    perror("Error closing debug file");
    return -1;
  }
  return 0; // Success
}
