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

int command_with_redirection(char *args[], int argsc){
    bool input,output;
    input = false;
    output = false;
    for(int i = 0; i < argsc; i++){
        if(strcmp(">",args[i]) == 0 || strcmp(">>",args[i]) == 0) output = true;
        else if(strcmp("<",args[i]) == 0) input = true;
    }
    return (output << OUTPUT_BIT) | input;
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

void init_lwd(char *lwd){
    //If return NULL, getcwd() has failed
    if(getcwd(lwd,MAX_PROMPT_LEN-6) == NULL){
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
    arg1 = args[ARG_1];
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

void pipe_closer(int pipe_fd[]){
    for(int i = 0; i < PIPE_PORT_NUM; i++){
        if(pipe_fd[i] != -1){
            close(pipe_fd[i]);
        }
    }
}

void pipe_redirection(int pipe_fd[], bool pipe_connection[]){
    int dp;
    for(int i=0; i < PIPE_PORT_NUM; i++){
        if(pipe_fd[i] != -1){
            if(pipe_connection[i]){
                if(i <= INPUT_WRITE){
                    dp = dup2(pipe_fd[i],STDIN_FILENO);
                }else{
                    dp = dup2(pipe_fd[i],STDOUT_FILENO);
                }
                if(dp == -1){
                    printf("dup2 failed");
                    exit(1);
                }
            }
            close(pipe_fd[i]);
        }
    }
}

void single_process_command(char *args[], int argsc, char *lwd, char *pwd,int pipe_fd[], bool pipe_connection[], int *childcnt){
    if(is_cd(args, argsc)){//cd command
            run_cd(args, argsc, lwd, pwd);
    }
    else if (strcmp("exit",args[ARG_PROGNAME]) == 0) {
        exit(0);
    }
    else{
        int rc = fork();
        if(rc < 0){
            printf("fork failed\n");
            exit(1);
        }
        else if (rc == 0){
            //Child branch
            pipe_redirection(pipe_fd, pipe_connection);

            if(command_with_redirection(args, argsc)){
                child_with_redirection(args, argsc);
            }else{
                child(args, argsc);
            }
            pipe_closer(pipe_fd);
        }
        else{
            //Parent branch
            (*childcnt)++;
        }
    }
}


void single_command_handler(char *args[], int argsc, char *lwd, char *pwd){
    //current and previous command
    char *args_c[MAX_ARGS], *args_p[MAX_ARGS];
    int argsc_c = 0, argsc_p = -1;
    int pipe_fd_c[PIPE_PORT_NUM], pipe_fd_p[PIPE_PORT_NUM];
    bool pipe_connection_c[PIPE_PORT_NUM], pipe_connection_p[PIPE_PORT_NUM];
    //initialization
    for(int i = 0; i < PIPE_PORT_NUM; i++){
        pipe_fd_p[i] = -1;
        pipe_fd_c[i] = -1;
        pipe_connection_c[i] = false;
        pipe_connection_p[i] = false;
    }
    int fd[2],fd_p[2],fd_pp[2];
    for(int i = 0; i < 2;i++){
        fd[i] = -1;
        fd_p[i] = -1;
        fd_pp[i] = -1;
    }
    int rc;
    int is_redirected;
    int childcnt = 0;

    for (int i = 0; i < argsc; i++){
        if(strcmp("|", args[i]) == 0){
            if(argsc_c == 0 || (i + 1 >= argsc)){
                printf("pipe input/output does not exist");
                exit(1);
            }
            
            args_c[argsc_c] = NULL;
            rc = pipe(fd);
            if(rc < 0){
                printf("pipe creation failed");
                exit(1);
            }
            pipe_fd_c[OUTPUT_READ] = fd[0];
            pipe_fd_c[OUTPUT_WRITE] = fd[1];
            pipe_connection_c[OUTPUT_READ] = false;
            pipe_connection_c[INPUT_WRITE] = false;

            is_redirected = command_with_redirection(args_c, argsc_c);
            //The current command should read from the pipe if it:
            //1. is not the first command
            //2. does not have a read redirection
            //3. the previous command (if exist) writes to the pipe
            pipe_connection_c[INPUT_READ] = (argsc_p != -1) && !((is_redirected >> INPUT_BIT) & 1) && pipe_connection_p[OUTPUT_WRITE];
            //The current command should write to the pipe if it:
            //1. is not the last command (the last otwo will be excecuted separately)
            //2. does not have a write redirection
            //3. the next command (if exist) reads the pipe
            pipe_connection_c[OUTPUT_WRITE] = !((is_redirected >> OUTPUT_BIT) & 1);
            if(argsc_p != -1){
                pipe_connection_p[OUTPUT_WRITE] = pipe_connection_p[OUTPUT_WRITE] && pipe_connection_c[INPUT_READ];
                //assign a pointer for command execution
                char *args_exe[MAX_ARGS];
                memcpy(args_exe, args_p, sizeof(args_exe));
                single_process_command(args_exe, argsc_p, lwd, pwd, pipe_fd_p, pipe_connection_p, &childcnt);
                pipe_closer(fd_pp);
            }

            //copy
            memcpy(args_p, args_c, sizeof(args_p));
            argsc_p = argsc_c;
            memcpy(pipe_connection_p, pipe_connection_c, sizeof(pipe_connection_p));
            memcpy(pipe_fd_p, pipe_fd_c, sizeof(pipe_fd_p));
            //reset
            argsc_c = 0;
            //for the next command coming
            pipe_fd_c[INPUT_READ] = fd[0];
            pipe_fd_c[INPUT_WRITE] = fd[1];
            fd_pp[0] = fd_p[0];
            fd_pp[1] = fd_p[1];
            fd_p[0] = fd[0];
            fd_p[1] = fd[1];
        }
        else{
            args_c[argsc_c++] = args[i];
        }
    }
    //excecute the last two command
    args_c[argsc_c] = NULL;
    pipe_fd_c[OUTPUT_READ] = -1;
    pipe_fd_c[OUTPUT_WRITE] = -1;
    pipe_connection_c[OUTPUT_READ] = false;
    pipe_connection_c[INPUT_WRITE] = false;
    is_redirected = command_with_redirection(args_c, argsc_c);
    pipe_connection_c[INPUT_READ] = (argsc_p != -1) && !((is_redirected >> INPUT_BIT) & 1) && pipe_connection_p[OUTPUT_WRITE];
    pipe_connection_c[OUTPUT_WRITE] = false;
    pipe_connection_p[OUTPUT_WRITE] = pipe_connection_p[OUTPUT_WRITE] && pipe_connection_c[INPUT_READ];
    if(argsc_p != -1){
        single_process_command(args_p, argsc_p, lwd, pwd, pipe_fd_p, pipe_connection_p, &childcnt);
        pipe_closer(fd_pp);
    } 
    single_process_command(args_c, argsc_c, lwd, pwd, pipe_fd_c, pipe_connection_c, &childcnt);
    pipe_closer(fd_p);
    //reap all child together
    for(int i = 0; i < childcnt; i++){
        reap();
    }
}
