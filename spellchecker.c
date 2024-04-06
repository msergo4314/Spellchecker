#include "header.h"

int main(int argc, char **argv) {
  bool l_flag = false;
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("\n mutex init has failed\n");
    return 1;
  }
  char fileNameString[MAX_FILE_NAME_LENGTH + 1] = "";
  if (argc > 1) {
    for(unsigned char i = 0; i < argc; i++) {
      convertEntireStringToLower(argv[i]);
      if (!(strcmp(argv[i], "l") || !strcmp(argv[i], "-l"))) {
        l_flag = true;
      }
      else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
        printf("Usage: spellchecker.exe {l or -l} {--help or -h} {--default or -d}\n");
        return ALTERNATE_SUCCESS;
      }
      else if (!strcmp(argv[i], "--default") || !strcmp(argv[i], "-d")) {
        strcpy(fileNameString, "dictionary.txt"); // default dictionary (must be in pwd!)
      }
    }
  }
  //   code to detect how many threads you have. REQUIRES -fopenmp compilation
  //   flag
  // running this code will add "leaks" but they are not true leaks
  //   int numThreads;
  //     #pragma omp parallel
  //     {
  //         #pragma omp single
  //         {
  //             numThreads = omp_get_num_threads();
  //         }
  //     }
  //   printf("Number of threads for this computer: %d\n", numThreads);
  char userInput[MAX_SIZE_USERINPUT + 1] = "";
  pthread_t *threadIDs = NULL;
  unsigned int numThreadsStarted = 0;
  unsigned int numThreadsInUse = 0;
  clock_t start_time, end_time;
  double cpu_time_used;
  // struct for thread arguments
  threadArguments args = {0}; // set all to 0, threads will update values
  args.errorArray = NULL;
  start_time = clock();
  main_menu:
    printf("Main Menu:\n");
  printf("1. Start a new spellchecking task\n2. Exit\n\n");
  printf("Select the mode of operation (1 or 2): ");
  fgets(userInput, sizeof(userInput), stdin);
  unsigned char x;
  while ((x = validateUserInput(userInput)) != INTEGER) {
    input_loop:
    // printf("Invalid. You entered a string of type %s. Try again: ",
    // returnTypeStrings[x]);
    printf("Invalid. Enter integer between 1 and 2: ");
    fgets(userInput, sizeof(userInput), stdin);
  }
  unsigned char selection = (unsigned char) strtoul(userInput, NULL, 10);
  switch (selection) {
  case 1:
    // if numThreads not used, then use #define constant for max threads
    if (numThreadsInUse >= MAX_THREADS) {
      // if (numThreadsInUse >= numThreads) {
      printf("too many threads in use\n");
      goto case_2;
    }
    char qString[] = "q";
    // get dictionary to process
    if (!strcmp(fileNameString, "")) { // if fielNameString is empty then ask for dictionary
      printf("Enter the filename of the dictionary (or 'Q' to quit): ");
      fgets(fileNameString, sizeof(fileNameString), stdin);
      removeNewline(fileNameString);
      convertEntireStringToLower(fileNameString);
      if (!strcmp(fileNameString, qString)) {
        printf("Exiting...\n");
        goto main_menu;
      }
    }
    args.dictionaryFileName = strdup(fileNameString);
    if (args.dictionaryFileName == NULL) {
      perror("Could not malloc with strdup in main. Exiting...\n");
      pthread_mutex_destroy(&lock);
      freePointer((void **)&threadIDs);
      freeArrayOfSpellingErrors(&(args.errorArray), args.prevSize);
      return FAILURE;
    }
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
    printf("%s\n", fileNameString);
    freePointer((void **)&(args.spellcheckFileName));
    args.spellcheckFileName = strdup(fileNameString);
    if (!args.dictionaryFileName) {
      perror("Could not malloc with strdup in main (input file strdup failure). Exiting...\n");
      pthread_mutex_destroy(&lock);
      freePointer((void **)&threadIDs);
      freeArrayOfSpellingErrors(&(args.errorArray), args.prevSize);
      return (FAILURE);
    }
    pthread_mutex_unlock(&lock);
    threadIDs = (pthread_t *) realloc( threadIDs, (sizeof(pthread_t) *(numThreadsStarted + 1)));
    if (!threadIDs) {
      printf("failed to realloc for threadIDs\n");
      free(args.dictionaryFileName);
      free(args.spellcheckFileName);
      pthread_mutex_destroy(&lock);
      return FAILURE;
    }
    threadIDs[numThreadsStarted] = 0;
    // printf("making thread %d\n", numThreadsStarted + 1);
    int error = pthread_create(&(threadIDs[numThreadsStarted]), NULL, threadFunction, (void *)&args);
    if (error != 0) {
      printf("Thread can't be created: %s\n", strerror(error));
      free(args.dictionaryFileName);
      free(args.spellcheckFileName);
      free(threadIDs);
      if (args.errorArray) {
        free(args.errorArray);
      }
      pthread_mutex_unlock(&lock);
      pthread_mutex_destroy(&lock);
      return FAILURE;
    }
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&updatedVariables, &lock); // make sure the variables have been updated
    numThreadsInUse = args.numThreadsInUse;
    numThreadsStarted = args.numThreadsStarted;
    pthread_mutex_unlock(&lock);
    goto main_menu;
  case 2:
    case_2:
      // int status;
    printf("\nexiting program...\n");
    pthread_mutex_lock(&lock);
    char *outputString;
    unsigned int numThreadsFinished = args.numThreadsFinished;
    numThreadsStarted = args.numThreadsStarted;
    numThreadsInUse = args.numThreadsInUse;
    printf("Number of threads started: %d\n", numThreadsStarted);
    printf("%d threads finished execution\n", numThreadsFinished);
    printf("%d threads running currently\n", numThreadsInUse);
    pthread_mutex_unlock(&lock);
    if (threadIDs) {
      for (int i = 0; i < numThreadsStarted; i++) {
        printf("waiting for thread #%d (ID = %lu)\n", i + 1, threadIDs[i]);
        // printf("there are %d threads running currently\n",
        // args.numThreadsInUse); pthread_mutex_unlock(&lock);
        pthread_join(threadIDs[i], NULL);
      }
      printf("All threads finished\n");
      outputString = getOutputString(args); // pass in thread struct
      freeArrayOfSpellingErrors(&(args.errorArray), args.prevSize);
      free(threadIDs);
      if (l_flag) {
        printToOutputFile(threadOutputFile, outputString);
      }
      else {
        printf("\n%s\n", outputString);
      }
      free(outputString);
    }
    pthread_mutex_destroy(&lock);
    end_time = clock(); // Record the end time
    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("\n\nExecution time: %lf\n\n", cpu_time_used);
    return SUCCESS; // do not need break since end of main
  default:
    printf("not valid. Try again\n");
    goto input_loop;
  }
}

void *threadFunction(void *vargp) {
  threadArguments *data = (threadArguments *) vargp;
  pthread_mutex_lock(&lock); // immediately lock mutex to update variables
  data -> numThreadsInUse++;
  unsigned int threadID = data -> numThreadsStarted;
  data -> numThreadsStarted++;
  char *dictionaryFileName = data -> dictionaryFileName;
  char *spellcheckFileName = data -> spellcheckFileName;
  // printFlush("Thread ID: %d\n", pthread_self());
  char **fileArrayOfStrings = NULL;
  char **dictionaryArrayOfStrings = NULL;
  pthread_cond_signal(&updatedVariables); // will notify the main thread that struct has been
  // updated to prevent invalid reads
  pthread_mutex_unlock(&lock);

  printToLog(debugFile, "attempting to read file %s\n", dictionaryFileName);
  if (detailedDebug) {
    printToLog(debugFile, "split file is:\n\n");
  }
  unsigned int wordCountDictionary = 0;
  dictionaryArrayOfStrings = readFileArray(dictionaryFileName, &wordCountDictionary);
  if (!dictionaryArrayOfStrings) { 
    // fprintf(stderr, "Reading words array from dictionary file failed!\n");
    goto exit_failure;
  }
  printToLog(debugFile, "wordcount is: %d\n", wordCountDictionary);
  if (detailedDebug) {
    unsigned int x;
    char *y = readEntireFileIntoStr(dictionaryFileName, &x);
    printToLog(debugFile, "%s\n", y);
    free(y);
  }

  /*****************************SECOND FILE******************************/

  printToLog(debugFile, "\nattempting to read file %s\n", spellcheckFileName);
  if (detailedDebug) {
    printToLog(debugFile, "split file is:\n\n");
  }
  unsigned int wordCountFile = 0;
  fileArrayOfStrings = readFileArray(spellcheckFileName, &wordCountFile);
  if (fileArrayOfStrings == NULL) {
    // fprintf(stderr, "Reading words array from file failed!\n"); 
    goto exit_failure;
  }
  printToLog(debugFile, "word count is: %d\n", wordCountFile);
  if (detailedDebug) {
    unsigned int x;
    char *y = readEntireFileIntoStr(spellcheckFileName, &x);
    printToLog(debugFile, "%s\n", y);
    free(y);
  }
  if (wordCountDictionary < 1 || wordCountFile < 1) {
    exit_failure:
    pthread_mutex_lock(&lock);
    freePointer((void **)&(data -> dictionaryFileName));
    freePointer((void **)&(data -> spellcheckFileName));
    free2DArray((void ***)&fileArrayOfStrings, wordCountFile);
    free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
    data -> numThreadsFinished++;
    data -> numThreadsInUse--;
    pthread_mutex_unlock(&lock);
    return NULL;
  }

  unsigned int countTotalMistakes, countInArr;
  spellingError *mistakes = compareFileData((const char **) dictionaryArrayOfStrings,
    (const char **) fileArrayOfStrings, wordCountDictionary,
    wordCountFile, &countTotalMistakes, &countInArr);
  free2DArray((void ***)&fileArrayOfStrings, wordCountFile);
  free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
  // should ALWAYS return valid pointer
  
  pthread_mutex_lock(&lock);
  unsigned int previousSize = data -> prevSize;
  unsigned int newSize = countInArr + previousSize;
  data -> errorArray = realloc(data -> errorArray, sizeof(spellingError) * newSize);
  data -> prevSize = newSize;
  pthread_mutex_unlock(&lock);
  for (int i = 0; i < countInArr; i++) {
    mistakes[i].threadID = threadID;
    mistakes[i].dictionaryName = strdup(dictionaryFileName);
    mistakes[i].fileName = strdup(spellcheckFileName);
    if (!mistakes[i].fileName || !mistakes[i].dictionaryName) {
      perror("strdup malloc failed!\n");
      freeArrayOfSpellingErrors(&mistakes, data -> prevSize); // prevsize now updated
      goto exit_failure;
    }
  }
  pthread_mutex_lock(&lock);
  unsigned int numIterations = 0;
  for (unsigned int i = previousSize; i < countInArr + previousSize; i++) { // can also use data->prevSize for loop condition
    memcpy(&(data -> errorArray[i]), &(mistakes[numIterations++]), sizeof(spellingError));
  }
  pthread_mutex_unlock(&lock);
  quickSortSpellingErrorArr(mistakes, 0, countInArr - 1); // sort high to low
  // do file IO with sorted results
  if (writeThreadToFile(threadOutputFile, mistakes, countInArr) == FAILURE) {
    fprintf(stderr, "error with logging output of thread analysis to file %s!\n", threadOutputFile);
    free(mistakes);
    goto exit_failure;
  }
  // exit success
  pthread_mutex_lock(&lock);
  freePointer((void **)&(data -> dictionaryFileName));
  freePointer((void **)&(data -> spellcheckFileName));
  free(mistakes);
  data -> numThreadsInUse--;
  data -> threadSucccessCount++;
  data -> numThreadsFinished++;
  pthread_mutex_unlock(&lock);
  return NULL;
}

int writeThreadToFile(const char *fileName, spellingError *listOfMistakes, unsigned int numElements) {

  if (!fileName) {
    perror("invalid file name given\n");
    return FAILURE;
  }
  // asumes the array of spellingErrors is already sorted in descending order
  int totalErrors = 0;
  char *str = NULL, *currentFileName = NULL;
  // listOfMistakes[0] must be valid but should be since numElements >= 1
  currentFileName = listOfMistakes[0].fileName; // always at least 1 in the list so safe to dereference
  // if (currentFileName == NULL) {
  //   perror("Not enough file names when writing thread info to file!\n");
  //   return FAILURE;
  // }
  for (int i = 0; i < numElements; i++) {
    totalErrors += listOfMistakes[i].countErrors;
  }
  printToLog(debugFile, "total errors for file %s: %d\n", currentFileName, totalErrors);
  size_t strSize = strlen(currentFileName) + getNumDigitsInNumber(totalErrors, 10) + 3; // extra 3 for two spaces and \0
  str = malloc(strSize);
  if (!str) {
    perror("malloc failed inside writeThreadToFile\n");
    return FAILURE;
  }
  str[0] = '\0';
  snprintf(str, strSize, "%s %d ", currentFileName, totalErrors); // first comes the filename itself
  char *tempString = NULL;
  for (int i = 0; i < numElements; i++) {
    printToLog(debugFile, "string: %s\n", str);
    if ((tempString = listOfMistakes[i].misspelledString)) { // it is possible that there are no
      str = realloc(str, strlen(str) + strlen(tempString) + 2); // 2 chars extra -- \0 and space/newline
      if (str == NULL) {
        perror("realloc failed for string variable inside writeThreadToFile function");
        return FAILURE;
      }
      strcat(str, tempString); // add the misspelled word
      if (i + 1 < numElements) {
        strcat(str, " ");
      } else {
        strcat(str, "\n"); // ending newline
      }
    }
    else {
      if (printToLog(debugFile, "mistake misspelled string at index %d for file %s is NULL\n", i, currentFileName) == ALTERNATE_SUCCESS) {
        fprintf(stderr, "mistake misspelled string at index %d for file %s is NULL\n", i, currentFileName);
      }
    }
  }
  // Write the formatted string
  printToLog(debugFile, "attempting to write to output file string %s\n", str);
  pthread_mutex_lock(&lock);
  printToOutputFile(fileName, str);
  pthread_mutex_unlock(&lock);
  free(str);
  return SUCCESS; // Success
}

void removeNewline(char *string) {
  const unsigned int length = strlen(string);
  if (string[length - 1] == '\n') {
    string[length - 1] = '\0';
  }
}

char *readEntireFileIntoStr(const char *fileName, unsigned int *sizeBytes) {
  // use mutex in case another thread is also reading the file
  int fd;
  if ((fd = open(fileName, O_RDONLY)) == -1) {
    printf("bad file descriptor. File most likely does not exist\n");
    return NULL;
  }
  int size = lseek(fd, 0, SEEK_END); // sizeof file in bytes
  char *readString = malloc(size + 1); // one byte extra for \0
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
    } else if (inputString[i] == '.') {
      hasDecimal = true;
      continue;
    } else {
      chars++;
    }
  }
  if (hasDecimal && (digit >= 2)&& !chars) {
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

char **readFileArray(const char *fileName, unsigned int *wordCount) {
  unsigned int sizeBytes;
  char **stringArrToReturn = NULL;
  char *singleFileString;
  if (!(singleFileString = readEntireFileIntoStr(fileName, &sizeBytes))) {
    return NULL;
  }
  convertEntireStringToLower(singleFileString);
  stringArrToReturn = splitStringOnWhiteSpace(singleFileString, wordCount);
  if (stringArrToReturn == NULL) {
    free(singleFileString);
    return NULL;
  }
  free(singleFileString);
  return stringArrToReturn;
}

void convertEntireStringToLower(char *string) {
  for (long long unsigned int index = 0; string[index]; index++) {
    if (isalpha(string[index])) {
      string[index] = tolower(string[index]);
    }
  }
}

char *getNonAlphabeticalCharsString() {
  static char nonAlphaString[CHAR_MAX - CHAR_MIN + 1] = ""; // Assuming CHAR_MIN is negative
  char temp[2] = {0};
  for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
    if (!isalpha(i) && i != '\'') { // Exclude alphabetic characters and '
      temp[0] = (char)i;
      strcat(nonAlphaString, temp); // Concatenate to result string
    }
  }
  return nonAlphaString;
}

char **splitStringOnWhiteSpace(const char *inputString, unsigned int *wordCount) {
  if (!inputString || !wordCount) {
    fprintf(stderr, "passed in NULL\n");
    return NULL;
  }
  unsigned int wordCountLocal = 0;
  bool prevWasWhitespace = true;
  // Count the number of words
  const char *ptr;
  char character;
  for (ptr = inputString; *ptr != '\0'; ptr++) {
    character = *ptr;
    if (!(isalpha(character) || character == '\'')) { // apostrophe is special case
      if (prevWasWhitespace) {
        continue; // ignore whitespaces until non-whitespace char found
      } else {
        prevWasWhitespace = true;
      }
      wordCountLocal++;
    } else {
      prevWasWhitespace = false;
    }
  }
  if (!prevWasWhitespace) {
    wordCountLocal++; // Account for the last word at EOF
  }
  if (wordCountLocal == 0) {
    printf("no words in file. Returning early...\n");
    *wordCount = 0;
    return NULL;
  }
  // Allocate memory for array of strings
  char **words = (char **) malloc((wordCountLocal) *sizeof(char *));
  if (words == NULL) {
    if (printToLog(debugFile, "Memory allocation failed for words arr\n") == ALTERNATE_SUCCESS) {
      fprintf(stderr, "Memory allocation failed for words arr\n");
    }
    *wordCount = wordCountLocal;
    return NULL;
  }
  for (int i = 0; i < wordCountLocal; i++) {
    words[i] = NULL; // useful if words is freed before the inner ptrs are set
  }
  const char *delimiters = getNonAlphabeticalCharsString();
  if (!delimiters) {
    if (printToLog(debugFile, "Delimiters were not created properly inside splitStringOnWhitespace\n") == ALTERNATE_SUCCESS) {
      fprintf(stderr, "Delimiters were not created properly inside splitStringOnWhitespace\n");
    }
    free2DArray((void ***)&words, wordCountLocal);
    *wordCount = wordCountLocal;
    return NULL;
  }
  // printf("delims: %s\n", delimiters);
  char *token = strtok((char *) inputString, delimiters); // cast to char *
  int index = 0;
  while (token != NULL && index < wordCountLocal) {
    words[index] = strdup(token); // Allocate memory for each word
    if (words[index] == NULL) {
      if (printToLog(debugFile, "Memory allocation failed for strdup inside splitOnWhitespace") == ALTERNATE_SUCCESS) {
        fprintf(stderr, "Memory allocation failed for strdup inside splitOnWhitespace");
      }
      free2DArray((void ***)&words, index);
      *wordCount = wordCountLocal;
      return NULL;
    }
    index++;
    token = strtok(NULL, delimiters);
  }
  *wordCount = wordCountLocal;
  return words;
}

int partitionSpellingErrorArr(spellingError *arr, int start, int end) {
  int pivot = arr[end].countErrors;
  int i = start - 1;

  for (int j = start; j < end; j++) {
    if (arr[j].countErrors >= pivot) {
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

spellingError *compareFileData(const char **dictionaryData, const char **fileData, const unsigned int numEntriesDictionary,
const unsigned int numEntriesFile, unsigned int *countTotalMistakes, unsigned int *countInArr) {
  *countTotalMistakes = 0;
  *countInArr = 0;
  spellingError *mistakesArr = NULL; // no entries yet
  unsigned int countMistakesForCurrentWord = 0;
  
  quicksortStrings((char **) dictionaryData, 0, numEntriesDictionary - 1);
  if (!verifySortedStr(dictionaryData, numEntriesDictionary)) {
    printFlush("DICTIONARY NOT SORTED!");
    return NULL;
  }
  bool flag = false;
  char **seenWords = malloc(sizeof(char *) *numEntriesFile);
  if (!seenWords) {
    if (printToLog(debugFile, "failed malloc inside compareFileData\n") == ALTERNATE_SUCCESS) {
      fprintf(stderr, "failed malloc inside compareFileData\n");
    }
    return NULL;
  }
  for (int i = 0; i < numEntriesFile; i++) {
    seenWords[i] = NULL;
  }
  for (int i = 0; i < numEntriesFile; i++) { // for each word in the file...
    flag = false;
    for (int j = 0; j < i; j++) {
      if (seenWords[j] != NULL && fileData[i] &&
        strcmp(fileData[i], seenWords[j]) == 0) {
        flag = true; // Word already seen
        break;
      }
    }
    if (flag) {
      continue;
    }
    seenWords[i] = strdup(fileData[i]);
    // if ((countMistakesForCurrentWord = numStringMismatchesInArrayOfStrings(
    //     (const char **) dictionaryData, (const char **) fileData,
    //     numEntriesDictionary, numEntriesFile, fileData[i]))) {

      if ((countMistakesForCurrentWord = binarySearchArrayOfStrings((const char **)dictionaryData, numEntriesDictionary, (const char**)fileData, numEntriesFile, (const char *)fileData[i])) > 0) {
      // if ((countMistakesForCurrentWord = numStringMismatchesInArrayOfStrings(dictionaryData, fileData, numEntriesDictionary, numEntriesFile, (const char *)fileData[i])) > 0) {
      // this is when a match is NOT found...
      // printf("Found %d misspellings of the word %s\n",
      // countMistakesForCurrentWord, fileData[i]);
      (*countTotalMistakes) += countMistakesForCurrentWord;
      (*countInArr)++;
      unsigned int temp = *countInArr - 1;
      mistakesArr = (spellingError *) realloc(mistakesArr, ((temp + 1) *sizeof(spellingError)));
      // printf("size of mistakes array is now %d\n", (int)*countInArr);
      mistakesArr[temp].countErrors = countMistakesForCurrentWord;
      mistakesArr[temp].misspelledString = (char *) strdup(fileData[i]);
      mistakesArr[temp].dictionaryName = NULL;
      mistakesArr[temp].fileName = NULL;
      if (mistakesArr[temp].misspelledString == NULL) {
        if (debugOutput) {
          printToLog(debugFile, "error with malloc for strdup\n");
        } else {
          printf("error with malloc for strdup\n");
        }
        return NULL;
      }
    }
  }
  free2DArray((void ***)&seenWords, numEntriesFile);
  if (!mistakesArr) { // if there are no mistakes create the array and
    // populate it with one entry
    mistakesArr = (spellingError *) malloc(sizeof(spellingError));
    if (!mistakesArr) {
      perror("could not malloc mistake array inside compareFileData\n");
      return NULL;
    }
    mistakesArr[0].countErrors = 0;
    mistakesArr[0].misspelledString = NULL;
    mistakesArr[0].dictionaryName = NULL;
    mistakesArr[0].fileName = NULL;
    (*countInArr) = 1;
  }
  return mistakesArr;
}

unsigned int numStringMismatchesInArrayOfStrings(const char **arrayOfDictionaryStrings,
  const char **arrayOfFileStrings,
  unsigned int sizeDictionary, unsigned int sizeFile,
  const char *target) {
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
  return count; // Target string not found
}

unsigned int binarySearchArrayOfStrings(const char **sortedDictionaryArrayOfStrings, unsigned int dictionarySize,
                                        const char **arrayOfFileStrings, unsigned int fileSize, const char *target) {
    // Binary search in the dictionary array
    if (sortedDictionaryArrayOfStrings == NULL || arrayOfFileStrings == NULL || *sortedDictionaryArrayOfStrings == NULL || *arrayOfFileStrings == NULL || target == NULL) {
      return 0;
    }
    unsigned int left = 0, right = dictionarySize - 1;
    while (left <= right) {
      unsigned int mid = left + (right - left) / 2;
      int comparison = strcmp(sortedDictionaryArrayOfStrings[mid], target);
      if (comparison == 0) {
        // Target substring exists in the dictionary, return 0
        return 0;
      } else if (comparison < 0) {
        left = mid + 1; // If target is greater, ignore left half
      } else {
        right = mid - 1; // If target is smaller, ignore right half
      }
    }

    // If the target substring is not found in the dictionary, count its occurrences in the file string array
    unsigned int count = 0;
    for (unsigned int i = 0; i < fileSize; ++i) {
        const char *fileString = arrayOfFileStrings[i];
        const char *substring = strstr(fileString, target);
        while (substring != NULL) {
            count++;
            substring = strstr(substring + 1, target);
        }
    }
    return count;
}

void quicksortStrings(char **arr, int left, int right) {
  if (left < right) {
    int i = left, j = right;
    char *pivot = arr[(left + right) / 2];

    // Partition
    while (i <= j) {
      while (strcmp(arr[i], pivot) < 0) i++;
      while (strcmp(arr[j], pivot) > 0) j--;
      if (i <= j) {
        char *temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
        i++;
        j--;
      }
    }
    // Recursion
    quicksortStrings(arr, left, j);
    quicksortStrings(arr, i, right);
  }
}

void freeArrayOfSpellingErrors(spellingError **arrayOfMistakes, unsigned int countInArr) {
  if (arrayOfMistakes == NULL || *arrayOfMistakes == NULL) {
    return;
  }
  spellingError *array = *arrayOfMistakes; // safe as long as user does not pass in NULL
  for (int i = 0; i < countInArr; i++) {
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

bool verifySortedStr(const char **sortedArrayOfStrings,
  const unsigned int numStrings) {
  if (!sortedArrayOfStrings) {
    return true;
  }
  const char *min = sortedArrayOfStrings[0];
  for (unsigned int i = 0; i < numStrings; i++) {
    if (strcmp(sortedArrayOfStrings[i], min) < 0) {
      printf("string %s is less than %s\n", sortedArrayOfStrings[i], min);
      return false;
    }
  }
  return true;
}

// bool verifySortedSpellingErrors(const spellingError *arrayOfSpellingErrors,
//   const int numElements) {
//   if (!arrayOfSpellingErrors || numElements < 1) {
//     return true;
//   }
//   const unsigned int min = arrayOfSpellingErrors[0].countErrors;
//   for (unsigned int i = 0; i < numElements; i++) {
//     if (arrayOfSpellingErrors[i].countErrors < min) {
//       // printf("int %d is less than %d\n",
//       //   arrayOfSpellingErrors[i].countErrors, min);
//       return false;
//     }
//   }
//   return true;
// }

int printToLog(const char *debugFile, const char *stringLiteral, ...) {
  if (!debugOutput) {
    // if no debug enabled, do not log
    return ALTERNATE_SUCCESS; // indicates success, but no logging
  }
  pthread_mutex_lock(&lock);
  va_list args;
  va_start(args, stringLiteral);
  va_end(args);

  // Open the debug file
  int fd;
  if (firstWriteDebug) {
    if ((fd = open(debugFile, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
      perror("Error opening debug file (first attempt)");
      pthread_mutex_unlock(&lock);
      return FAILURE;
    }
    firstWriteDebug = false;
  } else {
    if ((fd = open(debugFile, O_CREAT | O_WRONLY | O_APPEND, 0644)) == -1) {
      perror("Error opening debug file");
      pthread_mutex_unlock(&lock);
      return FAILURE;
    }
  }
  // Write the formatted string to the debug file
  if (vdprintf(fd, stringLiteral, args) == -1) {
    perror("Error writing to debug file");
    close(fd);
    pthread_mutex_unlock(&lock);
    return FAILURE;
  }
  // Close the file descriptor
  if (close(fd) == -1) {
    perror("Error closing debug file");
    pthread_mutex_unlock(&lock);
    return FAILURE;
  }
  pthread_mutex_unlock(&lock);
  return SUCCESS; // Success
}

void free2DArray(void ***addressOfGenericPointer, int numberOfInnerElements) {
  if (addressOfGenericPointer == NULL) {
    return;
  }
  if (* addressOfGenericPointer == NULL) {
    return;
  }
  void **genericPointer = (void **) *addressOfGenericPointer;
  for (int i = 0; i < numberOfInnerElements; i++) {
    if ((* addressOfGenericPointer)[i]) {
      free(genericPointer[i]);
      genericPointer[i] = NULL;
    }
  }
  free(genericPointer);
  *addressOfGenericPointer = NULL;
  return;
}

void freePointer(void **addressOfGenericPointer) {
  if (addressOfGenericPointer == NULL || *addressOfGenericPointer == NULL) {
    return;
  }
  void *genericPointer = *addressOfGenericPointer;
  free(genericPointer);
  *addressOfGenericPointer = NULL; // set to NULL since freed
  return;
}

char *getOutputString(threadArguments threadArgs) {
  char **errorsInEachFile = NULL;
  unsigned int size = threadArgs.prevSize;
  unsigned int numThreads = threadArgs.numThreadsFinished;
  unsigned int numSuccessfulThreads = threadArgs.threadSucccessCount;
  unsigned int totalMistakes = 0;

  // num array elements should be >= number of files processed
  errorsInEachFile = (char **)malloc(sizeof(char *) * numThreads);
  if (!errorsInEachFile) {
    perror("malloc failed for errorsInEachFile\n");
    return NULL;
  }
  for (unsigned int i = 0; i < size; i++) {
    if (threadArgs.errorArray[i].misspelledString) {
      totalMistakes += threadArgs.errorArray[i].countErrors;
    }
  }
  int *numUniqueMisspelledWords = (int *) malloc(sizeof(int) *numThreads);
  if (!numUniqueMisspelledWords) {
    perror("malloc failed for numUniqueMisspelledWords\n");
    free(errorsInEachFile);
    return NULL;
  }
  for (unsigned int i = 0; i < numThreads; i++) {
    errorsInEachFile[i] = NULL;
    numUniqueMisspelledWords[i] = countMistakesForThread(threadArgs.errorArray, size, i);
    if (debugOutput) {
      printToLog(debugFile, "number of unique errors for thread %d: %d\n", i + 1, numUniqueMisspelledWords[i]);
    }
  }
  const char *fileName = NULL, *mistake = NULL;
  unsigned int arrSize;
  unsigned int totalSize = 1; // start with one for \0
  for (unsigned int i = 0; i < numThreads; i++) {
    // rough estimate for max size
    arrSize = MAX_WORD_LENGTH *numUniqueMisspelledWords[i] +
      MAX_FILE_NAME_LENGTH + max(numUniqueMisspelledWords[i] - 1, 0) * 2 + 2; // includes the n-1 ", " strings, \n, and \0
    totalSize += arrSize - 1;
    errorsInEachFile[i] = malloc(arrSize);
    if (!errorsInEachFile[i]) {
      fprintf(stderr, "malloc failed for errorsInEachFile[%d]\n", i);
      free(numUniqueMisspelledWords);
      for (int j = 0; j < i; j++) {
        free(errorsInEachFile[j]);
      }
      free(errorsInEachFile);
      return NULL;
    }
    fileName = getFileNameFromThreadID(threadArgs.errorArray, i, size);
    if (!fileName) {
      sprintf(errorsInEachFile[i], "NO INPUT FILE FOR THREAD %d (or file was empty)\n", i + 1);
    } else {
      sprintf(errorsInEachFile[i], "File %d (%s): ", i + 1, fileName);
      for (int j = 0; j < numUniqueMisspelledWords[i]; j++) {
        mistake = getMistakeAtIndex(threadArgs.errorArray, i, j, size);
        if (mistake) {
          strcat(errorsInEachFile[i], mistake);
          if (j < numUniqueMisspelledWords[i] - 1) {
            strcat(errorsInEachFile[i], ", ");
          } else {
            strcat(errorsInEachFile[i], "\n");
          }
        }
      }
    }
  }
  char *outputString = malloc(totalSize);
  if (!outputString) {
    perror("malloc failed inside the outputString function\n");
    return NULL;
  }
  outputString[0] = '\0'; // add null terminator
  for (int i = 0; i < numThreads; i++) {
    strcat(outputString, errorsInEachFile[i]);
    free(errorsInEachFile[i]);
  }
  quickSortSpellingErrorArr(threadArgs.errorArray, 0, size - 1);
  outputString = generateSummary(threadArgs.errorArray, numThreads, size, outputString, numSuccessfulThreads);
  free(errorsInEachFile);
  free(numUniqueMisspelledWords);
  return outputString;
}

char *generateSummary(spellingError *errorArr, unsigned int numThreads,
  unsigned int arrayLength, char *inputString, unsigned int numSuccessfulThreads) {
  char formatString[250] = "";
  sprintf(formatString, "Number of files processed: %d\n", numSuccessfulThreads);
  unsigned int biggest = 0, secondBiggest = 0, thirdBiggest = 0,
    totalErrors = 0;
  char *biggestStr = NULL, *secondBiggestStr = NULL, *thirdBiggestStr = NULL;

  // printFlush("LENGTH OF input string: %d\n", (int)strlen(inputString));

  for (int i = 0; i < arrayLength; i++) {
    spellingError current = errorArr[i];
    if (current.countErrors && current.misspelledString) {
      if (current.countErrors > biggest) {
        biggest = current.countErrors;
        biggestStr = current.misspelledString;
      } else if (current.countErrors > secondBiggest) {
        secondBiggest = current.countErrors;
        secondBiggestStr = current.misspelledString;
      } else if (current.countErrors > thirdBiggest) {
        thirdBiggest = current.countErrors;
        thirdBiggestStr = current.misspelledString;
      }
    }
    totalErrors += errorArr[i].countErrors;
  }
  inputString = realloc(inputString, strlen(inputString) + strlen(formatString) + 1);
  strcat(inputString, formatString);
  sprintf(formatString, "Number of spelling errors: %d\n", totalErrors);
  inputString = realloc(inputString, strlen(inputString) + strlen(formatString) + 1);
  strcat(inputString, formatString);
  char numWordsToPrint =
    (biggest != 0) + (secondBiggest != 0) + (thirdBiggest != 0);
  switch (numWordsToPrint) {
  case 1:
    inputString = realloc(inputString, strlen(inputString)  + strlen("Most common misspelling: ") + 1);
    strcat(inputString, "Most common misspelling: ");
    break;
  case 2:
    inputString = realloc(inputString, strlen(inputString) + strlen("Two most common misspellings: ") + 1);
    strcat(inputString, "Two most common misspellings: ");
    break;
  case 3:
    inputString = realloc(inputString, strlen(inputString)  + strlen("Three most common misspellings: ") + 1);
    strcat(inputString, "Three most common misspellings: ");
  }
  if (!(biggest || secondBiggest || thirdBiggest)) {
    inputString = realloc(inputString, strlen(inputString)  + strlen("No mistakes!") + 1);
    strcat(inputString, "No mistakes!");
    return inputString;
  }
  for (char i = 0; i < numWordsToPrint; i++) {
    if (i == 0) {
      sprintf(formatString, "%s (%d times)", biggestStr, biggest);
    } else if (i == 1) {
      sprintf(formatString, "%s (%d times)", secondBiggestStr,
        secondBiggest);
    } else if (i == 2) {
      sprintf(formatString, "%s (%d times)", thirdBiggestStr,
        thirdBiggest);
    }
    inputString = realloc(inputString, strlen(inputString)  + strlen(formatString) + 1);
    if (!inputString) {
      perror("input string realloc failed inside generateSummary\n");
      return NULL;
    }
    strcat(inputString, formatString);
    if (i < numWordsToPrint - 1) {
      inputString = realloc(inputString, strlen(inputString)  + 3);
      if (!inputString) {
        perror("input string realloc failed inside generateSummary\n");
        return NULL;
      }
      strcat(inputString, ", ");
    }
  }
  return inputString;
}

const char *getMistakeAtIndex(spellingError *arr, int threadNum, int index, unsigned int numElements) {
  int count = 0;
  for (int i = 0; i < numElements; i++) {
    if (arr[i].threadID == threadNum) {
      if (count != index) {
        count++;
        continue;
      }
      return arr[i].misspelledString;
    }
  }
  return NULL;
}

unsigned int countMistakesForThread(spellingError *errorArr, unsigned int numEntriesInArr, int index) {
  // counts the number of misspelled unique words (NOT how many misspellings occured)
  unsigned int count = 0;
  for (int i = 0; i < numEntriesInArr; i++) {
    if (errorArr[i].threadID == index) {
      count++;
    }
  }
  return count;
}

const char *getFileNameFromThreadID(spellingError *arr, int index, unsigned int numElements) {
  if (arr) {
    for (int i = 0; i < numElements; i++) {
      if (arr[i].threadID == index) {
        return arr[i].fileName;
      }
    }
  }
  return NULL;
}

unsigned int max(unsigned int a, unsigned int b) {
  return a < b ? b : a;
}

void printToOutputFile(const char *fileName,
  const char *stringToPrint) {
  int fd;
  if (firstWriteThreadOutput) {
    // create if it does not exist.
    if ((fd = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
      fprintf(stderr, "Error opening spellcheck file (first attempt)%s\n", fileName);
      return;
    }
    firstWriteThreadOutput = false;
  } else {
    // open for appending
    if ((fd = open(fileName, O_CREAT | O_WRONLY | O_APPEND, 0644)) == -1) {
      fprintf(stderr, "Error opening spellcheck file %s\n", fileName);
      return;
    }
  }
  // Write the formatted string to the debug file
  if (write(fd, stringToPrint, strlen(stringToPrint)) == -1) {
    fprintf(stderr, "Error writing to output file %s\n", fileName);
    close(fd);
    return;
  }
  // Close the file descriptor
  if (close(fd) == -1) {
    fprintf(stderr, "could not close file descriptor when writing to file %s\n.", fileName);
    return;
  }
  return;
}

unsigned int getNumDigitsInNumber(int number, unsigned char base) {
  if (number == 0) {
    return 1;
  }
  int digits = 0;
  while (number) {
    number /= base;
    digits++;
  }
  return digits;
}
