#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[], char *lwd)
{
    strcpy(shell_prompt, "[");
    strcat(shell_prompt, lwd);
    strcat(shell_prompt, "]$ ");
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[], char *lwd)
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt, lwd);
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
    char *token = strtok(line, " \t\n");//any of three: space, tab and newline
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
    //if execvp() successes, the following lines won't be excecuted
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
        //O_TRUNC clears the file before writing to it
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
        //output redirection
        if(strcmp(">",args[i]) == 0 || strcmp(">>",args[i]) == 0){
            if(i + 1 >= argsc){
                printf("missing output file name\n");
                exit(1);
            }
            output_redirection(args, argsc, i+1, strcmp(">",args[i]) == 0);
            i++;
        }
        //input redirection
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

void init_lwd(char *lwd){
    //If lwd is NULL, getcwd() has failed
    getcwd(lwd,MAX_PROMPT_LEN-6);
    if(lwd == NULL){
        printf("get directory failed\n");
        exit(1);
    }
}

bool is_cd(char *args[], int argsc){
    if (strcmp(args[ARG_PROGNAME],"cd") == 0) return true;
    return false;
}

void run_cd(char *args[],int argsc,char *lwd,char *pwd){
    char *arg1;
    arg1 = args[1];
    if(arg1 == NULL){
        strcpy(pwd,lwd);
        strcpy(lwd,"/");
    }else if(strcmp(arg1,"-") == 0){
        //swap lwd and pwd
        char temp[MAX_PROMPT_LEN-6]; 
        strcpy(temp,lwd);
        strcpy(lwd,pwd);
        strcpy(pwd,temp);
    }else{
        strcpy(pwd,lwd);
        strcpy(lwd,arg1);
    }
    if(chdir(lwd) == -1){
        printf("change directory failed\n");
        exit(1);
    }
}