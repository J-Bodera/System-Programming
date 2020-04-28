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
#define WRITER_TURNS 2

#define OBJECTS_COUNT 10
#define OBJECT_LENGTH 1000

#define TEST_TEXT_LENGTH 5

char OBJECTS[OBJECTS_COUNT][OBJECT_LENGTH];
int WRITERS_WRITE_COUNT[WRITERS_COUNT];

sem_t OBJECT_READ_SEMAPHORE[OBJECTS_COUNT];
sem_t OBJECT_WRITE_SEMAPHORE[OBJECTS_COUNT];

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond   = PTHREAD_COND_INITIALIZER;