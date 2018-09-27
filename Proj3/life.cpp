/*
Pedro de vasconcellos Oporto
pdevasconcelloso@wpi.edu
CS3013 - Prof. Wills
Project 3 - Part 2
Due 09/28
*/

#include <iostream>
using namespace std;
#include <pthread.h>
#include <semaphore.h>
#include <vector>
#include <fstream>

#define RANGE 1 //Range of rows
#define ALLDONE 2 //Thread done with all generations
#define GO 3 //To start next thread gen
#define GENDONE 4 // Generation Done
#define EMPTY 5 //Specified rows are empty (could also be a repeat if value1 is 1)
#define REPEAT 6 //Specified rows repeated from past generation
#define STOP 7 //To stop thread from executing
#define MAXGRID 40 //Maximum grid size
#define MAXTHREAD 10 //Maximum number of worker threads

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

std::vector<std::vector<int>> pastgrid;
std::vector<std::vector<int>> nextgrid;

int nGens;

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

void SendMsg(int iTo, struct msg &Msg){
  sem_wait(&sendSems[iTo]);
  sem_wait(&mutexSems[iTo]);
  //cout<<"Sending message: " << Msg.type<<" from "<< Msg.iSender << " to " << iTo<<endl;
  mailboxes[iTo] = Msg;
  sem_post(&recvSems[iTo]);
  sem_post(&mutexSems[iTo]);
}

void RecvMsg(int iFrom, struct msg &Msg){
  sem_wait(&recvSems[iFrom]);
  sem_wait(&mutexSems[iFrom]);
  Msg = mailboxes[iFrom];
  //cout<<"Receiving message: " << Msg.type<<" from "<< Msg.iSender << " to " << iFrom<<endl;
  sem_post(&sendSems[iFrom]);
  sem_post(&mutexSems[iFrom]);
}

void printGrid(std::vector<std::vector<int>> * grid, int x, int y){
  int i, j;
  for (i = 0; i < x; i++){
    cout<<'\t';
    for (j = 0; j < y; j ++){
      cout<<' '<<(*grid)[i][j];
    }
    cout<<endl;
  }
  cout<<"\n"<<endl;
}

void parseInput(string file){
  string line;
  int i;
  int j;
  char c;
  ifstream myfile (file);
  int row = 0;
  int col = 0;
  int maxCol = 0;
  std::vector<int> rowV;
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
      for(std::string::iterator it = line.begin(); it != line.end(); ++it) {
        c = *it;
        if (c == '0'){
          rowV.push_back(0);
          col++;
        } else if( c== '1'){
          rowV.push_back(1);
          col++;
        }
        if (col > MAXGRID){
          perror("Past_MaxGrid");
          exit(1);
        }
      }
      if (col > maxCol){
        maxCol = col;
      }
      pastgrid.push_back(rowV);
      nextgrid.push_back(rowV);
      col = 0;
      if (row++ > MAXGRID){
        perror("Past_MaxGrid");
        exit(1);
      }
      rowV = std::vector<int>();
    }
    myfile.close();
  }
  for (i = 0; i < row; i++){
    j = pastgrid[i].size();
    while (j++ != maxCol){
      pastgrid[i].push_back(0);
    }
  }
}

int main(int argc, char *argv[])
 /* argc -- number of arguments */
 /* argv -- an array of strings */
{
  string text; //initial text file
  int t; //Number of threads to create
  int tWorking = 0;
  int doPrint = 0;
  int doInput = 0;
  int rangeSize; //Size of range
  int x;
  int y;
  int i = 0;
  int j;
  int gen = 0;
  void *WorkerThread(void *);
  struct msg Msg = msg();
  bool done = false;
  bool empty = false;
  bool repeat = false;
  //read input
  if (argc < 4){
    perror("bad_input");
    exit(1);
  }
  t = atoi(argv[1]);
  text = argv[2];
  nGens = atoi(argv[3]);
  if (argc >= 5 && *argv[4] == 'y'){
    doPrint = 1;
  }
  if (argc >= 6 && *argv[5] == 'y'){
    doInput = 1;
  }
  
  parseInput(text);
  x = pastgrid.size();
  y = pastgrid[0].size();
  
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
      if (pthread_create(&threads[i], NULL, WorkerThread, (void *)i) != 0) {
        perror("pthread_create");
        exit(1);
      }
    }
  }
  //cout<<"Threads and semaphores created and waiting"<<endl;
  //Initialize operation by sending all messages
  rangeSize = x / t;
  if (x % t > 0){
    rangeSize++;
  }
  i = 0;
  j = 1;
  Msg.iSender = 0;
  Msg.type = RANGE;
  
  while (i < x){
    Msg.value1 = i;
    i += rangeSize;
    if (i > x){
      i = x;
    }
    Msg.value2 = i;
    SendMsg(j++, Msg);
    tWorking++;
  }
  //Wait for all threads to respond and perform generations
  while(!done && !empty && !repeat){
    j = 0;
    done = true;
    repeat = true;
    empty = true;
    pastgrid.swap(nextgrid);
    if (doPrint || gen == 0){
      cout<<"\n Generation "<<gen<<": \n"<<endl;
      printGrid(&pastgrid, x, y);
    }
    //Input checkpoint
    if (doInput){
      getchar();
    }
    //Send go
    while (j < tWorking){
      Msg = msg();
      Msg.type = GO;
      SendMsg(++j,Msg);
    }
    j = 0;
    while (j < tWorking){
      Msg = msg();
      RecvMsg(0,Msg);
      switch (Msg.type){
        case GENDONE:
          done = false;
          empty = false;
          repeat = false;
          break;
        case EMPTY:
          done = false;
          if (Msg.value1 != 1){
            repeat = false;
          }
          break;
        case REPEAT:
          done = false;
          empty = false;
          break;
        case ALLDONE:
          break;
        default:
          clean(t);
          perror("bad_message");
          exit(1);
      }
      j++;
      //cout<<"Receiving final message from "<< Msg.iSender<<endl;
      
    }
    gen++;
  }
  //Send stop to all threads
  j = 0;
  while (j < t){
    Msg = msg();
    Msg.type = STOP;
    SendMsg(++j,Msg);
  }
  //print next
  cout<<"\n The game ended after "<<gen<<" generations with: \n"<<endl;
  printGrid(&nextgrid, x, y);
  //Clean up
  clean(t);
  return 0;
}

void *WorkerThread(void * arg){
  int thisId = *((int*)(&arg));
  struct msg message = msg();
  struct msg send = msg();
  int x, y, xi, yi, count = 0, start, end, gen, xboundary, yboundary;
  bool empty, repeat;
  //cout<<"Thread started "<<thisId<<endl;
  RecvMsg(thisId, message);
  if (message.iSender != 0 || message.type != RANGE){
    if (message.type == STOP){
      pthread_exit((void *)0);
    }
    perror("message_error");
    pthread_exit((void *)1);
  }
  start = message.value1;
  end = message.value2;
  xboundary = pastgrid.size();
  yboundary = pastgrid[0].size();
  for (gen = 1; gen <= nGens; gen++) {
    //Receive GO message from thread 0
    empty = true;
    repeat = true;
    message = msg();
    RecvMsg(thisId, message);
    if (message.iSender != 0){
      perror("message_error");
      pthread_exit((void *)1);
    }
    if (message.type != GO){
      pthread_exit((void *)0);
    }
    //cout<<start<<end<<yboundary<<endl;
    //play a generation
    for (x = start; x < end; x++){
      for (y = 0; y < yboundary; y++){
        count = 0;
        for (xi = x - 1; xi <= x + 1; xi++){
          for (yi = y - 1; yi <= y + 1; yi++){
            if (!(xi < 0 || xi >= xboundary || yi < 0 || yi >= yboundary) && pastgrid[xi][yi] && !(xi == x && yi == y)){
              count++;
            }
          }
        }
        if ((count == 2 && pastgrid[x][y] == 1) || count == 3){
          nextgrid[x][y] = 1;
          empty = false;
        }else{
          nextgrid[x][y] = 0;
        }
        if (nextgrid[x][y] != pastgrid[x][y]){
          repeat = false;
        }
      }
    }
    //Check for cases
    send = msg();
    send.iSender = thisId;
    if(empty){
      send.type = EMPTY;
      if(repeat){
        send.value1 = 1;
      }
    }
    else if (repeat){
      send.type = REPEAT;
    }
    else{
      send.type = GENDONE;
    }
    //Send appropriate message to thread 0.
    if (gen != nGens){
      SendMsg(0,send);
    }
  }
  send = {thisId, ALLDONE, 0, 0};
  SendMsg(0,send);
}