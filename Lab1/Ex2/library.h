#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/random.h>
#include <semaphore.h>

#define READERS_COUNT 10
#define READER_TURNS 4
#define WRITERS_COUNT 1
#define WRITER_TURNS 3

#define OBJECTS_COUNT 1
#define OBJECT_LENGTH 1000

#define TEST_TEXT_LENGTH 5

char OBJECTS[OBJECTS_COUNT][OBJECT_LENGTH];

sem_t OBJECT_READ_SEMAPHORE[OBJECTS_COUNT];
sem_t OBJECT_WRITE_SEMAPHORE[OBJECTS_COUNT];

#define BUFFER_LENGTH 100
#define BUFFER_UPDATED_TEXT_LENGTH 10
char BUFFER[OBJECTS_COUNT][BUFFER_LENGTH];
int BUFFER_START[OBJECTS_COUNT], BUFFER_END[OBJECTS_COUNT];

sem_t BUFFER_SEMAPHORE;