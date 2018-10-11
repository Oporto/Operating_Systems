/*
Pedro de vasconcellos Oporto
pdevasconcelloso@wpi.edu
CS3013 - Prof. Wills
Project 3 - Part 1
Due 09/28
*/

#include <iostream>
using namespace std;
#include <pthread.h>
#include <semaphore.h>

#define RANGE 1
#define ALLDONE 2
#define MAXTHREAD 10

struct msg {
int iSender; /* sender of the message (0 .. number-of-threads) */
int type; /* its type */
int value1; /* first value */
int value2; /* second value */
};

sem_t mutexSems[MAXTHREAD + 1]; //Mutual Exclusion semaphores
sem_t sendSems[MAXTHREAD + 1]; //Producer semaphores
sem_t recvSems[MAXTHREAD + 1]; //Consumer semaphores
pthread_t threads[MAXTHREAD]; //Stores thread id
struct msg mailboxes[MAXTHREAD + 1]; //Mailboxes

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

void clean(int t){
  int j = 0;
  while (j <= t){
    if (j != 0){
      (void)pthread_join(threads[j - 1], NULL); //Joins all threads
    }
    //Destroy all semaphores
    (void)sem_destroy(&mutexSems[j]);
    (void)sem_destroy(&sendSems[j]);
    (void)sem_destroy(&recvSems[j]);
    j++;
  }
}

int main(int argc, char *argv[])
 /* argc -- number of arguments */
 /* argv -- an array of strings */
{
  int n; //range to add
  int t; //Number of threads to create
  int rangeSize; //Size of range
  int finalSum = 0; //Overall final sum
  int i = 0, j; //Loop iterators
  void *SumThread(void *); //Declaration of thread function
  struct msg Msg = msg(); //Initializes message struct
  //read input
  if (argc != 3){
    perror("bad_input");
    exit(1);
  }
  //Gets number of threads and checks if it is between 1 and MAXTHREAD, otherwise it is adjusted
  t = atoi(argv[1]);
  if (t > MAXTHREAD){
    cout<<"Addem has a limit of threads: "<< MAXTHREAD << endl;
    t = MAXTHREAD;
  }
  if (t < 1){
    t = 1;
  }
  n = atoi(argv[2]);
  //Intialize semaphores for mailboxes and threads
  for (i; i <= t; i++){
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
      if (pthread_create(&threads[i], NULL, SumThread, (void *)i) != 0) {
        perror("pthread_create");
        exit(1);
      }
    }
  }
  //Initialize operation by sending all messages
  rangeSize = n / t; //Range size
  i = 1;
  j = 1;
  Msg.iSender = 0;
  Msg.type = RANGE;
  while (i < n + 1){ //Sends a message to each thread while increasing the loop
    Msg.value1 = i;
    i += rangeSize;
    if (i + rangeSize > n + 1){ //If there are remainders, include in range
      i = n + 1;
    }
    Msg.value2 = i;
    SendMsg(j++, Msg);
  }
  //Wait for all threads to respond and perform final sum
  j = 0;
  while (j < t){
    Msg = msg();
    RecvMsg(0,Msg);
    if (Msg.type == ALLDONE){ //Makes sure the received message is correct
      finalSum += Msg.value1; //Adds return vlue to overall sum
    }else{
      perror("Message_error");
      exit(1);
    }
    j++;
  }
  cout << "The total for 1 to " << n << " using " << t << " threads is " << finalSum << "." << endl;
  
  //Clean up
  clean(t);
  return 0;
}

void *SumThread(void * arg){
  int thisId = *((int*)(&arg)); //Stores id of this thread
  struct msg message = msg(); //Initializes receive message
  struct msg send = msg(); //Initializes send message (could have used only one)
  int thisSum = 0; //This thread's sum
  int i; //Iterative variable
  RecvMsg(thisId, message); //Waits for RANGE message
  if (message.iSender != 0 || message.type != RANGE || !message.value1 || !message.value2){ //Checks if message is what was expected
    perror("message_error");
    exit(1);
  }
  for (i = message.value1; i < message.value2; i++){ //Sums all values in range
    thisSum += i;
  }
  send = {thisId, ALLDONE, thisSum, 0}; //Creates and sends ALLDONE message
  SendMsg(0,send);
}