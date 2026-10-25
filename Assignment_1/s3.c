#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0';
}

void parse_command(char line[], char *args[], int *argsc)
{
    ///Implements simple tokenization (space delimited)
    ///Note: strtok puts '\0' (null) characters within the existing storage, 
    ///to split it into logical cstrings.
    ///There is no dynamic allocation.

    ///See the man page of strtok(...)
    char *token = strtok(line, " ");
    *argsc = 0;
    while (token != NULL && *argsc < MAX_ARGS - 1)
    {
        args[(*argsc)++] = token;
        token = strtok(NULL, " ");
    }
    
    args[*argsc] = NULL; ///args must be null terminated
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.
    execvp(args[ARG_PROGNAME], args);
    printf("execvp() failed\n%s",args[0]);
    exit(1);
}

void launch_program(char *args[], int argsc)
{
    ///Implement this function:
    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.
    if (strcmp("exit",args[ARG_PROGNAME]) == 0) {
        exit(0);
    }

    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.
    int rc = fork();
    if(rc < 0){
        printf("fork failed\n");
        exit(1);
    }
    else if (rc == 0){
        //Child branch
        child(args, argsc);
    }
    else{
        //Parent branch
        reap();
    }

}

bool command_with_redirection(char line[]){
    for(int i = 0; i < strlen(line); i++){
        char c;
        c = line[i];
        if(c == '<' || c == '>') return true;
    }
    return false;
}

void input_redirection(char *args[], int argsc, int idx){
    //making 100% sure
    if(idx >= argsc){
        printf("missing output file name\n");
        exit(1);
    }

    int fd = open(args[idx],O_RDONLY);

    if(fd == -1){
        printf("open input file failed\n");
        exit(1);
    }

    if(dup2(fd, STDIN_FILENO) == -1){
        printf("dup2 input failed\n");
        close(fd);
        exit(1);
    }

    close(fd);

}
void output_redirection(char *args[], int argsc, int idx, bool overwrite){
    //making 100% sure
    if(idx >= argsc){
        printf("missing output file name\n");
        exit(1);
    }

    int fd;
    if(overwrite){
        fd = open(args[idx], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    else{
        fd = open(args[idx], O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    if(fd == -1){
        printf("open output file failed\n");
        exit(1);
    }

    if(dup2(fd, STDOUT_FILENO) == -1){
        printf("dup2 output failed\n");
        close(fd);
        exit(1);
    }

    close(fd);
    
}




void child_with_redirection(char *args[], int argsc){
    //+1 for filename testing purpose
    char *clean_args[argsc+1];
    int clean_argsc = 0;

    for(int i = 0; i < argsc; i++){
        if(strcmp(">",args[i]) == 0 || strcmp(">>",args[i]) == 0){
            if(i + 1 >= argsc){
                printf("missing output file name\n");
                exit(1);
            }
            output_redirection(args, argsc, i+1, strcmp(">",args[i]) == 0);
            i++;
        }
        else if(strcmp("<",args[i]) == 0){
            if(i + 1 >= argsc){
                printf("missing output file name\n");
                exit(1);
            }
            input_redirection(args, argsc, i+1);
            i++;
        }
        else{
            clean_args[clean_argsc++] = args[i];
        }
    }
    clean_args[clean_argsc] = NULL; ///args must be null terminated

    child(clean_args,clean_argsc);
}


void launch_program_with_redirection(char *args[], int argsc){
    int rc = fork();
    if(rc < 0){
        printf("fork failed\n");
        exit(1);
    }
    else if (rc == 0){
        //Child branch
        child_with_redirection(args, argsc);
    }
    else{
        //Parent branch
        reap();
    }
}