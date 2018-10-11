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

sem_t mutexSems[MAXTHREAD + 1]; //Mutual Exclusion semaphores
sem_t sendSems[MAXTHREAD + 1]; //Producer semaphores
sem_t recvSems[MAXTHREAD + 1]; //Receiver semaphores
pthread_t threads[MAXTHREAD]; //Stores thread ids
struct msg mailboxes[MAXTHREAD + 1]; //Mailboxes

std::vector<std::vector<int>> pastgrid; //Stores past grid, or grid where generation computation starts from
std::vector<std::vector<int>> nextgrid; //Stores next grid, or grid where new generation is stored

int nGens; //Global number of generations, accessable by any threads

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

void printGrid(std::vector<std::vector<int>> * grid, int x, int y){
  int i, j;
  //loops for each element in given grid and prints it out
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
  string line; //Variable to store each line in text file
  int i,j, row = 0, col = 0, maxCol = 0; //Iterative variables
  char c; //Variable to store each each char
  ifstream myfile (file); //Read file into stream
  std::vector<int> rowV; //Stores each row
  if (myfile.is_open())
  {
    while ( getline (myfile,line) )
    {
      //For each line the program will iterate through the characters and treat them accordingly:
      //If blank space, skip. Else if 0 add 0 to row. Else if 1 add 1 to row
      //If it goes over the maximum grid size, exits with error
      //At the end of every line, add row to grid and start a new one
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
          exit(-1);
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
        exit(-1);
      }
      rowV = std::vector<int>();
    }
    myfile.close();
  }
  //If input file had different row lengths, it adds 0s to all rows until they all have the same length
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
  int tWorking = 0; //Number of threads that are actually used (in case of more threads than rows)
  int doPrint = 0; //0 for not printing and 1 for printing
  int doInput = 0; //0 to not stop for input and 1 to stop
  int rangeSize; //Size of range
  int x; //number of rows
  int y; //number of columns
  int i = 0, j; //Iterative variables
  int gen = 0; //Iterative variable for generation
  void *WorkerThread(void *); //Declares worker thread
  struct msg Msg = msg();//Initializes message for mailboxes
  bool done = false; //Variables that control in every generation if overall, the threads are done, have empty rows and or repeated rows
  bool empty = false;
  bool repeat = false;
  //read input
  if (argc < 4){
    perror("bad_input");
    exit(1);
  }
  t = atoi(argv[1]);
  //Checks number of threads and adjusts it if < 1 or > MAXTHREAD
  if (t > MAXTHREAD){
    cout<<"Game has a limit of threads: "<< MAXTHREAD << endl;
    t = MAXTHREAD;
  }
  if (t < 1){
    t = 1;
  }
  text = argv[2];
  nGens = atoi(argv[3]);
  //Checks if specify printing or input pause
  if (argc >= 5 && *argv[4] == 'y'){
    doPrint = 1;
  }
  if (argc >= 6 && *argv[5] == 'y'){
    doInput = 1;
  }
  //Parses input file
  parseInput(text);
  //Gets grid size after parsing
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
  //Gets range of rows to split between threads
  rangeSize = x / t;
  if (x % t > 0){ //If split is not exact, increases range size by one
    rangeSize++;
  }
  i = 0;
  j = 1;
  Msg.iSender = 0;
  Msg.type = RANGE;
  //Initialize operation by sending all messages
  while (i < x){
    Msg.value1 = i;
    i += rangeSize;
    if (i > x){ //If this message would have range over the limit, adjust it
      i = x;
    }
    Msg.value2 = i;
    SendMsg(j++, Msg);
    tWorking++; //Increases and computes number of threads actually working
  }
  //Wait for all threads to respond and perform generations
  while(!done && !empty && !repeat){ //Plays for each generation until they are done, all rows repeat or all rows are empty
    j = 0;
    //Assumes the game is done, rows repeat and are empty, until each of these conditions are proven otherwise
    done = true;
    repeat = true;
    empty = true;
    //Swaps past and next grid to have alternating effect
    pastgrid.swap(nextgrid);
    if (doPrint || gen == 0){ //prints if pause or first generation
      cout<<"\n Generation "<<gen<<": \n"<<endl;
      printGrid(&pastgrid, x, y);
    }
    //Input checkpoint
    if (doInput){
      getchar();
    }
    //Send go to all working threads
    while (j < tWorking){
      Msg = msg();
      Msg.type = GO;
      SendMsg(++j,Msg);
    }
    j = 0;
    while (j < tWorking){ //Waits for message from all working threads
      Msg = msg();
      RecvMsg(0,Msg);
      switch (Msg.type){
        case GENDONE: //If message was GENDONE, the game is not done, rows dont repeat and are not empty
          done = false;
          empty = false;
          repeat = false;
          break;
        case EMPTY: //If message was EMPTY, game is not done
          done = false;
          if (Msg.value1 != 1){ //If rows repeated as well, that information comes in the value1 field, so that is checked
            repeat = false;
          }
          break;
        case REPEAT: //If message was REPEAT, rows are not empty and game is not done
          done = false;
          empty = false;
          break;
        case ALLDONE: //Game is done, loop should end
          break;
        default:
          clean(t); //If other message, cleans it all up and exits with error
          perror("bad_message");
          exit(1);
      }
      j++;
      //cout<<"Receiving final message from "<< Msg.iSender<<endl;
      
    }
    gen++;
  }
  //Send stop to all threads (If threads already exited, it does not matter because their mailboxes are empty)
  //This is for non working threads and if the game ended before the limit of generations
  j = 0;
  while (j < t){
    Msg = msg();
    Msg.type = STOP;
    SendMsg(++j,Msg);
  }
  //prints final generation and cleans it up
  cout<<"\n The game ended after "<<gen<<" generations with: \n"<<endl;
  printGrid(&nextgrid, x, y);
  //Clean up
  clean(t);
  return 0;
}

void *WorkerThread(void * arg){
  int thisId = *((int*)(&arg)); //Stores id of this thread
  struct msg message = msg(); //Initializes receive and send messages
  struct msg send = msg();
  int x, y, xi, yi, count = 0, start, end, gen, xboundary, yboundary; //Interative and boundary variables
  bool empty, repeat; //Stores empty and repeat so far conditions
  RecvMsg(thisId, message); //Waits for range message
  if (message.iSender != 0 || message.type != RANGE){
    if (message.type == STOP){ //If stop message exits without error
      pthread_exit((void *)0);
    }
    perror("message_error"); //error otherwise
    pthread_exit((void *)1);
  }
  start = message.value1; //Stores row range
  end = message.value2;
  xboundary = pastgrid.size(); //Stores grid boundaries
  yboundary = pastgrid[0].size();
  for (gen = 1; gen <= nGens; gen++) { //Loops through each generation
    //Receive GO message from thread 0
    empty = true; //Rows assumed to be empty and repeat until proven otherwise
    repeat = true;
    message = msg(); //Receives message from thread 0, expecting it to be GO
    RecvMsg(thisId, message);
    if (message.iSender != 0){
      perror("message_error");
      pthread_exit((void *)1);
    }
    if (message.type != GO){
      pthread_exit((void *)0);
    }
    //play a generation, looping for each element in this threads range
    for (x = start; x < end; x++){
      for (y = 0; y < yboundary; y++){
        count = 0;//Counts all the neighboring elements = 1
        for (xi = x - 1; xi <= x + 1; xi++){
          for (yi = y - 1; yi <= y + 1; yi++){
            if (!(xi < 0 || xi >= xboundary || yi < 0 || yi >= yboundary) && pastgrid[xi][yi] && !(xi == x && yi == y)){
              //Checked if xi and yi are out of bounds or the same as x y and if they = 1 otherwise
              count++;
            }
          }
        }
        //Assigns next generation element in same x y coord based on count around neighbors
        if ((count == 2 && pastgrid[x][y] == 1) || count == 3){
          nextgrid[x][y] = 1;
          empty = false; //Sets empty to false if any elements are added on next grid
        }else{
          nextgrid[x][y] = 0;
        }
        if (nextgrid[x][y] != pastgrid[x][y]){
          repeat = false; //Sets repeat to false if element does not repeat
        }
      }
    }
    //Check for cases and sends appropriate message
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
    //if not last generation, sends message
    if (gen != nGens){
      SendMsg(0,send);
    }
  }
  //Sends ALLDONE if game is done
  send = {thisId, ALLDONE, 0, 0};
  SendMsg(0,send);
}