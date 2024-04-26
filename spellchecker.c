/*
NOTE: USES GCC and O1 optimizations
*/

#include "header.h"

int main(int argc, char **argv) {
  bool l_flag = false, defaultMode = false;
  if (pthread_mutex_init(&lock, NULL) != 0) {
    printf("mutex init has failed\n");
    return FAILURE;
  }
  char fileNameString[MAX_FILE_NAME_LENGTH + 1] = "";
  if (argc > 1) {
    for(unsigned char i = 0; i < argc; i++) {
      convertEntireStringToLower(argv[i]);
      if (!strcmp(argv[i], "l") || !strcmp(argv[i], "-l")) {
        l_flag = true;
      }
      else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
        printf("Usage: %s {l or -l} {--help or -h} {--default or -d}\n", argv[0]);
        return SUCCESS;
      }
      else if (!strcmp(argv[i], "--default") || !strcmp(argv[i], "-d")) {
        // make sure to have dictionary.txt file in pwd of the exe (does not have to be actual pwd)
        defaultMode = true;
      }
    }
  }
  //   code to detect how many threads you have. REQUIRES -fopenmp compilation flag
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
  start_time = clock();
  char cwd[1024 + MAX_FILE_NAME_LENGTH + 1];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("could not find CWD. Aborting.\n");
    return FAILURE;
  }
  // printf("Current working directory: %s\n", cwd);
  // Use cwd to construct file paths relative to the current directory
  char *qString = "q";
  main_menu:
  printf("Main Menu:\n");
  printf("1. Start a new spellchecking task\n2. Exit\n\n");
  printf("Select the mode of operation (1 or 2): ");
  if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
    perror("fgets failed\n");
    return FAILURE;
  }
  unsigned char x;
  while ((x = validateUserInput(userInput)) != UINTEGER) {
    input_loop:
    // printf("Invalid. You entered a string of type %s. Try again: ",
    // returnTypeStrings[x]);
    printf("Invalid. Enter integer between 1 and 2: ");
    if (fgets(userInput, sizeof(userInput), stdin) == NULL) {
      perror("fgets failed\n");
      return FAILURE;
    }
  }
  unsigned char selection = (unsigned char) strtoul(userInput, NULL, 10);
  switch (selection) {
  case 1:
    ; // empty statement or else gcc will complain (pedantic flag)
    char fileNameStringCopy[MAX_FILE_NAME_LENGTH + 1];
    // get dictionary to process
    if (!defaultMode) { // if fielNameString is empty then ask for dictionary
      printf("Enter the filename of the dictionary (or 'Q' to quit): ");
      if (fgets(fileNameString, sizeof(fileNameString), stdin) == NULL) {
        perror("fgets failed\n");
        return FAILURE;
      }
      removeNewline(fileNameString);
      strcpy(fileNameStringCopy, fileNameString);
      convertEntireStringToLower(fileNameStringCopy);
      if (!strcmp(fileNameStringCopy, qString)) {
        printf("Exiting...\n");
        goto main_menu;
      }
    } else {
      strcpy(fileNameString, dictionaryAbsPath);
    }
    args.dictionaryFileName = realloc(args.dictionaryFileName, strlen(fileNameString) + 1);
    if (!args.dictionaryFileName) {
      perror("realloc failure for args struct dictionaryFileName\n");
      pthread_mutex_destroy(&lock);
      freePointer((void **)&threadIDs);
      freeArrayOfSpellingErrors(&(args.errorArray), args.prevSize);
      return FAILURE;
    }
    strcpy(args.dictionaryFileName, fileNameString);
    // get file to process
    printf("Enter the filename of the input file (or 'Q' to quit): ");
    if (fgets(fileNameString, sizeof(fileNameString), stdin) == NULL) {
      perror("fgets failed\n");
      return FAILURE;
    }
    removeNewline(fileNameString);
    strcpy(fileNameStringCopy, fileNameString);
    convertEntireStringToLower(fileNameStringCopy);
    if (!strcmp(fileNameStringCopy, qString)) {
      freePointer((void **)&(args.dictionaryFileName));
      freePointer((void **)&(args.spellcheckFileName));
      printf("Exiting...\n");
      goto main_menu;
    }
    pthread_mutex_lock(&lock);
    args.spellcheckFileName = realloc(args.spellcheckFileName, strlen(fileNameString) + 1);
    if (!args.spellcheckFileName) {
      perror("realloc failure for args struct spellcheckFileName\n");
      pthread_mutex_destroy(&lock);
      freePointer((void **)&threadIDs);
      freeArrayOfSpellingErrors(&(args.errorArray), args.prevSize);
      return FAILURE;
    }
    strcpy(args.spellcheckFileName, fileNameString);
    pthread_mutex_unlock(&lock);
    threadIDs = (pthread_t *) realloc(threadIDs, (sizeof(pthread_t) * (numThreadsStarted + 1)));
    if (!threadIDs) {
      printf("failed to realloc for threadIDs\n");
      freePointer((void **)&(args.dictionaryFileName));
      freePointer((void **)&(args.spellcheckFileName));
      pthread_mutex_destroy(&lock);
      return FAILURE;
    }
    threadIDs[numThreadsStarted] = 0;
    // printf("making thread %d\n", numThreadsStarted + 1);
    int error = pthread_create(&(threadIDs[numThreadsStarted]), NULL, threadFunction, (void *)&args);
    if (error != 0) {
      fprintf(stderr, "Thread can't be created: %s\n", strerror(error));
      free(args.dictionaryFileName);
      free(args.spellcheckFileName);
      free(threadIDs);
      if (args.errorArray) {
        free(args.errorArray);
      }
      pthread_mutex_destroy(&lock);
      freePointer((void **)&(args.dictionaryFileName));
      freePointer((void **)&(args.spellcheckFileName));
      return FAILURE;
    }
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&updatedVariables, &lock); // make sure the variables have been updated
    numThreadsInUse = args.numThreadsInUse;
    numThreadsStarted = args.numThreadsStarted;
    pthread_mutex_unlock(&lock);
    goto main_menu;
  case 2:
    printf("\nexiting...\n");
    char *outputString;
    pthread_mutex_lock(&lock);
    unsigned int numThreadsFinished = args.numThreadsFinished;
    numThreadsStarted = args.numThreadsStarted;
    numThreadsInUse = args.numThreadsInUse;
    pthread_mutex_unlock(&lock);
    printf("Number of threads started: %d\n", numThreadsStarted);
    printf("%d threads finished execution\n", numThreadsFinished);
    printf("%d threads running currently\n", numThreadsInUse);
    if (threadIDs) {
      for (int i = 0; i < numThreadsStarted; i++) {
        pthread_mutex_lock(&lock);
        printf("waiting for thread #%d (ID = %lu) -- %d threads still running\n", i + 1, threadIDs[i], args.numThreadsInUse);
        pthread_mutex_unlock(&lock);
        // join returns non zero int when it fails
        if (pthread_join(threadIDs[i], NULL)) {
          fprintf(stderr, "could not join thread with ID %lu\n", threadIDs[i]);
          freePointer((void **)&(args.dictionaryFileName));
          freePointer((void **)&(args.spellcheckFileName));
          pthread_mutex_destroy(&lock);
          return(FAILURE);
        }
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
    printf("\nExecution time: %lf\n", cpu_time_used);
    freePointer((void **)&(args.dictionaryFileName));
    freePointer((void **)&(args.spellcheckFileName));
    return SUCCESS; // end of main
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
  char *dictionaryFileName = malloc(strlen(data->dictionaryFileName) + 1);
  if (!dictionaryFileName) {
    perror("could not malloc dictionary file name inside thread function\n");
    pthread_mutex_unlock(&lock);
    return NULL;
  }
  strcpy(dictionaryFileName, data->dictionaryFileName);
  char *spellcheckFileName = malloc(strlen(data->spellcheckFileName) + 1);
  if (!spellcheckFileName) {
    free(dictionaryFileName);
    perror("could not malloc spellcheck file name inside thread function\n");
    pthread_mutex_unlock(&lock);
    return NULL;
  }
  strcpy(spellcheckFileName, data->spellcheckFileName);
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
  // printf("file to check (dictionary): %s\n", dictionaryFileName);
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
    free2DArray((void ***)&fileArrayOfStrings, wordCountFile);
    free2DArray((void ***)&dictionaryArrayOfStrings, wordCountDictionary);
    free(spellcheckFileName);
    free(dictionaryFileName);
    data -> numThreadsFinished++;
    data -> numThreadsInUse--;
    pthread_mutex_unlock(&lock);
    return NULL;
  }

  unsigned int countTotalMistakes, countInArr;
  spellingError *mistakes = compareFileData(dictionaryArrayOfStrings, fileArrayOfStrings,
  wordCountDictionary, wordCountFile, &countTotalMistakes, &countInArr);

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

  // if (writeThreadToFile(threadOutputFile, mistakes, countInArr) == FAILURE) {
  //   fprintf(stderr, "error with logging output of thread analysis to file %s!\n", threadOutputFile);
  //   free(mistakes);
  //   freeArrayOfSpellingErrors(&mistakes, data -> prevSize);
  //   goto exit_failure;
  // }
  
  // exit success
  pthread_mutex_lock(&lock);
  free(mistakes);
  data -> numThreadsInUse--;
  data -> threadSucccessCount++;
  data -> numThreadsFinished++;
  free(spellcheckFileName);
  free(dictionaryFileName);
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
  for (int i = 0; i < numElements; i++) {
    totalErrors += listOfMistakes[i].countErrors;
  }
  printToLog(debugFile, "total errors for file %s: %d\n", currentFileName, totalErrors);
  size_t strSize = strlen(currentFileName) + getNumDigitsInPositiveNumber(totalErrors, 10) + 3; // extra 3 for two spaces and \0
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
    // else {
    //   if (printToLog(debugFile, "mistake misspelled string at index %d for file %s is NULL\n", i, currentFileName) == ALTERNATE_SUCCESS) {
    //     fprintf(stderr, "mistake misspelled string at index %d for file %s is NULL\n", i, currentFileName);
    //   }
    // }
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
  if (fileName == NULL) {
    perror("fileName is NULL for readEntireFileIntoStr");
    return NULL;
  }
  int fd;
  // pthread_mutex_lock(&lock);
  if ((fd = open(fileName, O_RDONLY)) == -1) {
    printf("bad file descriptor. File %s most likely does not exist\n", fileName);
    return NULL;
  }
  int size = lseek(fd, 0, SEEK_END); // sizeof file in bytes
  char *readString = malloc(size + 1); // one byte extra for \0
  if (readString == NULL) {
    close(fd);
    perror("fatal error, malloc failed inside readEntireFileIntoStr\n");
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
    fprintf(stderr, "read failed! Expected %d bytes, received %d\n", size, numBytesRead);
    return NULL;
  }
  // pthread_mutex_unlock(&lock);
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
  if (hasDecimal && (digit >= 2) && !chars) {
    return FLOAT;
  }
  if (digit && !chars && !hasDecimal) {
    return UINTEGER;
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
  // clock_t start, end;

  if (!(singleFileString = readEntireFileIntoStr(fileName, &sizeBytes))) {
    return NULL;
  }
  convertEntireStringToLower(singleFileString);
  // start = clock();
  stringArrToReturn = splitStringOnWhiteSpace(singleFileString, wordCount);
  // end = clock();
  // printf("time to split string on whitespace is: %lf\n", (double)(end - start) / CLOCKS_PER_SEC);
  free(singleFileString);
  if (stringArrToReturn == NULL) {
    return NULL;
  }
  return stringArrToReturn;
}

void convertEntireStringToLower(char *string) {
  if (!string) {
    // best to check in case
    return;
  }
  for (long long unsigned int index = 0; string[index]; index++) {
    // it is faster to NOT use tolower() conditionally...so just call it every time
    string[index] = tolower(string[index]);
  }
}

void getNonAlphabeticalCharsString(char *buffer) {
  buffer[0] = '\0';  // Reset the string to an empty string
  char temp[2] = {0};
  for (int i = CHAR_MIN; i <= CHAR_MAX; i++) {
    if (!isalpha(i) && i != '\'') { // Exclude alphabetic characters and '
      temp[0] = (char)i;
      strcat(buffer, temp); // Concatenate to result string
    }
  }
  return;
}

char **splitStringOnWhiteSpace(const char *inputString, unsigned int *wordCount) {
  if (!inputString || !wordCount) {
    fprintf(stderr, "passed in NULL\n");
    return NULL;
  }
  unsigned int wordCountLocal = 0;
  bool prevWasWhitespace = true;
  // Count the number of words
  char character;
  for (const char *ptr = inputString; *ptr != '\0'; ptr++) {
    character = *ptr;
    if (!(isalpha(character) || character == '\'')) { // apostrophe is special case
      if (prevWasWhitespace) {
        continue; // ignore whitespaces until non-whitespace char found
      }
      prevWasWhitespace = true;
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
  char **words = (char **) calloc(wordCountLocal, sizeof(char *));
  if (words == NULL) {
    fprintf(stderr, "Memory allocation failed for words arr\n");
    *wordCount = wordCountLocal;
    return NULL;
  }
  char delimiters[CHAR_MAX - CHAR_MIN + 1] = "\n";
  getNonAlphabeticalCharsString(delimiters);
  // printf("delims: %s\n", delimiters);
  char *token = strtok((char *) inputString, delimiters); // cast to char *
  int index = 0;
  while (token != NULL) {
    words[index] = strdup(token); // Allocate memory for each word
    if (words[index] == NULL) {
      fprintf(stderr, "Memory allocation failed for strdup inside splitOnWhitespace");
      free2DArray((void ***)&words, wordCountLocal);
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
  spellingError temp = arr[++i];
  arr[i] = arr[end];
  arr[end] = temp;
  return i;
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

unsigned int hash(const char *key, size_t tableSize) {
  unsigned int hashVal = 0;
  while (*key != '\0') {
    hashVal = (hashVal << 5) + *key++; // increment happens BEFORE dereference
  }
  return hashVal % tableSize;
}

HashMap *createHashMap(size_t size) {
  HashMap *map = malloc(sizeof(HashMap));
  if (map == NULL) {
    perror("Could not malloc space for hashmap!\n");
    return NULL;
  }
  map->buckets = calloc(size, sizeof(HashNode *));
  if (map->buckets == NULL) {
    free(map);
    perror("Could not create buckets for hashmap!\n");
    return NULL;
  }
  map->size = size;
  return map;
}

void destroyStringEntry(StringEntry *entry) {
  // assumes entry is a valid address
  free(entry->key);
}

void destroyHashNode(HashNode *node) {
  // assumes node is a valid address
  destroyStringEntry(&(node->entry));
  free(node);
}

void destroyHashMap(HashMap *map) {
  if (map == NULL) {
    return;
  }
  for (size_t i = 0; i < map->size; i++) {
    HashNode *current = map->buckets[i];
    while (current != NULL) {
      HashNode *temp = current;
      current = current->next;
      destroyHashNode(temp);
    }
  }
  free(map->buckets);
  free(map);
}

HashNode *lookupInHashMap(HashMap *map, const char *key) {
  // returns NULL if the key is NOT in the hashmap else pointer to node
  if (map == NULL || key == NULL) {
    perror("map or key are NULL\n");
    return NULL;
  }
  unsigned int index = hash(key, map->size);
  HashNode *current = map->buckets[index];
  while (current != NULL) {
    if (strcmp(current->entry.key, key) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

bool addToHashMap(HashMap *map, const char *key, unsigned int occurrences) {
  if (map == NULL || key == NULL) {
    perror("map or key are NULL\n");
    return false;
  }
  unsigned int index = hash(key, map->size);
  HashNode *current = map->buckets[index];
  // Check if the key already exists in the map
  while (current != NULL) {
    if (strcmp(current->entry.key, key) == 0) {
      // Update the occurrences count
      current->entry.occurrences += occurrences;
      return true;
    }
    current = current->next;
  }
  // Key not found, add a new entry
  HashNode *newNode = malloc(sizeof(HashNode));
  if (newNode == NULL) {
    perror("could not malloc a new HashNode!\n");
    return false;
  }
  newNode->entry.key = strdup(key);
  if (newNode->entry.key == NULL) {
    perror("strdup failed for key!\n");
    free(newNode);
    return false;
  }
  newNode->entry.occurrences = occurrences;
  newNode->next = map->buckets[index];
  map->buckets[index] = newNode;
  return true;
}

spellingError *compareFileData(char **dictionaryData, char **fileData, const unsigned int numEntriesDictionary,
const unsigned int numEntriesFile, unsigned int *countTotalMistakes, unsigned int *countInArr) {

  *countTotalMistakes = 0;
  *countInArr = 0;
  spellingError *mistakesArr = NULL; // no entries yet

  // clock_t f_start, f_end;
  // f_start = clock();
  // clock_t start, end;
  // start = clock();
  quicksortStrings(dictionaryData, 0, numEntriesDictionary - 1);
  // end = clock();
  // printFlush("Time to sort dictionary: %lf\n", ((double)(end - start)) / CLOCKS_PER_SEC);
  // start = clock();
  quicksortStrings(fileData, 0, numEntriesFile - 1);
  // end = clock();
  // printFlush("Time to sort spellcheck file: %lf\n", ((double)(end - start)) / CLOCKS_PER_SEC);

  // Create hash map for seen words
  HashMap *seenWordsMap = createHashMap(numEntriesFile);
  if (seenWordsMap == NULL) {
    perror("Failed to create hash map for seen words");
    return NULL;
  }

  // Populate hash map with unique words and their occurrences
  for (unsigned int i = 0; i < numEntriesFile; i++) {
    HashNode *node = lookupInHashMap(seenWordsMap, fileData[i]);
    if (node == NULL) {
      // Word not seen before, add it to the map
      if (!addToHashMap(seenWordsMap, fileData[i], 1)) {
        fprintf(stderr, "Failed to add word %s to hash map", fileData[i]);
        destroyHashMap(seenWordsMap);
        return NULL;
      }
    } else {
      // Increment occurrences count for existing word
      node->entry.occurrences++;
    }
  }
  // printf("There are %u entries in the hashmap\n", numEntriesFile);
  size_t uniqueEntries = 0;
  for (size_t i = 0; i < numEntriesFile; i++) {
    HashNode *current = seenWordsMap->buckets[i];
    while (current != NULL) {
      uniqueEntries++;
      current = current->next;
    }
  }
  // printf("There are %lu unique entries in the hashmap\n", uniqueEntries);
  StringEntry * list = malloc(sizeof(StringEntry) * uniqueEntries);
  if (list == NULL) {
    perror("bad list malloc");
    destroyHashMap(seenWordsMap);
    return NULL;
  }
  for (size_t i = 0, list_i = 0; i < numEntriesFile && list_i < uniqueEntries; i++) {
    HashNode *current = seenWordsMap->buckets[i];
    while (current != NULL) {
      list[list_i].key = strdup(current->entry.key);
      if (list[list_i].key == NULL) {
        perror("failed strudp for unique hash map list");
        while(list_i >= 1) {
          free(list[--list_i].key);
        }
        free(list);
        destroyHashMap(seenWordsMap);
        return NULL;
      }
      list[list_i++].occurrences = current->entry.occurrences;
      current = current->next;
    }
  }
  // Check each unique word against the dictionary
  unsigned int temp;
  for (unsigned int i = 0; i < uniqueEntries; i++) {
    if ((numberOfStringMatchesInArrayOfStrings((const char **) dictionaryData, numEntriesDictionary, list[i].key) == 0)) {
      // Word not found in dictionary, add to mistakes array
      // printf("string %s not in dictionary\n", list[i].key);
      temp = *countInArr;
      mistakesArr = realloc(mistakesArr, (++(*countInArr)) * sizeof(spellingError));
      if (mistakesArr == NULL) {
        perror("Failed to reallocate memory for mistakes array");
        for (size_t i = 0; i < uniqueEntries; i++) {
          free(list[i].key);
        }
        free(list);
        destroyHashMap(seenWordsMap);
        return NULL;
      }
      // Populate mistake entry
      mistakesArr[temp].countErrors = list[i].occurrences;
      mistakesArr[temp].misspelledString = strdup(list[i].key);
      if (mistakesArr[temp].misspelledString == NULL) {
        perror("Failed to duplicate misspelled string");
        for (size_t i = 0; i < uniqueEntries; i++) {
          free(list[i].key);
        }
        free(list);
        destroyHashMap(seenWordsMap);
        return NULL;
      }
      mistakesArr[temp].dictionaryName = NULL;
      mistakesArr[temp].fileName = NULL;
      // Update total mistakes count
      (*countTotalMistakes) += list[i].occurrences;
    }
  }
  // Clean up
  for (size_t i = 0; i < uniqueEntries; i++) {
    free(list[i].key);
  }
  free(list);
  destroyHashMap(seenWordsMap);
  if (mistakesArr == NULL) {
    printf("No mistakes\n");
    mistakesArr = malloc(sizeof(spellingError));
    if (mistakesArr == NULL) {
      perror("could not malloc mistakes array!\n");
      return NULL;
    }
    mistakesArr[0].countErrors = 0;
    mistakesArr[0].dictionaryName = NULL;
    mistakesArr[0].fileName = NULL;
    mistakesArr[0].misspelledString = NULL;
    *countInArr = 1;
  }
  // f_end = clock();
  // printFlush("Time to fully process spelling error file with dictionary: %lf\n", ((double)(f_end - f_start)) / CLOCKS_PER_SEC);
  return mistakesArr;
}

unsigned int numberOfStringMatchesInArrayOfStrings(const char **arrayOfStrings, unsigned int numStrings, const char *target) {
  // Assume sorted input, use binary search
  int left = 0, right = numStrings - 1, mid, comparison, count = 0;
  while (left <= right) {
    mid = left + (right - left) / 2;
    comparison = strcmp(arrayOfStrings[mid], target);
    if (comparison == 0) {
      // Increment count if target string matches current string
      count++;
      // Search for more matches to the left
      int i = mid - 1;
      while (i >= left && strcmp(arrayOfStrings[i], target) == 0) {
        count++;
        i--;
      }
      // Search for more matches to the right
      i = mid + 1;
      while (i <= right && strcmp(arrayOfStrings[i], target) == 0) {
        count++;
        i++;
      }
      // Return count of matches
      return count;
    } else if (comparison < 0) {
      left = mid + 1; // If target is greater, ignore left half
    } else {
      right = mid - 1; // If target is smaller, ignore right half
    }
  }
  // If target string is not found, return 0
  return 0;
}

void quicksortStrings(char **arr, int left, int right) {
  if (left < right) {
    int i = left, j = right;
    char *pivot = arr[(left + right) / 2];

    // Partition
    while (i <= j) {
      while (strcmp(arr[i], pivot) < 0)  {
        i++;
      }
      while (strcmp(arr[j], pivot) > 0) {
        j--;
      }
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
  spellingError *array = *arrayOfMistakes; // safe to dereference
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
  if (addressOfGenericPointer == NULL || *addressOfGenericPointer == NULL) {
    return;
  }
  void **genericPointer = (void **) *addressOfGenericPointer;
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
  const char *fileName = NULL, *mistake = NULL;
  unsigned int arrSize;
  unsigned int totalSize = 1; // start with one for \0
  for (unsigned int i = 0; i < numThreads; i++) {
    numUniqueMisspelledWords[i] = countMistakesForThread(threadArgs.errorArray, size, i);
    printToLog(debugFile, "number of unique errors for thread %d: %d\n", i + 1, numUniqueMisspelledWords[i]);
    // rough estimate for max size
    arrSize = MAX_WORD_LENGTH * numUniqueMisspelledWords[i] + MAX_FILE_NAME_LENGTH + max(numUniqueMisspelledWords[i] - 1, 0) * 2 + 2; // includes the n-1 ", " strings, \n, and \0
    totalSize += arrSize - 1;
    errorsInEachFile[i] = malloc(arrSize);
    if (!errorsInEachFile[i]) {
      fprintf(stderr, "malloc failed for errorsInEachFile[%d]\n", i);
      free(numUniqueMisspelledWords);
      for (unsigned int j = 0; j < i; j++) {
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
      for (unsigned int j = 0; j < numUniqueMisspelledWords[i]; j++) {
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

char *generateSummary(spellingError *errorArr, unsigned int numThreads, unsigned int arrayLength, char *inputString, unsigned int numSuccessfulThreads) {
  if (!inputString) {
    return NULL;
  }
  
  char formatString[250] = "";
  sprintf(formatString, "Number of files processed: %d\n", numSuccessfulThreads);
  unsigned int biggest = 0, secondBiggest = 0, thirdBiggest = 0, totalErrors = 0;
  char *biggestStr = NULL, *secondBiggestStr = NULL, *thirdBiggestStr = NULL;

  // printFlush("LENGTH OF input string: %d\n", (int)strlen(inputString));

  for (unsigned int i = 0; i < arrayLength; i++) {
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
  // inputString = realloc(inputString, strlen(inputString) + strlen(formatString) + 1);
  // strcat(inputString, formatString);
  char numWordsToPrint = (biggest != 0) + (secondBiggest != 0) + (thirdBiggest != 0);
  char *newStr = NULL;
  switch (numWordsToPrint) {
    case 1:
      newStr = "Most common misspelling: ";
      break;
    case 2:
      newStr = "Two most common misspellings: ";
      break;
    case 3:
      newStr = "Three most common misspellings: ";
      break;
    default:
      // if there are no mistakes at all
      newStr = "No mistakes!";
      inputString = realloc(inputString, strlen(inputString)  + strlen(newStr) + 1);
      strcat(inputString, newStr);
      return inputString;
  }
  inputString = realloc(inputString, strlen(inputString)  + strlen(newStr) + 1);
  strcat(inputString, newStr);
  for (char i = 0; i < numWordsToPrint; i++) {
    if (i == 0) {
      sprintf(formatString, "%s (%d times)", biggestStr, biggest);
    } else if (i == 1) {
      sprintf(formatString, "%s (%d times)", secondBiggestStr, secondBiggest);
    } else if (i == 2) {
      sprintf(formatString, "%s (%d times)", thirdBiggestStr, thirdBiggest);
    }
    inputString = realloc(inputString, strlen(inputString)  + strlen(formatString) + 1);
    if (!inputString) {
      perror("input string realloc failed inside generateSummary\n");
      exit(EXIT_FAILURE);
    }
    strcat(inputString, formatString);
    if (i < numWordsToPrint - 1) {
      inputString = realloc(inputString, strlen(inputString)  + 3);
      if (!inputString) {
        perror("input string realloc failed inside generateSummary\n");
        exit(EXIT_FAILURE);
      }
      strcat(inputString, ", ");
    }
  }
  return inputString;
}

const char *getMistakeAtIndex(spellingError *arr, unsigned int threadNum, unsigned int index, unsigned int numElements) {
  unsigned int count = 0;
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

int max(int a, int b) {
  return a < b ? b : a;
}

void printToOutputFile(const char *fileName, const char *stringToPrint) {
  int fd;
  if (firstWriteThreadOutput) {
    // create if it does not exist.
    if ((fd = open(fileName, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
      fprintf(stderr, "Error opening spellcheck file (first attempt)%s\n", fileName);
      return;
    }
    firstWriteThreadOutput = false;
  } else if ((fd = open(fileName, O_CREAT | O_WRONLY | O_APPEND, 0644)) == -1) {
    // open for appending
    fprintf(stderr, "Error opening spellcheck file %s\n", fileName);
    return;
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

unsigned int getNumDigitsInPositiveNumber(unsigned int positiveNumber, unsigned char base) {
  if (positiveNumber == 0) {
    return 1;
  }
  int digits = 0;
  while (positiveNumber) {
    positiveNumber /= base;
    digits++;
  }
  return digits;
}
