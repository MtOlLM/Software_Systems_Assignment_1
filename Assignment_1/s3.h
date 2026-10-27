#ifndef _S3_H_
#define _S3_H_

///See reference for what these libraries provide
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <stdbool.h>

///Constants for array sizes, defined for clarity and code readability
#define MAX_LINE 1024
#define MAX_ARGS 128
#define MAX_PROMPT_LEN 256
#define PIPE_PORT_NUM 4

///Enum for readable argument indices (use where required)
enum ArgIndex
{
    ARG_PROGNAME,
    ARG_1,
    ARG_2,
    ARG_3,
};


//Enum for readable redirection bits
enum RedirectionBit{
    INPUT_BIT,
    OUTPUT_BIT
};
///With inline functions, the compiler replaces the function call 
///with the actual function code;
///inline improves speed and readability; meant for short functions (a few lines).
///the static here avoids linker errors from multiple definitions (needed with inline).
static inline void reap()
{
    wait(NULL);
}

///Shell I/O and related functions (add more as appropriate)
void read_command_line(char line[], char *lwd);
void construct_shell_prompt(char shell_prompt[], char *lwd);
void parse_command(char line[], char *args[], int *argsc);

///Child functions (add more as appropriate)
void child(char *args[], int argsc);
///Program launching functions (add more as appropriate)

//Determine whether a command contains redirection.
int command_with_redirection(char *args[], int argsc);
//redirection command child function
void child_with_redirection(char *args[], int argsc);
//functions handling two redirection cases
void input_redirection(char *args[], int argsc, int idx);
void output_redirection(char *args[], int argsc, int idx, bool overwrite);

//initializes lwd with the cwd (using getcwd)
void init_lwd(char *lwd);

bool is_cd(char *args[],int argsc);
void run_cd(char *args[],int argsc,char *lwd, char *pwd);

void pipe_redirection(int in_fd, int out_fd, int unused_fd);
void single_stage_command(char *args[], int argsc, char *lwd, char *pwd,int in_fd, int out_fd, int unused_fd, int *childcnt);
void single_command_handler(char *args[], int argsc, char *lwd, char *pwd);
#endif