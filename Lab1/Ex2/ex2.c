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
void PrepareThreads(pthread_t * writerThreads, pthread_t * readerThreads);
void Error(char* error_meesage);
int GetRandom(int max_value);

//SEMAPHORES
void DestroyObjectSemaphores();
void AskForObjectToWrite(int ObjectID);
void EndWritingObject(int ObjectID);
void AskForObjectToRead(int ObjectID);
void EndReadingObject(int ObjectID) ;

//BUFFER
void Buffer();
char* WriteToBuffer();
void LockBuffer();
void UnlockBuffer();
char* WriteFromBuffer(int ObjectID, char buffered_text[], int text_length);

void InitSemaphores() {
    for(int i=0; i<OBJECTS_COUNT; i++) {
        sem_t read_semaphore, write_semaphore;

        if(sem_init(&read_semaphore, 0, READERS_COUNT)!=0)
            Error("Couldn't initialize object's read semaphore");
        if(sem_init(&write_semaphore, 0, 1)!=0)
            Error("Couldn't initialize object's write semaphore");

        OBJECT_READ_SEMAPHORE[i]=read_semaphore;
        OBJECT_WRITE_SEMAPHORE[i]=write_semaphore;
    }

    sem_t buffer_semaphore;
    if(sem_init(&buffer_semaphore, 0, 1)!=0)
        Error("Couldn't initialize buffer semaphore");

    BUFFER_SEMAPHORE = buffer_semaphore;
}

int main(int argc, char* argv[])
{
    srand(100005);
    InitSemaphores();
    usleep(5000);
    pthread_t writerThreads[WRITERS_COUNT];
    pthread_t readerThreads[READERS_COUNT];
    pthread_t bufferThread;

    //Initializing Buffer thread
    int rc = pthread_create(
                &bufferThread,      // thread identifier
                NULL,               // thread attributes
                (void*) Buffer,     // thread function
                (void*) NULL        // thread function argument
            );
    if(rc!=0) Error("Couldn't create buffer thread");

    int i;

    usleep(3000);

    for (i = 0; i < WRITERS_COUNT; i++) {
        usleep(GetRandom(100));
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

    for(int i=0; i<OBJECTS_COUNT; i++) {
        printf("Buffer %d: %s\n", i+1, BUFFER[i]);
        printf("Book %d: %s\n", i+1, OBJECTS[i]);
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

        // end reading object
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

        printf("(Writer %d)\tWriting to Book %d: %s\n", threadId, objectID+1, WriteToBuffer(objectID));
        fflush(stdout);

        usleep(GetRandom(2000)); //WriterThinking
    }
    free(data);

    return 0;
}

char* Read(int ObjectID) {
    return OBJECTS[ObjectID];
}

char* Write(int ObjectID) {
    int object_end=0;
    while(OBJECTS[ObjectID][object_end] != '\0') object_end++;

    char writtenText[TEST_TEXT_LENGTH] = "Text ";

    for(int i=0; i<TEST_TEXT_LENGTH; i++) {
        OBJECTS[ObjectID][object_end+i] = writtenText[i];
    }

    return OBJECTS[ObjectID];
}

void Buffer(void* data) {
    for(int i=0; i<OBJECTS_COUNT; i++) {
        BUFFER_START[i] = 0;
        BUFFER_END[i] = 0;
    }

    while (1)
    {
        usleep(GetRandom(2000));
        LockBuffer();
        for(int i=0; i<OBJECTS_COUNT; i++) {
            int text_length = BUFFER_END[i]-BUFFER_START[i];

            if(text_length<=0) continue;
            if(text_length>BUFFER_UPDATED_TEXT_LENGTH) text_length = BUFFER_UPDATED_TEXT_LENGTH;

            char buffered_text[BUFFER_UPDATED_TEXT_LENGTH];
            for(int j=0; j<text_length; j++) {
                int read_point = BUFFER_START[i]+j;
                buffered_text[j] = BUFFER[i][read_point];
            }

            // ask for object to write
            if(sem_wait(&OBJECT_WRITE_SEMAPHORE[i])!=0)
                Error("Couldn't increment object's write semaphore");
            for(int j=0; j<READERS_COUNT; j++) {
                if(sem_wait(&OBJECT_READ_SEMAPHORE[i])!=0)
                    Error("Couldn't increment object's read semaphore");
            }

            printf("(Buffer)\tUpdated Book %d: %s\n", i+1, WriteFromBuffer(i, buffered_text, text_length));
            fflush(stdout);

            if(text_length <= BUFFER_UPDATED_TEXT_LENGTH) {
                BUFFER_START[i]=0;
                BUFFER_END[i]=0;
            }
            else {
                BUFFER_START[i] = BUFFER_START[i]+text_length;
            }

            // end writitng objects
            for(int j=0; j<READERS_COUNT; j++) {
                if(sem_post(&OBJECT_READ_SEMAPHORE[i])!=0)
                    Error("Couldn't decrement object's read semaphore");
            }
            if(sem_post(&OBJECT_WRITE_SEMAPHORE[i])!=0)
                Error("Couldn't decrement object's write semaphore");

        }
        UnlockBuffer();
    }
}

void LockBuffer() {
    if(sem_wait(&BUFFER_SEMAPHORE)!=0) Error("Couldn't decrement buffer's semaphore");
}

void UnlockBuffer() {
    if(sem_post(&BUFFER_SEMAPHORE)!=0) Error("Couldn't increment buffer's semaphore");
}

char* WriteToBuffer(int ObjectID) {
    char writtenText[TEST_TEXT_LENGTH] = "Text ";

    LockBuffer();

    while(BUFFER_LENGTH-BUFFER_END[ObjectID] < TEST_TEXT_LENGTH) {
        UnlockBuffer();
        usleep(GetRandom(3000));
        LockBuffer();
    }

    for(int i=0; i<TEST_TEXT_LENGTH; i++) {
        int write_point = BUFFER_END[ObjectID]+i;
        BUFFER[ObjectID][write_point] = writtenText[i];
    }

    BUFFER_END[ObjectID] = BUFFER_END[ObjectID] + TEST_TEXT_LENGTH;
    UnlockBuffer();

    return "...written to buffer";
}

char* WriteFromBuffer(int ObjectID, char buffered_text[], int text_length) {
    int object_end=0;
    while(OBJECTS[ObjectID][object_end] != '\0') object_end++;

    for(int i=0; i<text_length; i++) {
        OBJECTS[ObjectID][object_end+i] = buffered_text[i];
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
