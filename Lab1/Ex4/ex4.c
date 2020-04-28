#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>
#include <semaphore.h>

#include "library.h"

int Reader(void* data);
char* Read(int ObjectID);
int Writer(void* data);
char* Write(int ObjectID);
void Error(char* error_meesage);
int GetRandom(int max_value);

void Critic(void* data);
char* Review(int ObjectID);
int FullfilledWritersCount();

void InitSemaphores() {
    for(int i=0; i<OBJECTS_COUNT; i++) {
        sem_t read_semaphore, write_semaphore;

        if(sem_init(&read_semaphore, 0, READERS_COUNT) != 0)
            Error("Couldn't initialize object's read semaphore");
        if(sem_init(&write_semaphore, 0, 1) != 0)
            Error("Couldn't initialize object's write semaphore");

        OBJECT_READ_SEMAPHORE[i]=read_semaphore;
        OBJECT_WRITE_SEMAPHORE[i]=write_semaphore;
    }
}

int main(int argc, char* argv[])
{
    srand(100005);
    InitSemaphores();
    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];
    pthread_t criticThread;

    for(int i=0; i<WRITERS_COUNT; i++) {
        WRITERS_WRITE_COUNT[i]=0;
    }

    //Initializing Buffer thread
    int rc = pthread_create(
                &criticThread,      // thread identifier
                NULL,               // thread attributes
                (void*) Critic,     // thread function
                (void*) NULL        // thread function argument
            );
    if(rc!=0) Error("Couldn't create critic's thread");

    int i;

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

        if (rc != 0) Error ("Couldn't create the writer thread");
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

    // destroy object semaphores
    for(int i=0; i<OBJECTS_COUNT; i++) {
        if(sem_destroy(&OBJECT_READ_SEMAPHORE[i]) != 0)
            Error("Couldn't destory object's read semaphore");
        if(sem_destroy(&OBJECT_WRITE_SEMAPHORE[i]) != 0)
            Error("Couldn't destory object's write semaphore");
    }
    return (0);
}

// Reader thread function
int Reader(void* data) {
    int i;
    int threadId = *(int*) data;

    for (i = 0; i < READER_TURNS; i++) {
        // Read
        int objectID = GetRandom(OBJECTS_COUNT);

        // ask for object to read
        if(sem_wait(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
            Error("Couldn't decrement object's semaphore");


        printf("(Reader %d)\tReading Book %d: %s\n", threadId, objectID+1, Read(objectID));
        fflush(stdout);
        usleep(GetRandom(200));

        // end reading objects
        if(sem_post(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
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
            Error("Couldn't increment object's write semaphore");
        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_wait(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
                Error("Couldn't increment object's read semaphore");
        }

        printf("(Writer %d)\tWriting Book %d: %s\n", threadId, objectID+1, Write(objectID));
        fflush(stdout);

        // end writing object
        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_post(&OBJECT_READ_SEMAPHORE[objectID]) != 0)
                Error("Couldn't decrement object's read semaphore");
        }
        if(sem_post(&OBJECT_WRITE_SEMAPHORE[objectID]) != 0)
            Error("Couldn't decrement object's write semaphore");

        pthread_mutex_lock(&mutex);

        WRITERS_WRITE_COUNT[threadId-1]++;
        if(FullfilledWritersCount()>=WRITERS_COUNT) pthread_cond_broadcast(&cond);

        pthread_mutex_unlock(&mutex);

        usleep(GetRandom(2000)); //WriterThinking
    }
    free(data);

    return 0;
}

char* Read(int ObjectID) {
    return OBJECTS[ObjectID];
}

char* Write(int ObjectID) {
    usleep(GetRandom(2000)); //wirting

    int object_end=0;
    while(OBJECTS[ObjectID][object_end] != '\0') object_end++;

    char writtenText[TEST_TEXT_LENGTH] = "Text ";

    for(int i=0; i<TEST_TEXT_LENGTH; i++) {
        OBJECTS[ObjectID][object_end+i] = writtenText[i];
    }

    return OBJECTS[ObjectID];
}

void Critic(void* data) {
    usleep(4000); //wirting
    pthread_mutex_lock(&mutex);

    while (FullfilledWritersCount() < WRITERS_COUNT) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    for(int i=0; i<OBJECTS_COUNT; i++) {
        usleep(GetRandom(2000)); //reviewing

        // ask for object to write
        if(sem_wait(&OBJECT_WRITE_SEMAPHORE[i]) != 0)
            Error("Couldn't increment object's write semaphore");
        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_wait(&OBJECT_READ_SEMAPHORE[i]) != 0)
                Error("Couldn't increment object's read semaphore");
        }

        printf("(Critic)\tReviewing Book %d: %s\n", i+1, Review(i));
        fflush(stdout);

        // end writing object
        for(int i=0; i<READERS_COUNT; i++) {
            if(sem_post(&OBJECT_READ_SEMAPHORE[i]) != 0)
                Error("Couldn't decrement object's read semaphore");
        }
        if(sem_post(&OBJECT_WRITE_SEMAPHORE[i]) != 0)
            Error("Couldn't decrement object's write semaphore");
    }
}

char* Review(int ObjectID) {
    int object_end=0;
    while(OBJECTS[ObjectID][object_end] != '\0') object_end++;

    char badReview[TEST_TEXT_LENGTH] = "2/10 ";
    char goodReview[TEST_TEXT_LENGTH] = "9/10 ";

    if(OBJECTS[ObjectID][0] == '\0') {
        for(int i=0; i<TEST_TEXT_LENGTH; i++) {
            OBJECTS[ObjectID][object_end+i] = badReview[i];
        }
    }
    else {
        for(int i=0; i<TEST_TEXT_LENGTH; i++) {
            OBJECTS[ObjectID][object_end+i] = goodReview[i];
        }
    }

    return OBJECTS[ObjectID];
}

int FullfilledWritersCount() {
    int required_write_count = 2, fulfilled_writers=0;

    for(int i=0; i<WRITERS_COUNT; i++) {
        if(WRITERS_WRITE_COUNT[i]>=required_write_count) fulfilled_writers++;
    }
    return fulfilled_writers;
}

void Error(char* error_meesage) {
    perror(error_meesage);
    exit(-1);
}

int GetRandom(int max_value) {
    return rand()%max_value;
}
