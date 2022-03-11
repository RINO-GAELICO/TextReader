#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define LONGEST_WORD_ENGLISH 45
#define MIN_LENGTH_WORD 6
#define TOP_NUM 10

// You may find this Useful
char *delim = "\"\'.“”‘’?:;-,—*($%)! \t\n\x0A\r";

// Function prototypes
void *processChunk(void *param); // threads call this function
void resizeArray(int newSize);
void initilizeArray(); // resize Array

// Hash element is an object that associates the C string with the tally of that particular word
typedef struct HashElement
{
    char *word;
    int tally;

} HashElement;

// Statically allocated variables
HashElement *arrayTally;
int fd, chunkSize, numberWords, indexArray = 0;
pthread_mutex_t lock;

int main(int argc, char *argv[])
{
    //***TO DO***  Look at arguments, open file, divide by threads
    //             Allocate and Initialize and storage structures
    int fileSize, threadCount;
    char *fileName;

    // we assume the first argument provided is filename
    if (argc > 1)
    {
        fileName = argv[1];
    }
    else
    {
        printf("Filename not provided\n");
        exit(1);
    }

    // we assume the second argument provided is the thead count
    if (argc > 2)
    {
        threadCount = atoi(argv[2]);
    }
    else
    {
        printf("Thread count not provided\n");
        exit(1);
    }

    // initialize a mutex to its default value
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("Mutex initilization failed\n");
        exit(EXIT_FAILURE);
    }

    // opening a file for Read only
    fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror("error");
    }
    // getting the filesize using lseek that moves the offset to the end of the file
    fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize == -1)
    {
        perror("error");
    }
    // repositioning the offset at the beginning of the file
    lseek(fd, 0, SEEK_SET);

    // calculating the chunk's size keeping into account the remainder for the last chunk
    chunkSize = (fileSize / threadCount);
    chunkSize = chunkSize + (fileSize % threadCount);

    // calculating the maximum number of words that we could find with more than 6 characters
    numberWords = fileSize / (MIN_LENGTH_WORD + 1);

    // allocating memory for the array based on the maximum possible number of words
    arrayTally = (HashElement *)malloc(numberWords * sizeof(HashElement));

    // check for malloc failure
    if (arrayTally == NULL)
    {
        exit(EXIT_FAILURE);
    }

    //**************************************************************
    // DO NOT CHANGE THIS BLOCK
    // Time stamp start
    struct timespec startTime;
    struct timespec endTime;

    clock_gettime(CLOCK_REALTIME, &startTime);
    //**************************************************************

    // an array of threads to perform the task of counting the words
    pthread_t workers[threadCount];

    // an array of int to hold the offset values to be used by the thread function
    int offSet[threadCount];
    int offSetCount = 0;

    // creating a series of threads that will process the chunks in parallel (as much as possible)
    for (int i = 0; i < threadCount; i++)
    {

        // creates thread
        offSet[i] = offSetCount;
        pthread_create(&workers[i], NULL, processChunk, (void *)&offSet[i]);

        // updating the offset value to be recorded into the offset array
        offSetCount += chunkSize;
    }

    // waiting each thread to finish
    for (int i = 0; i < threadCount; i++)
    {
        pthread_join(workers[i], NULL);
    }

    // ***TO DO *** Process TOP 10 and display

    HashElement temp;

    // sorting the words in decreasing order using a selection sort algorithm
    for (int i = 0; i < indexArray; ++i)
    {
        for (int j = i + 1; j < indexArray; ++j)
        {
            if (arrayTally[i].tally < arrayTally[j].tally)
            {
                temp = arrayTally[i];
                arrayTally[i] = arrayTally[j];
                arrayTally[j] = temp;
            }
        }
    }

    printf("\n");
    printf("Word Frequency Count on %s with %d threads\n", fileName, threadCount);
    printf("Printing top %d words %d characters or more.\n", TOP_NUM, MIN_LENGTH_WORD);

    for (int index = 0; index < TOP_NUM; index++)
    {
        printf("Number %d is %s with a count of %d\n", index + 1, arrayTally[index].word, arrayTally[index].tally);
    }

    // DO NOT CHANGE THIS BLOCK
    // Clock output
    clock_gettime(CLOCK_REALTIME, &endTime);
    time_t sec = endTime.tv_sec - startTime.tv_sec;
    long n_sec = endTime.tv_nsec - startTime.tv_nsec;
    if (endTime.tv_nsec < startTime.tv_nsec)
    {
        --sec;
        n_sec = n_sec + 1000000000L;
    }

    printf("Total Time was %ld.%09ld seconds\n", sec, n_sec);
    //**************************************************************

    // ***TO DO *** cleanup

    // freeing the memory allocated for each string representing the words
    for (int i = 0; i < indexArray; ++i)
    {
        free(arrayTally[i].word);
        arrayTally[i].word = NULL;
    }

    // freeing the rest of the memory allocated and closing the file
    free(arrayTally);
    arrayTally = NULL;
    close(fd);
    pthread_mutex_destroy(&lock);
}

void *processChunk(void *param)
{
    // casting the parameter into a int pointer
    int *offSetThread = (int *)param;

    // dinamically allocating memory for the buffer and token, +1 for null terminator
    char *bufferThread;
    bufferThread = (char *)malloc((sizeof(char) * chunkSize) + 1);
    if (bufferThread == NULL)
    {
        printf("malloc not successfull\n");
        exit(EXIT_FAILURE);
    }

    char *token = (char *)malloc(LONGEST_WORD_ENGLISH + 1);
    if (token == NULL)
    {
        printf("malloc not successfull\n");
        exit(EXIT_FAILURE);
    }

    // null pointer to provide context to the strtok_r function
    char *context = NULL;

    pread(fd, bufferThread, chunkSize, *offSetThread);

    token = strtok_r(bufferThread, delim, &context);

    // inside the loop while the token is something different than null
    while (token != NULL)
    {

        // we found a word that has 6 or more characters
        if (strlen(token) >= MIN_LENGTH_WORD)
        {
            // check one by one if the token word is in the array
            int i = 0;
            int compare = 1;

            while (i < indexArray && compare != 0)
            {
                compare = strcasecmp(arrayTally[i].word, token);
                if (compare == 0)
                {

                    // we found the word let's increase the count
                    // also let's protect this section to ensure the shared data is modified properly
                    pthread_mutex_lock(&lock);
                    arrayTally[i].tally++;
                    pthread_mutex_unlock(&lock);
                }
                i++;
            }

            // we didn't found the word into the array, let's create a new entry
            if (compare != 0)
            {

                // check if there is space
                if (indexArray < numberWords)
                {
                    // we protect this section because we are about to write into the shared data structure
                    pthread_mutex_lock(&lock);
                    // allocate memory for char pointer in each entry
                    arrayTally[indexArray].word = malloc(strlen(token) + 1);
                    strcpy(arrayTally[indexArray].word, token);
                    arrayTally[indexArray].tally = 1;
                    indexArray++;
                    pthread_mutex_unlock(&lock);
                }
                else if (indexArray >= numberWords)
                { // add space if needed (normally we shouldn't get here)
                    pthread_mutex_lock(&lock);
                    // checking if the thread has been already resized by another thread
                    if (indexArray >= numberWords)
                    {
                        resizeArray(indexArray * 2);
                    }
                    arrayTally[indexArray].word = malloc(strlen(token) + 1);
                    strcpy(arrayTally[indexArray].word, token);
                    arrayTally[indexArray].tally = 1;
                    indexArray++;
                    pthread_mutex_unlock(&lock);
                }
            }
        }
        // next word stored in token
        token = strtok_r(NULL, delim, &context);
    }

    // let's free the memory allocated so far within this function
    free(bufferThread);
    free(token);
    bufferThread = NULL;
    token = NULL;

    return 0;
}

void resizeArray(int newSize)
{
    // in case we need more space it reallocates the memory making sure the array stays in order
    arrayTally = realloc(arrayTally, newSize * (sizeof(HashElement)));
    // check for malloc failure
    if (arrayTally == NULL)
    {
        exit(EXIT_FAILURE);
    }
    return;
}
