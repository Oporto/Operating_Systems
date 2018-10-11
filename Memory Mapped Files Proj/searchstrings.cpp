// CS 3013 by Prof. Craig Wills
// Pedro de Vasconcellos Oporto (pdevasconcelloso)
// A term 2018
// Main program for the searchstrings18 project

#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXTHREAD 16
#define RANGE 1 //Range of of buffer
#define SEARCH 2
#define ALLDONE 3 //Thread done with file size

class Mmapper{
  //Mmapper class to handle mapping and removing file from memory
  public:
  
  int fd;
  struct stat sb;
  char *file;
  
  ~Mmapper(){
    if(munmap(this->file, this->sb.st_size) < 0){
    	perror("Could not unmap memory");
    	exit(1);
    }

    close(fd);
  }
  char *mapFile(char *file){
    char * pchFile;
    /* Map the input file into memory */
    if ((this->fd = open (file, O_RDONLY)) < 0) {
    	perror("Could not open file");
    	exit (1);
    }
  
    if(fstat(this->fd, &this->sb) < 0){
  	  perror("Could not stat file to obtain its size");
  	  exit(1);
    }
  
    if ((pchFile = (char *) mmap (NULL, this->sb.st_size, PROT_READ, MAP_SHARED, this->fd, 0)) == (char *) -1)	{
  	  perror("Could not mmap file");
  	  exit (1);
    }
    this->file = pchFile;
    return pchFile;
  }
  
};


struct msg {
  int iSender; /* sender of the message (0 .. number-of-threads) */
  int type; /* its type */
  char* string; /* string */
  int value; /* value */
  int value2;
  int value3;
};

sem_t mutexSems[MAXTHREAD + 1]; //Mutual Exclusion semaphores
sem_t sendSems[MAXTHREAD + 1]; //Producer semaphores
sem_t recvSems[MAXTHREAD + 1]; //Receiver semaphores
pthread_t threads[MAXTHREAD]; //Stores thread ids
struct msg mailboxes[MAXTHREAD + 1]; //Mailboxes

void clean(int t){
  int j = 0;
  while (j <= t){
    if (j != 0){
      (void)pthread_join(threads[j - 1], NULL); //joins all threads
    }
    //Destroy semaphores
    (void)sem_destroy(&mutexSems[j]);
    (void)sem_destroy(&sendSems[j]);
    (void)sem_destroy(&recvSems[j]);
    j++;
  }
}

void SendMsg(int iTo, struct msg &Msg){
  sem_wait(&sendSems[iTo]); //Waits for producer semaphore
  sem_wait(&mutexSems[iTo]); //Waits for mutual exclusion semaphore
  
  mailboxes[iTo] = Msg; //Inserts message in mailbox
  sem_post(&recvSems[iTo]); //Signals consumer semaphore
  sem_post(&mutexSems[iTo]); //Signals/releases mutual exclusion semaphore
}

void RecvMsg(int iFrom, struct msg &Msg){
  sem_wait(&recvSems[iFrom]); //Waits for consumer semaphore
  sem_wait(&mutexSems[iFrom]); //Waits for mutual exclusion semaphore
  
  Msg = mailboxes[iFrom];
  sem_post(&sendSems[iFrom]); //Signals producer semaphore
  sem_post(&mutexSems[iFrom]); //Signals/releases mutual exclusion semaphore
}

int *searchRange(char* start, int rangeSize, char* searchString, int searchSize){
  int* returnInts = new int[3]; //If the searchstring is split in the middle at the start and/or end of the range:
  //Position of the searchstring split at start(returnInts[1]) and/or end(returnInts[2])
  //returnInts[0] is count of occurrances of searchString
  bool match = true; //Keeps track in a loop if the match is true
  int i, j; //Loop counter
  returnInts[0] = 0;
  returnInts[1] = -1;
  returnInts[2] = -1;
  
  //Start: Check for substring of searchString at start
  for (i = 1; i < searchSize; i++){ //Loop through possible starts (second, third, ... positions in the search string)
    match = true;
    for (j = 0; j < searchSize - i; j++){ //Check for substring match
      //i is offset on searchString and j is current position being checked in that substring and range start
      if (start[j] != searchString[j+i]){
        match = false; //records match as
        break; //Gets out of loop
      }
    }
    if(match){ //If matched, record the substring start that is that the start of the range
      returnInts[1] = i;
      break;
    }
  }
  //End: Check for substring of searchString at start
  
  //Start: Check for searchString match in range
  for (i = 0; i < rangeSize; i++){
    match = true;
    for (j = 0; j < searchSize; j++){
      //at this point, i is the start being checked and j is the offset on the searchString
      if (j + i >= rangeSize){
        returnInts[2] = j - 1;
        return returnInts;
      }
      if (start[i + j] != searchString[j]){
        match = false;
        break;
      }
    }
    if(match){ //If start[i] was the start of the searchStirng (match)
      returnInts[0]++; //Increase searchstring count
      i += j; //Move cursor to end of searchString (so that next search starts in the char after)
    } //If searchString was not found, start next search normaly at next index (will catch 'world' in 'hellowoworld')
  }
  //End: Check for searchString match in range
  
  return returnInts;
}

int searchOnRead(char* fileName, int bufSize, char* searchString, int searchSize){
  char buf[bufSize];
  int fdIn, cnt, i,fileSize = 0, count = 0, pastEnd = -1;
  int *rangeResult;

  if ((fdIn = open(fileName, O_RDONLY)) < 0) {
	  cerr << "file open\n";
	  exit(1);
  }
  while ((cnt = read(fdIn, buf, bufSize)) > 0) {
    
    rangeResult = searchRange(buf, cnt, searchString, searchSize);
    count += rangeResult[0];
    if (pastEnd +1 == rangeResult[1] && pastEnd >-1){
      count++;
    }
    pastEnd = rangeResult[2];
    fileSize += cnt;
  }
  
  
  if (fdIn > 0)
    close(fdIn);
    
  cout<<"File size: "<<fileSize<<" bytes."<<endl;
  return count;
}
  

int main(int argc, char *argv[]){
  
  char* fileName;//Name of file
  char* filePointer; //pointer to mapped file
  char* searchString; //String searched for
  int searchStringSize; //Search string size
  int bufSize = 1024; //Size of buffer for normal read operation
  int nThreads = 1; //Number of threads for program
  int count = 0;
  int i, j; //Loop counters
  int mmapResults[MAXTHREAD][2];
  bool mmap = false;
  msg Msg = msg();
  Mmapper mmapper; //Initialized mmapper object
  
  void *MmapThread(void * arg);
  
  int fileSize, rangeSize;
  
  //Start: Manage program input
  if(argc < 3) {
  	cerr << "Wrong input: Needs at least file name and search string\n";
  	exit(1);
  }
  fileName = argv[1];
  searchString = argv[2];
  searchStringSize = (int)strlen(argv[2]);
  //Managing third argument
  if (argc == 4){
    if (strcmp(argv[3],"mmap") == 0){
      mmap = true;
    }
    else if (argv[3][0] == 'p'){
      nThreads = atoi(&argv[3][1]);
      if (nThreads > MAXTHREAD){
        nThreads = MAXTHREAD;
        cout << "Adjusting number of threads to " <<MAXTHREAD << endl;
      }else if (nThreads <= 0) {
        nThreads = 1;
        cout << "Adjusting number of threads to 1" << endl;
      }
      mmap = true;
    }else{
      bufSize = atoi(argv[3]);
    }
  }
  //End: Manage program input
  
  //Start: Read using normal system call
  if (!mmap){
    count = searchOnRead(fileName, bufSize, searchString, searchStringSize);
    //Print something
    cout<<"Occurrences of the string \""<<searchString<<"\": "<<count<<endl;
    exit(0);
  }
  //End: Read using normal system call
  //Start: Initialize threads and mailboxes
  for (i = 0; i <= nThreads; i++){
    if (sem_init(&mutexSems[i],1,1) < 0){
      perror("sem_init");
      exit(1);
    }
    if (sem_init(&sendSems[i],1,1) < 0){
      perror("sem_init");
      exit(1);
    }
    if (sem_init(&recvSems[i],1,0) < 0){
      perror("sem_init");
      exit(1);
    }
    if (i > 0){
      if (pthread_create(&threads[i], NULL, MmapThread, (void *)i) != 0) {
        perror("pthread_create");
        exit(1);
      }
    }
  }
  //End: Initialize threads and mailboxes
  //Start: Send messages to start threads
  mmapper = Mmapper();
  filePointer = mmapper.mapFile(fileName);
  fileSize = mmapper.sb.st_size;
  cout<<"File size: "<<fileSize<<" bytes."<<endl;
  //Initialize operation by sending all messages
  rangeSize = fileSize / nThreads; //Range size
  i = 1;
  j = 1;
  Msg = msg();
  Msg.iSender = 0;
  Msg.type = RANGE;
  Msg.string = filePointer;
  while (i < fileSize + 1){ //Sends a message to each thread while increasing the loop
    Msg.value = i;
    i += rangeSize;
    if (i + rangeSize > fileSize + 1){ //If there are remainders, include in range
      i = fileSize + 1;
    }
    Msg.value2 = i;
    SendMsg(j++, Msg);
  }
  
  Msg = msg();
  Msg.iSender = 0;
  Msg.type = SEARCH;
  Msg.string = searchString;
  Msg.value = searchStringSize;
  for (j = 1; j <= nThreads; j ++){
    SendMsg(j, Msg);
  }
  //End: Send messages to start threads
  
  //Start: Fecth messages from threads
  j = 0;
  while (j < nThreads){
    Msg = msg();
    RecvMsg(0, Msg);
    if (Msg.type == ALLDONE){ //Makes sure the received message is correct
      count += Msg.value; //Adds return value to overall count
      mmapResults[j][0] = Msg.value2;
      mmapResults[j][1] = Msg.value3;
    }else{
      perror("Message_error");
      exit(1);
    }
    j++;
  }
  //End: Fecth messages from threads
  
  //Start: Process results
  i = -1;
  for (j = 0; j < nThreads; j++){
    if (i+1 == mmapResults[j][0] && i>-1){
      
      count++;
    }
    i = mmapResults[j][1];
  }
  //End: Process results
  
  cout<<"Occurrences of the string \""<<searchString<<"\": "<<count<<endl;
  clean(nThreads);
  exit(0);
}

void *MmapThread(void * arg){
  int thisId = *((int*)(&arg)); //Stores id of this thread
  struct msg message = msg(); //Initializes receive message
  struct msg send = msg(); //Initializes send message (could have used only one)
  int searchSize, rangeSize;
  int *rangeResults;
  char *rangeStart;
  char *searchString;
  RecvMsg(thisId, message); //Waits for RANGE message
  if (message.iSender != 0 || message.type != RANGE || !message.value || !message.string){ //Checks if message is what was expected
    perror("message_error");
    exit(1);
  }
  rangeSize = message.value2-message.value;
  rangeStart = &message.string[message.value - 1];
  message = msg();
  RecvMsg(thisId, message); //Waits for SEARCH message
  if (message.iSender != 0 || message.type != SEARCH || !message.value || !message.string){ //Checks if message is what was expected
    perror("message_error");
    exit(1);
  }
  searchSize = message.value;
  searchString = message.string;
  rangeResults = searchRange(rangeStart, rangeSize, searchString, searchSize);
  send.iSender = thisId;
  send.type = ALLDONE;
  send.value = rangeResults[0];
  send.value2 = rangeResults[1];
  send.value3 = rangeResults[2];
  SendMsg(0,send);
}