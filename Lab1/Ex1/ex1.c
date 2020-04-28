#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>
#include <semaphore.h>

#define READERS_COUNT 10
#define READER_TURNS 5
#define WRITERS_COUNT 4
#define WRITER_TURNS 5

#define OBJECTS_COUNT 1
#define OBJECT_LENGTH 1000

#define TEST_TEXT_LENGTH 5

char OBJECTS[OBJECTS_COUNT][OBJECT_LENGTH];

sem_t OBJECT_READ_SEMAPHORE[OBJECTS_COUNT];
sem_t OBJECT_WRITE_SEMAPHORE[OBJECTS_COUNT];

int Reader(void* data);
char* Read(int ReaderID, int ObjectID);
int Writer(void* data);
char* Write(int WriterID, int ObjectID);
void Error(char* error_meesage);
int GetRandom(int max_value);

void InitSemaphores() {
    for(int i=0; i<OBJECTS_COUNT; i++) {
        sem_t read_semaphore, write_semaphore;

        if( sem_init(&read_semaphore, 0, READERS_COUNT) != 0 )
            Error("Couldn't initialize object's read semaphore");
        if( sem_init(&write_semaphore, 0, 1) != 0 )
            Error("Couldn't initialize object's write semaphore");

        OBJECT_READ_SEMAPHORE[i] = read_semaphore;
        OBJECT_WRITE_SEMAPHORE[i] = write_semaphore;
    }
}

int main(int argc, char* argv[])
{
    srand(100005);
    InitSemaphores();
    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];

    int rc, i;

    // Create the Writer thread
    for (i = 0; i < WRITERS_COUNT; i++) {
        int* threadId = malloc(sizeof(int));
        *threadId = i+1;

        rc = pthread_create(
                &writerThreads[i],  // thread identifier
                NULL,               // thread attributes
                (void*) Writer,     // thread function
                (void*) threadId    // thread function argument
            );

        if (rc != 0) Error("Couldn't create the writer thread");
    }

    // Create the Reader threads
    for (i = 0; i < READERS_COUNT; i++) {
	      usleep(GetRandom(1000)); // Reader initialization - takes random amount of time

        int* threadId = malloc(sizeof(int));
        *threadId = i+1;

	      rc = pthread_create(
                &readerThreads[i], // thread identifier
                NULL,              // thread attributes
                (void*) Reader,    // thread function
                (void*) threadId   // thread function argument
        );

        if (rc != 0) Error("Couldn't create the reader threads");
    }
    // At this point, the readers and writers should perform their operations

    // Wait for the Readers
    for (int i = 0; i < READERS_COUNT; i++)
        pthread_join(readerThreads[i],NULL);

    // Wait for the Writer
    for (int i = 0; i < WRITERS_COUNT; i++)
        pthread_join(writerThreads[i],NULL);

    // Destroy object semaphores
    for(int i=0; i<OBJECTS_COUNT; i++) {
        if( sem_destroy(&OBJECT_READ_SEMAPHORE[i]) != 0 )
            Error("Couldn't destory object's read semaphore");
        if( sem_destroy(&OBJECT_WRITE_SEMAPHORE[i]) != 0 )
            Error("Couldn't destory object's write semaphore");
    }

    return (0);
}

// Reader thread function
int Reader(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS; i++) {
        int objectID = GetRandom(OBJECTS_COUNT);

        // ask for object to read
        if( sem_wait(&OBJECT_READ_SEMAPHORE[objectID]) != 0 )
            Error("Couldn't decrement object's semaphore");

        printf("(Reader %d)\tReading Book %d: %s\n", threadId, objectID+1, Read(threadId, objectID));
        fflush(stdout);
        usleep(GetRandom(200));

        // end reading object
        if( sem_post(&OBJECT_READ_SEMAPHORE[objectID]) != 0 )
            Error("Couldn't increment object's semaphore");

        usleep(GetRandom(1000));
    }
    free(data);

    return 0;
}

// Writer thread function
int Writer(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < WRITER_TURNS; i++) {
        // Write
        int objectID = GetRandom(OBJECTS_COUNT);

        // ask for object to write
        if(sem_wait(&OBJECT_WRITE_SEMAPHORE[objectID]) != 0)
            Error("Couldn't decrement object's write semaphore");

        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_wait(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
                Error("Couldn't decrement object's read semaphore");
        }

        printf("(Writer %d)\tWriting Book %d: %s\n", threadId, objectID+1, Write(threadId, objectID));
        fflush(stdout);

        // end writing objects
        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_post(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
                Error("Couldn't increment object's read semaphore");
        }

        if(sem_post(&OBJECT_WRITE_SEMAPHORE[objectID]) != 0)
            Error("Couldn't increment object's write semaphore");

        usleep(GetRandom(2000)); //WriterThinking
    }
    free(data);

    return 0;
}

char* Read(int ReaderID, int ObjectID) {
    return OBJECTS[ObjectID];
}

char* Write(int WriterID, int ObjectID) {
    int object_end = 0;
    while(OBJECTS[ObjectID][object_end] != '\0') object_end++;

    char writtenText[TEST_TEXT_LENGTH] = "Text ";

    for(int i=0; i<TEST_TEXT_LENGTH; i++) {
        OBJECTS[ObjectID][object_end+i] = writtenText[i];
    }

    return OBJECTS[ObjectID];
}

void Error(char* error_meesage) {
    perror(error_meesage);
    exit(-1);
}

int GetRandom(int max_value) {
    return rand()%max_value;
}
