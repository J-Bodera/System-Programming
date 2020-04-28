#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>
#include <semaphore.h>

#define READERS_COUNT 20
#define READER_TURNS 6
#define WRITERS_COUNT 4
#define WRITER_TURNS 6

#define OBJECTS_COUNT 5
#define OBJECT_LENGTH 1000

int OBJECT_MAX_READERS[OBJECTS_COUNT] = {4, 2, 5, 10, 13};

#define TEST_TEXT_LENGTH 5

char OBJECTS[OBJECTS_COUNT][OBJECT_LENGTH];

sem_t OBJECT_READ_SEMAPHORE[OBJECTS_COUNT];
sem_t OBJECT_WRITE_SEMAPHORE[OBJECTS_COUNT];