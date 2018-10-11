#include <iostream>
using namespace std;
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string>
#include <cstring>
#include <stack>

struct process{ //This struct records the id and the process name for a process, with the name being the command that creates such process
  int pid;
  char* processName;
};
/**
 * This function loops through all the arguments given through the shell to check if it contains the
 * & character, meaning the process should be run in the background
 * @arg arguments, list of command and arguments given to shell
 * @return boolean indicating if task should be run on the background
*/
int isBackground(char* arguments[]){
  int i = 0; //Iterates through array
  char* arg = arguments[0];//Current argument
  while(arg != '\0'){ //Loops through the argument array
    if (strcmp(arg, "&") == 0){ //Checks if argument is the & character
      arguments[i] = '\0';
      return i; //returns the position of the & character
    }
    arg = arguments[++i];
  }
  return -1;
}
/**
 * This function simply checks if a process is still running
 * @arg pid, process id
 * @return boolean indicating if process is still running
*/
bool isProcessRunning(int pid){
  /*The waitpid function with the WNOHANG option will return the status of the process based on process id.
  If the process is done, waitpid wont return 0.
  */
  if (waitpid(pid,NULL,WNOHANG) == 0){
    return true;
  }
  else{
    return false;
  }
}
/**
 * This function loops through all the processes in the background process stack by temporarely adding it to another stack, inverting the order. When it does so, it removes all tasks that are not running anymore. Then it moves all tasks to original stack, while outputting them to the terminal in the order that they started in the first place.
 * @arg backgroundStack, stack of background tasks/processes that could still be running
 * @return 0 when the opperation succeeds
*/
int listJobs(stack<process>& backgroundStack){
  stack<process> tempStack; //Stack to temporarely store process structs when backgroundStack is being read
  process tempProcess; //Process being looked at
  int i = 1; //Iterating variable to indicate task number
  //Empty main stack and for all tasks still in progress, and add to temporary stack
  while(!backgroundStack.empty()){
    tempProcess = backgroundStack.top();
    backgroundStack.pop();
    if (isProcessRunning(tempProcess.pid)){
      tempStack.push(tempProcess);
    }
  }
  //Add processes not completed back to main stack and print them to the console
  while(!tempStack.empty()){
    tempProcess = tempStack.top();
    cout << '[' << i++ << "] " << tempProcess.pid << " " << tempProcess.processName << endl;
    backgroundStack.push(tempProcess);
    tempStack.pop();
  }
  return 0;
}
/**
 * This function parses the input for the shell program, separating command and arguments into an array,
 * using spaces and new lines as delimiters
 * @arg inputString, shell input in one unparsed string
 * @return array of strings with command and arguments parsed
*/
char** parseShell(char inputString[]){
  
  int i = 0; //index of input string
  char* token; //token from strtok
  char** arguments = new char*[32];
  
  token = strtok(inputString," ");
  arguments[i++] = token;
  while(token!=NULL){
      token = strtok(NULL," ");
      arguments[i++] = token;
  }
  arguments[i] = '\0';
  return arguments;
  
}
/**
 * This function prints out the statistics given the variables that stored them
 * @arg startClock, clock when a process started
 * @arg endClock, clock when a process finish
 * @arg usageS, usage metrics gathered when the process started
 * @arg usageP, usage metrics gathered when the process finished
*/
void printStats(long startClock, long endClock, struct rusage usageS, struct rusage usageP){
  long wallClock = endClock-startClock;
  
  long userSecs = usageP.ru_utime.tv_sec - usageS.ru_utime.tv_sec;
  long userMicro = usageP.ru_utime.tv_usec - usageS.ru_utime.tv_usec;
  long userMilliSecs = userSecs*1000 + userMicro/1000;

  long sysSecs = usageP.ru_stime.tv_sec - usageS.ru_stime.tv_sec;
  long sysMicro = usageP.ru_stime.tv_usec - usageS.ru_stime.tv_usec;
  long sysMilliSecs = sysSecs*1000 + sysMicro/1000;
  
  printf("\n--Statistics--\n\n");
  printf("1 - User Time %ld ms \n", userMilliSecs);
  printf("  - System Time %ld ms\n", sysMilliSecs);
  printf("2 - Wall Clock %ld ms\n",wallClock);
  printf("3 - Times Process was preempted involuntary %ld\n",usageP.ru_nivcsw);
  printf("4 - Times Process gave up CPU voluntary %ld\n",usageP.ru_nvcsw);
  printf("5 - Major Page Faults %ld\n",usageP.ru_majflt);
  printf("6 - Minor Page Faults %ld\n",usageP.ru_minflt);
  printf("7 - Maximum Reident Set Size used %ld kb\n", usageP.ru_maxrss);
  printf("----------\n");

}
/**
 * Forks a new process in which to execute a task, used for all command executions under the doit program
 * @arg argvNew, character array of commands and arguments to pass to execvp
 * @return 0 if forked process succeeded and was awaited, exits otherwise
*/
int exec(char* argvNew[]){
  int pid; //process id
  
  //Initialize resource usage measurements
  long startClock, endClock;//Measures start and end "wall-clock time"
  struct timeval timeCheck;
  struct rusage usageS;//rusage struct from sys/resource.h
  struct rusage usageP;
  
  
  gettimeofday(&timeCheck, NULL);
  
  startClock = (long)timeCheck.tv_usec / 1000 + (long)timeCheck.tv_sec * 1000;
  
  getrusage(RUSAGE_SELF,&usageS);
  
  
  if ((pid = fork()) < 0) {
    cerr << "Fork error\n";
    exit(1);
  }
  else if (pid == 0){
    //Child process
    //Execute command that was inputed
    if (execvp(argvNew[0], argvNew) < 0) {
      cerr << "Invalid command" << endl;
      exit(1);
    }
  }
  else {
    //Parent process
    wait(0); //Wait for child to finish
    //Collect metrics
    gettimeofday(&timeCheck, NULL);
    endClock = (long)timeCheck.tv_usec / 1000 + (long)timeCheck.tv_sec * 1000;
    if ((getrusage(RUSAGE_SELF,&usageP)) == 0){
      printStats(startClock, endClock, usageS, usageP);
      return 0;
    } else{
      cout << "Error getting usage metrics" << endl;
      exit(1);
    }
  }
}

int main(int argc, char *argv[])
 /* argc -- number of arguments */
 /* argv -- an array of strings */
{
  string inputString = ""; //Shell input
  int i; //Iterator
  char* inputCString; //Input for shell converted to c string
  char** inputArgs; //Arguments for shell after parsing
  char** inputArgsBackground; //Arguments for background task after removing &
  int inputSize; //Number of characters in input, excluding null terminator
  
  bool managingBackground = false; //Indicates wheather or not this process is managing a background process or not
  stack<process> backgroundStack; //Stack with ongoing background processes
  int pid; //Process id of background process
  int pos; //Position of & character
  int nBackgroundtask; //Number of background task in fork
  if (argc > 1){
    //Command execution case
    if (exec(argv + 1) == 0){
      exit(0);
    } else{
      exit(1);
    }
    
  }
  else{
    //Shell case
    char prompt[] = "==>";
    while (true){
      cout << prompt;
      //Getting input
      try{
        getline(cin, inputString);
      }catch(int e){
        cout << "Error reading prompt" << endl;
        exit(1);
      }
      
      //Converting input to c-style string before parsing
      inputSize = inputString.length();
      inputCString = new char[inputSize + 1];
      strcpy(inputCString, inputString.c_str());
      //parsing input
      inputArgs = parseShell(inputCString);
      if(!inputArgs || !inputArgs[0]){
        cout << endl;
        continue;
      }
      //Handle background task separately by forking this process
      if((pos = isBackground(inputArgs)) >= 1){
        //Making array without & character
        inputArgsBackground = new char*[pos];
        for (i = 0; i < pos; i++){
          inputArgsBackground[i] = inputArgs[i];
        }
        inputArgsBackground[pos] = '\0';
        if ((pid = fork()) < 0) {
          cerr << "Fork error to execute background task\n";
          //exit(1); //Should the shell terminate if it is unable to start background task?
          continue;
        }
        else if (pid == 0){
          //Child process for background process
          if (exec(inputArgsBackground) == 0){
            cout << '[' << backgroundStack.size() + 1 << "] " << getpid() << " Completed" << endl;
            cout << prompt;
          }else{
            cout << "Background process failed to complete" << endl;
          }
          exit(0);
        }else{
          //Parent process, which will continue to run shell
          backgroundStack.push((process){pid, inputArgs[0]});
          cout << '[' << backgroundStack.size() << "] " <<  pid << endl;
          continue; //Go back to beginning of shell loop
        }
      }
      //Checking for special cases
      if(strcmp(inputArgs[0], "exit") == 0){
        wait(0);
        exit(0);
      }
      else if(strcmp(inputArgs[0], "jobs") == 0){
        listJobs(backgroundStack);
      }
      else if(strcmp(inputArgs[0], "cd") == 0 && inputArgs[1]){
        chdir(inputArgs[1]);
        cout << "Changing directory to: " << inputArgs[1] << endl;
      }else if(inputArgs[1] && inputArgs[2] && inputArgs[3] && strcmp(inputArgs[0], "set") == 0 && strcmp(inputArgs[1], "prompt") == 0 && strcmp(inputArgs[2], "=") == 0){
        strcpy(prompt, inputArgs[3]);
      }else{
        exec(inputArgs);
      }
    }
  }
}