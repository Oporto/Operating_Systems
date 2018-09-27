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

sem_t mutexSems[MAXTHREAD + 1];
sem_t sendSems[MAXTHREAD + 1];
sem_t recvSems[MAXTHREAD + 1];
pthread_t threads[MAXTHREAD];
struct msg mailboxes[MAXTHREAD + 1];

void SendMsg(int iTo, struct msg &Msg){
  sem_wait(&sendSems[iTo]);
  sem_wait(&mutexSems[iTo]);
  if (iTo == 0){
    //cout<<"Sending message: " << Msg.value1<<" from "<< Msg.iSender << " to " << iTo<<endl;
  }
  
  mailboxes[iTo] = Msg;
  sem_post(&recvSems[iTo]);
  sem_post(&mutexSems[iTo]);
}

void RecvMsg(int iFrom, struct msg &Msg){
  sem_wait(&recvSems[iFrom]);
  sem_wait(&mutexSems[iFrom]);
  
  Msg = mailboxes[iFrom];
  //cout<<"Receiving message from "<< Msg.iSender << " to " << iFrom<<endl;
  sem_post(&sendSems[iFrom]);
  sem_post(&mutexSems[iFrom]);
}

void clean(int t){
  int j = 0;
  while (j <= t){
    if (j != 0){
      (void)pthread_join(threads[j - 1], NULL);
    }
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
  int finalSum = 0;
  int i = 0;
  int j;
  void *SumThread(void *);
  struct msg Msg = msg();
  //read input
  if (argc != 3){
    perror("bad_input");
    exit(1);
  }
  t = atoi(argv[1]);
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
  //cout<<"Threads and semaphores created and waiting"<<endl;
  //Initialize operation by sending all messages
  rangeSize = n / t;
  i = 1;
  j = 1;
  Msg.iSender = 0;
  Msg.type = RANGE;
  while (i < n + 1){
    Msg.value1 = i;
    i += rangeSize;
    if (i + rangeSize > n + 1){
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
    if (Msg.type == ALLDONE){
      finalSum += Msg.value1;
    }
    j++;
    //cout<<"Receiving final message from "<< Msg.iSender<<endl;
  }
  cout << "The total for 1 to " << n << " using " << t << " threads is " << finalSum << "." << endl;
  
  //Clean up
  clean(t);
  return 0;
}

void *SumThread(void * arg){
  int thisId = *((int*)(&arg));
  struct msg message = msg();
  struct msg send = msg();
  int thisSum = 0;
  int i;
  //cout<<"Thread started "<<thisId<<endl;
  RecvMsg(thisId, message);
  if (message.iSender != 0 || message.type != RANGE || !message.value1 || !message.value2){
    perror("message_error");
    exit(1);
  }
  for (i = message.value1; i < message.value2; i++){
    thisSum += i;
  }
  send = {thisId, ALLDONE, thisSum, 0};
  SendMsg(0,send);
}