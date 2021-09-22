/**
  Name: OS Assignment (process)
  Date: 07/05/18
  Author: Rares Popa
  Student ID: 19159700
  Unit: Operating Systems (COMP2006)

  This code contains my finished Operating Systems Assignment,
  Lisence for use is MIT
**/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <pthread.h>
#include "colour.h"   //Adds Colour to the output

//Define important Values with defaults
#define BUFFER_SIZE 20  //Buffer Size
#define FILENAME "shared_data.txt"  //Filename to read from
#define READERS 5   //Amount of readers
#define WRITERS 2   //Amount of writers
#define SLEEP_TIME 1  //Sleep amount

//Mutexes
sem_t mutex;   //General Mutex
sem_t rf_mutex;  //Mutex used in read_file
sem_t w_mutex;  //Mutex used in writer
sem_t *semaphoresPtr;
//Counters
int reader_count = 0;	//Define the amount of readers
int writer_count = 0;	//Define the amount of writers
//define file/writer/reader position
int file_pos, writer_pos, reader_pos;
//define num from file read and if allRead
int num, reader_amount = -1, writer_amount = -1;
/**
  --3 types of arrays--
  data_buffer - which holds values from the shared_data file (MAX 20 Values) (Shared memory)
  hasRead - Checks to see if the reader has read the data_buffer (MAX (READER) Values)
  readerPieces - Increments each time a reader has read the data_buffer
**/
//Header funtions
void signal_next(int *reader_count, sem_t **semaphores);
void writer(int pid, int (*data_buffer)[BUFFER_SIZE], int *allRead, int (*hasRead)[READERS],int *reader_count,int *writer_count, int *reader_pos, int *endOfFile, sem_t *semaphores, int *file_pos);
void reader(int pid, int (*data_buffer)[BUFFER_SIZE], int *allRead, int (*hasRead)[READERS],int *reader_count,int *writer_count, int *writer_pos, int *endOfRead, int (*readerPieces)[READERS], sem_t *semaphores);
void read_file(int pid, int (**data_buffer)[BUFFER_SIZE], int **allRead, int (**hasRead)[READERS], int **reader_pos, int **endOfRead, int (**readerPieces)[READERS], sem_t **semaphores);
void write_file(int pid, int (**data_buffer)[BUFFER_SIZE], int **allRead, int (**hasRead)[READERS], int **writer_pos, int **endOfFile, int **file_pos);
void reset_array(int (***hasRead)[READERS]);
int check_read(int (*hasRead)[READERS]);
void validateArg(int argc, char* argv[]);
void create_shared_memory(int *data_bufferFD,int *hasReadFD,int *allReadFD,int *reader_countFD,int *writer_countFD,int *reader_posFD,int *writer_posFD, int *endOfReadFD, int *endOfFileFD, int *readerPiecesFD, int *semaphoresFD, int *file_posFD, int (**data_bufferPtr)[BUFFER_SIZE],int **allReadPtr,int (**hasReadPtr)[READERS],int **reader_countPtr, int **writer_countPtr, int **reader_posPtr,int **writer_posPtr, int **endOfReadPtr, int **endOfFilePtr, int (**readerPiecesPtr)[READERS], sem_t **semaphoresPtr, int **file_posPtr);
void delete_shared_memory(int data_bufferFD,int hasReadFD,int allReadFD,int reader_countFD,int writer_countFD,int reader_posFD,int writer_posFD, int endOfReadFD, int endOfFileFD, int readerPiecesFD, int semaphoresFD, int file_posFD, int (**data_bufferPtr)[BUFFER_SIZE],int **allReadPtr,int (**hasReadPtr)[READERS],int **reader_countPtr, int **writer_countPtr, int **reader_posPtr,int **writer_posPtr, int **endOfReadPtr, int **endOfFilePtr, int (**readerPiecesPtr)[READERS], sem_t **semaphoresPtr, int **file_posPtr);

int main(int argc, char* argv[])
{
  //File descriptor
  int data_bufferFD, hasReadFD, allReadFD, reader_countFD, writer_countFD, reader_posFD, writer_posFD, endOfReadFD, endOfFileFD, readerPiecesFD, semaphoresFD, file_posFD;
  //Shared memory pointers
  int (*data_bufferPtr)[BUFFER_SIZE], *allReadPtr,(*hasReadPtr)[READERS] = {0}, *reader_countPtr, *writer_countPtr, *reader_posPtr, *writer_posPtr, *endOfReadPtr, *endOfFilePtr, (*readerPiecesPtr)[READERS] = {0}, *file_posPtr;

  //Validates the command line arguements
  //validateArg(argc, argv); -- Working on it
  //Creates the shared memory
  create_shared_memory(&data_bufferFD, &hasReadFD, &allReadFD, &reader_countFD, &writer_countFD, &reader_posFD, &writer_posFD, &endOfReadFD, &endOfFileFD, &readerPiecesFD, &semaphoresFD, &file_posFD, &data_bufferPtr, &allReadPtr, &hasReadPtr, &reader_countPtr, &writer_countPtr, &reader_posPtr, &writer_posPtr, &endOfReadPtr, &endOfFilePtr, &readerPiecesPtr, &semaphoresPtr, &file_posPtr);

  //Initiates all the semaphores
  sem_init(&mutex, 0, 1);
  sem_init(&rf_mutex, 0, 1);
  sem_init(&w_mutex, 0, 1);

  //Initiates values
  int ii, jj;
  *allReadPtr = 0;
  *reader_countPtr = 0;
  *writer_countPtr = 0;
  *endOfReadPtr = 0;
  *endOfFilePtr = 0;
  *file_posPtr = 0;
  semaphoresPtr[0] = mutex;
  semaphoresPtr[1] = rf_mutex;
  semaphoresPtr[2] = w_mutex;

  //Initiates Writers
  pid_t writer_pid = getpid();
  for (ii = 0; ii < WRITERS; ii++)
  {
    if (writer_pid > 0)
    {
      writer_amount++;
      printf("%sCreating Writer %d%s\n", GRN, ii, RESET);
      writer_pid = fork();
    }
  }

  //Initiates Readers
  pid_t reader_pid = getpid();
  for (jj = 0; jj < READERS; jj++)
  {
    if (reader_pid > 0)
    {
      reader_amount++;
      printf("%sCreating Reader %d%s\n", GRN, jj, RESET);
      reader_pid = fork();
    }
  }

  //Checks to make sure its a child
  if (writer_pid == 0 && reader_pid != 0)
  {
    writer(writer_amount,data_bufferPtr,allReadPtr,hasReadPtr,reader_countPtr,writer_countPtr,writer_posPtr,endOfFilePtr,semaphoresPtr, file_posPtr);
  }
  else if (reader_pid == 0 && writer_pid != 0)
  {
    reader(reader_amount,data_bufferPtr,allReadPtr,hasReadPtr,reader_countPtr,writer_countPtr,reader_posPtr,endOfReadPtr,readerPiecesPtr,semaphoresPtr);
  }
  else
  {
    //Else it is terminated
    for (ii = 0; ii < READERS+WRITERS; ii++)
    {
      wait(NULL);
    }
  }

  //destroy semaphores
  sem_destroy(&mutex);
  sem_destroy(&rf_mutex);
  sem_destroy(&w_mutex);
  //Removes shared memory
  delete_shared_memory(data_bufferFD, hasReadFD, allReadFD, reader_countFD, writer_countFD, reader_posFD, writer_posFD, endOfReadFD, endOfFileFD, readerPiecesFD, semaphoresFD, file_posFD, &data_bufferPtr, &allReadPtr, &hasReadPtr, &reader_countPtr, &writer_countPtr, &reader_posPtr, &writer_posPtr, &endOfReadPtr, &endOfFilePtr, &readerPiecesPtr, &semaphoresPtr, &file_posPtr);

  return 0;
}

/**
*  Depending on the read count determines on what happens
*  > If there are > 0 readers then pthread will broadcast (Waking up all
*  sleeping readers)
*  > If there are 0 readers then the writers will be signalled and
*  woken up
*
*  @void - No param inputs
**/
void signal_next(int *reader_count, sem_t **semaphores)
{
  if (*reader_count > 0)
  {
    //Wake up next reader
    signal(&(*semaphores)[0]);
  }
  else
  {
    //Wake up next writer
    signal(&(*semaphores)[2]);
  }
}

/**
* Handles the reading with mutexes, it outputs the info about the
* reader and its current status. At the end it outputs how many
* pieces the reader has read
*
* @pid - PID of the reader
* @data_buffer - Contains the shared memory location
* @allRead - Checker to make sure all the readers have read (true/false)
* @hasRead - Array to check if reader has read data
* @reader_count - amount of readers
* @writer_count - amount of writers
* @reader_pos - reader position in array
* @endOfRead - check if its the end of reading
* @readerPieces - amount the reader has read
* @semaphores - array of the semaphores
**/
void reader(int pid, int (*data_buffer)[BUFFER_SIZE], int *allRead, int (*hasRead)[READERS], int *reader_count, int *writer_count, int *reader_pos, int *endOfRead, int (*readerPieces)[READERS], sem_t *semaphores)
{
  //Until finished reading loop
  while (*endOfRead == 0)
  {
    wait(&semaphores[0]);
    printf("Reader-%d is trying to enter into database\n", pid);
    *reader_count++;
    //if more than one reader and more than 0 writers make the reader wait
    if (*reader_count > 1 || *writer_count > 0)
    {
      printf("Reader-%d is waiting until signalled\n", pid);
      //Make reader wait
      wait(&semaphores[0]);
    }
    signal(&semaphores[0]);

    printf("Reader-%d is reading the database\n", pid);
    //Read the data_buffer
    read_file(pid,&data_buffer,&allRead,&hasRead,&reader_pos,&endOfRead, &readerPieces,&semaphores);

    wait(&semaphores[0]);
    reader_count--;
    printf("Reader-%d is leaving the database\n", pid);
    signal_next(reader_count,&semaphores);
    signal(&semaphores[0]);
    //Sleep for amount of time
    sleep(SLEEP_TIME);
  }

  //Output reader result
  printf("Reader-%d has finished reading %d pieces of data from the data_buffer\n", pid, (*readerPieces)[pid]);
}

/**
* Handles the reading of the file and writing to the shared_data, it
* outputs the info about the writer and its current status.
*
* @pid - PID of the reader
* @data_buffer - Contains the shared memory location
* @allRead - Checker to make sure all the readers have read (true/false)
* @hasRead - Array to check if reader has read data
* @reader_count - amount of readers
* @writer_count - amount of writers
* @writer_pos - writer position in array
* @endOfFile - check if its the end of file
* @file_pos - where the writer is up to reading
* @semaphores - array of the semaphores
**/
void writer(int pid, int (*data_buffer)[BUFFER_SIZE], int *allRead, int (*hasRead)[READERS], int *reader_count, int *writer_count, int *writer_pos, int *endOfFile, sem_t *semaphores, int *file_pos)
{
  //Until end of file loop
  while (*endOfFile == 0)
  {
    printf("Writer-%d is trying to enter into database\n", pid);
    wait(&semaphores[2]);
    //if more then 0 readers and 0 writers make the writer wait
    while (*reader_count > 0 || *writer_count > 0)
    {
      //if more than 0 reader and more than 0 writers make the writer wait
      printf("Writer-%d is waiting until signalled\n", pid);
      wait(&semaphores[2]);
    }
    *writer_count++;

    signal(&semaphores[2]);
    printf("Writer-%d is writing into the database\n", pid);
    write_file(pid,&data_buffer,&allRead,&hasRead,&writer_pos,&endOfFile, &file_pos);

    wait(&(semaphores[2]));
    printf("Writer-%d is leaving the database\n", pid);
    *writer_count--;
    signal_next(reader_count,&semaphores);
    signal(&semaphores[2]);
    //Sleep for amount of time
    sleep(SLEEP_TIME);
  }
}

/**
* Reads the data_buffer and depending on the 3 types of outputs
* it will produce an outcome
* > -2147483648 - means it is the end of the array
* > data == 0 - means that the data_buffer is empty
* else we check to make sure all the readers have read the data_buffer
* if all have read we increment the reader_pos and set the allRead
* condition to 1 (true). It outputs the read status and the data the
* reader has read.
*
* @pid - PID of the reader
* @data_buffer - Contains the shared memory location
* @allRead - Checker to make sure all the readers have read (true/false)
* @hasRead - Array to check if reader has read data
* @reader_pos - reader position in array
* @endOfRead - check if its the end of reading
* @readerPieces - amount the reader has read
* @semaphores - array of the semaphores
**/
void read_file(int pid, int (**data_buffer)[BUFFER_SIZE], int **allRead, int (**hasRead)[READERS], int **reader_pos, int **endOfRead, int (**readerPieces)[READERS], sem_t **semaphores)
{
    wait(&(*semaphores)[1]);
    int data = (**data_buffer)[**reader_pos];
    //End of the array
    if (data == -2147483648)
    {
      **endOfRead = 1;
      printf("> No more data left to read!\n");
    }
    //Array is empty
    else if (data == 0)
    {
      printf("%s> Waiting for more data to be input%s\n", BLU, RESET);
    }
    else  //Valid data
    {
      //Check to make sure all the readers have read the data_buffer
      if (check_read(*hasRead) == 0)
      {
        //Check if given reader has not read the data_buffer
        if ((**hasRead)[pid] != 1)
        {
          //Increment the amount of pieces which the reader has read
          (**readerPieces)[pid]++;
          //Set the hasRead flag to 1 (True)
          (**hasRead)[pid] = 1;
          //Output status
          printf("- %sReader-%d%s | > %d\n", MAG, pid, RESET, data);
        }

        //If all the readers have read the data_buffer then
        if (check_read(*hasRead) != 0)
        {
          //Set all read true (0)
          **allRead = **allRead - 1;
          printf("= All data read!\n");
          //If reader_pos is not 20 increment else reset the reader_pos
          if (**reader_pos != 20)
          {
            **reader_pos++;
          }
          else
          {
            **reader_pos = 0;
          }
          //Reset the values in the array
          reset_array(&hasRead);
        }
      }
    }
    signal(&(*semaphores)[1]);
}

/**
* Reads the file line by line and writes it to the data_buffer,
* once the data_buffer has been filled to 20 it is reset back to 0.
* Once all the values in the file have been read it inputs the value
* -2147483648 at the end of the array to show it is done. Outputs
* the status of the writer.
*
* @pid - PID of the reader
* @data_buffer - Contains the shared memory location
* @allRead - Checker to make sure all the readers have read (true/false)
* @hasRead - Array to check if reader has read data
* @reader_pos - reader position in array
* @endOfRead - check if its the end of reading
* @file_pos - where the writer is up to reading
* @semaphores - array of the semaphores
**/
void write_file(int pid, int (**data_buffer)[BUFFER_SIZE], int **allRead, int (**hasRead)[READERS], int **writer_pos, int **endOfFile, int **file_pos)
{
  FILE *f;
  int i;

  //Open file for reading only
  f = fopen(FILENAME,"r");
  if (f == NULL){
      printf("Error! opening file");
  }

  //Get the file position from last time
  fseek(f, **file_pos, 0);
  if (fscanf(f,"%d", &num) != EOF)
  {
    //Check if all the readers have read the data_buffer
    if (**allRead == 0)
    {
      printf("+ %sWriter%s | > %d\n", CYN, RESET, num);
      (**data_buffer)[**writer_pos] = num;
      //If writer_pos is not 20 increment else reset the writer_pos
      if (**writer_pos != 20)
      {
        **writer_pos++;
      }
      else
      {
        **writer_pos = 0;
      }
      //Set the file_pos for next time
      **file_pos = ftell(f);
      //Set the allRead to false (1) as no readers would have read it
      **allRead = **allRead + 1;
    }
    else  //Not all readers have read the buffer
    {
      printf("= Must wait for all readers to read the data\n");
    }
  }
  else
  {
    //End of file reached
    **endOfFile = 1;
    //Min possible int (Because low chance of ever being used)
    (**data_buffer)[**writer_pos] = -2147483648;
  }

  fclose(f);
}

/**
* Resets the array to where all the values are 0
*
* @hasRead - Array of what readers have read (true/false)
**/
void reset_array(int (***hasRead)[READERS])
{
  for (int ii = 0; ii < READERS; ii++)
  {
    (***hasRead)[ii] = 0;
  }
}

/**
* Checks to make sure all the readers have a value in the array
* location, if it is not == 1 then it returns 0 (false)
*
* @hasRead - Array of what readers have read (true/false)
**/
int check_read(int (*hasRead)[READERS])
{
  int jj;
  for (jj = 0; jj < READERS; jj++)
  {
    if ((*hasRead)[jj] != 1)
    {
      return 0;
    }
  }
}

/**
* Validate the command line parameters to make sure the user
* gives the correct input
*
* @param - argc number of parameters
* @param - argv command line parameters
*/
void validateArg(int argc, char* argv[])
{
    //Make sure correct number of command line parameters
    if (argc != 4)
    {
        printf("Incorrect use: /process READERS WRITERS SLEEP_TIME\n");
        exit(1);
    }

    //Make sure READERS is valid
    if (atoi(argv[1]) < 1)
    {
        printf("All data needs to be read! More than 0 readers is needed\n");
        exit(1);
    }

    //Make sure WRITERS is valid
    if (atoi(argv[2]) < 1)
    {
        printf("Data doesn't make it in itself! More than 0 writers needed!\n");
        exit(1);
    }

    //Make sure SLEEP_TIME is valid
    if (atoi(argv[3]) < 1)
    {
        printf("We all need rest! Sleep time must be above 0\n");
        exit(1);
    }
}

/**
* Initiates and creates the shared memory location
*
* @pidFD - PID of the reader (File descriptor)
* @data_bufferFD - Contains the shared memory location (File descriptor)
* @allReadFD - Checker to make sure all the readers have read (true/false) (File descriptor)
* @hasReadFD - Array to check if reader has read data (File descriptor)
* @reader_countFD - amount of readers (File descriptor)
* @writer_countFD - amount of writers (File descriptor)
* @writer_posFD - writer position in array (File descriptor)
* @endOfFileFD - check if its the end of file (File descriptor)
* @file_posFD - where the writer is up to reading (File descriptor)
* @semaphoresFD - array of the semaphores (File descriptor)
* @pidPtr - A pointer to the PID of the reader (Pointer to memory)
* @data_bufferPtr - Contains the shared memory location (Pointer to memory)
* @allReadPtr - Checker to make sure all the readers have read (true/false) (Pointer to memory)
* @hasReadPtr - Array to check if reader has read data (Pointer to memory)
* @reader_countPtr - amount of readers (Pointer to memory)
* @writer_countPtr - amount of writers (Pointer to memory)
* @writer_posPtr - writer position in array (Pointer to memory)
* @endOfFilePtr - check if its the end of file (Pointer to memory)
* @file_posPtr - where the writer is up to reading (Pointer to memory)
* @semaphoresPtr - array of the semaphores (Pointer to memory)
*/
void create_shared_memory(int *data_bufferFD,int *hasReadFD,int *allReadFD,int *reader_countFD,int *writer_countFD,int *reader_posFD,int *writer_posFD, int *endOfReadFD, int *endOfFileFD, int *readerPiecesFD, int *semaphoresFD, int *file_posFD, int (**data_bufferPtr)[BUFFER_SIZE],int **allReadPtr,int (**hasReadPtr)[READERS],int **reader_countPtr, int **writer_countPtr, int **reader_posPtr,int **writer_posPtr, int **endOfReadPtr, int **endOfFilePtr, int (**readerPiecesPtr)[READERS], sem_t **semaphoresPtr, int **file_posPtr)
{
  //Create shared memory
  *data_bufferFD = shm_open("data_buffer", O_CREAT | O_RDWR, 0666);
  *hasReadFD = shm_open("hasRead", O_CREAT | O_RDWR, 0666);
  *allReadFD = shm_open("allRead", O_CREAT | O_RDWR, 0666);
  *reader_countFD = shm_open("reader_count", O_CREAT | O_RDWR, 0666);
  *writer_countFD = shm_open("writer_count", O_CREAT | O_RDWR, 0666);
  *reader_posFD = shm_open("reader_pos", O_CREAT | O_RDWR, 0666);
  *writer_posFD = shm_open("writer_pos", O_CREAT | O_RDWR, 0666);
  *endOfReadFD = shm_open("endOfRead", O_CREAT | O_RDWR, 0666);
  *endOfFileFD = shm_open("endOfFile", O_CREAT | O_RDWR, 0666);
  *readerPiecesFD = shm_open("readerPieces", O_CREAT | O_RDWR, 0666);
  *semaphoresFD = shm_open("semaphores", O_CREAT | O_RDWR, 0666);
  *file_posFD = shm_open("file_pos", O_CREAT | O_RDWR, 0666);

  //Check to make sure there are no errors
  if (*data_bufferFD == -1)
  {
    fprintf(stderr, "Error creating shared memory blocks\n");
    exit(1);
  }

  //Set the size of the shared memory
  if (ftruncate(*data_bufferFD, sizeof(int)*BUFFER_SIZE))
  {
    fprintf(stderr, "Error setting size of data_buffer\n");
    exit(1);
  }
  ftruncate(*hasReadFD, sizeof(int)*READERS);
  ftruncate(*allReadFD, sizeof(int));
  ftruncate(*reader_countFD, sizeof(int));
  ftruncate(*writer_countFD, sizeof(int));
  ftruncate(*reader_posFD, sizeof(int));
  ftruncate(*writer_posFD, sizeof(int));
  ftruncate(*endOfReadFD, sizeof(int));
  ftruncate(*endOfFileFD, sizeof(int));
  ftruncate(*readerPiecesFD, sizeof(int)*READERS);
  ftruncate(*semaphoresFD, sizeof(sem_t)*3);
  ftruncate(*file_posFD, sizeof(int));

  //Map shared memory to addresses
  *data_bufferPtr = mmap(NULL, sizeof(int)*BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, *data_bufferFD, 0);
  *hasReadPtr = mmap(0, sizeof(int)*READERS, PROT_READ | PROT_WRITE, MAP_SHARED, *hasReadFD, 0);
  *allReadPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *allReadFD, 0);
  *reader_countPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *reader_countFD, 0);
  *writer_countPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *writer_countFD, 0);
  *reader_posPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *reader_posFD, 0);
  *writer_posPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *writer_posFD, 0);
  *endOfReadPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *endOfReadFD, 0);
  *endOfFilePtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *endOfFileFD, 0);
  *readerPiecesPtr = mmap(0, sizeof(int)*READERS, PROT_READ | PROT_WRITE, MAP_SHARED, *readerPiecesFD, 0);
  *semaphoresPtr = mmap(0, sizeof(sem_t)*3, PROT_READ | PROT_WRITE, MAP_SHARED, *semaphoresFD, 0);
  *file_posPtr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, *file_posFD, 0);
}

/**
* Deletes and cleans up the shared memory location
*
* @pidFD - PID of the reader (File descriptor)
* @data_bufferFD - Contains the shared memory location (File descriptor)
* @allReadFD - Checker to make sure all the readers have read (true/false) (File descriptor)
* @hasReadFD - Array to check if reader has read data (File descriptor)
* @reader_countFD - amount of readers (File descriptor)
* @writer_countFD - amount of writers (File descriptor)
* @writer_posFD - writer position in array (File descriptor)
* @endOfFileFD - check if its the end of file (File descriptor)
* @file_posFD - where the writer is up to reading (File descriptor)
* @semaphoresFD - array of the semaphores (File descriptor)
* @pidPtr - A pointer to the PID of the reader (Pointer to memory)
* @data_bufferPtr - Contains the shared memory location (Pointer to memory)
* @allReadPtr - Checker to make sure all the readers have read (true/false) (Pointer to memory)
* @hasReadPtr - Array to check if reader has read data (Pointer to memory)
* @reader_countPtr - amount of readers (Pointer to memory)
* @writer_countPtr - amount of writers (Pointer to memory)
* @writer_posPtr - writer position in array (Pointer to memory)
* @endOfFilePtr - check if its the end of file (Pointer to memory)
* @file_posPtr - where the writer is up to reading (Pointer to memory)
* @semaphoresPtr - array of the semaphores (Pointer to memory)
*/
void delete_shared_memory(int data_bufferFD,int hasReadFD,int allReadFD,int reader_countFD,int writer_countFD,int reader_posFD,int writer_posFD, int endOfReadFD, int endOfFileFD, int readerPiecesFD, int semaphoresFD, int file_posFD, int (**data_bufferPtr)[BUFFER_SIZE],int **allReadPtr,int (**hasReadPtr)[READERS],int **reader_countPtr, int **writer_countPtr, int **reader_posPtr,int **writer_posPtr, int **endOfReadPtr, int **endOfFilePtr, int (**readerPiecesPtr)[READERS], sem_t **semaphoresPtr, int **file_posPtr);
{
  //fclose
  close(data_bufferFD);
  close(hasReadFD);
  close(reader_countFD);
  close(writer_countFD);
  close(reader_posFD);
  close(writer_posFD);
  close(endOfReadFD);
  close(endOfFileFD);
  close(readerPiecesFD);
  close(semaphoresFD);
  close(file_posFD);

  //Unmap
  munmap(data_bufferPtr, sizeof(int)*BUFFER_SIZE);
  munmap(hasReadPtr, sizeof(int)*READERS);
  munmap(allReadPtr, sizeof(int));
  munmap(reader_countPtr, sizeof(int));
  munmap(writer_countPtr, sizeof(int));
  munmap(reader_posPtr, sizeof(int));
  munmap(writer_posPtr, sizeof(int));
  munmap(endOfReadPtr, sizeof(int));
  munmap(endOfFilePtr, sizeof(int));
  munmap(readerPiecesPtr, sizeof(int)*READERS);
  munmap(semaphoresPtr, sizeof(sem_t)*3);
  munmap(file_posPtr, sizeof(int));

  //Unlink
  shm_unlink("data_buffer");
  shm_unlink("hasRead");
  shm_unlink("allRead");
  shm_unlink("reader_count");
  shm_unlink("writer_count");
  shm_unlink("reader_pos");
  shm_unlink("writer_pos");
  shm_unlink("endOfRead");
  shm_unlink("endOfWrite");
  shm_unlink("readerPieces");
  shm_unlink("semaphores");
  shm_unlink("file_pos");
}
