Readme file for Project 1, in CS 3013 - Operating Systems, with Professor Craig Wills,

Pedro de Vasconcellos Oporto
08 / 30 / 2018

    The purpose of this text is to go over how the program works and how to comile and operate it. Main code structure and other details are commented in doit.cpp.
    
The program is called doit and its source code is in doit.cpp, in C++. To compile, navigate to the folder the file is in and type the following in a linux terminal with g++ installed:

                                    g++ -o doit doit.cpp
                                    
After the program doit is compiled, there are 2 ways to run it:

A - Command Execution:

    Type ./doit and any other linux commands and its correspondent arguments, such as "./doit ls" or "./doit wc doit.cpp". This will fork a new process and execute the desired command in a new process. When the forked process is completed, the parent process will output important statistics, as instructed: 
    
            --Statistics--

            1 - User Time 0 ms 
              - System Time 0 ms
            2 - Wall Clock 7 ms
            3 - Times Process was preempted involuntary 0
            4 - Times Process gave up CPU voluntary 1
            5 - Major Page Faults 0
            6 - Minor Page Faults 158
            7 - Maximum Reident Set Size used 2904 kb
            ----------
            
      An example running a few commands this way is in the file commandExec.txt, in this directory.
      
B - "Mini" Shell:

    Only type ./doit with no arguments to open a mini shell that is managed by the program. The shell will initialize with the prompt "==>" and will allow you to execute linux commands like described in (A), also  statistics. On top of normal commands, the "mini" shell can also handle the following commands:
    - "cd <Valid directory>": will change the directory and maintain the shell running.
    - "set prompt = <new prompt>": will change the prompt of the mini shell to whatever term/word is given.
    - "exit": will stop shell and quit execution of doit
    
    Examples of running the mini shell, including these "special" commands is in the file shellExec.txt, in this directory.
    
    Background tasks:
    
    In case one desires to run a task on the background and keep the shell running without waiting, it can add the character '&' at the end of the command. The process id of the forked process running the task will be send to the terminal and a new prompt will come up. When any task is done, the output will intermingle with whatever is also being done in the mini shell, and a completion message will be displayed, along with the command statistics. The program will also not exit until all background tasks are awaited. To list all background tasks currently still running, especifically its command and process id, the command "jobs", with no arguments, can be run in the mini shell. An example of all functionalities of background tasks is in the backgroundExec.txt file, in this directory.
    
    Mini-shell vs Linux shell:
    
    When looking at the mini-shell developed in this project, it has very limited functionality but similar feeling to the regular linux shell. The outputs can get confusing and sometimes not show the prompt, depending on the order commands are called. The shell will also handle invalid commands and other exceptions such as the linux shell. "Meta" commands regarding background executions and directory management are definately limited when compared to the linux shell, but any commands that can be run with execvp will run normally. From a user perspective, the mini-shell also does not support navigating previous input with up and down arrows, like the Linux shell does.