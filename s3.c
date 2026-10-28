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
        perror("fgets failed\n");
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
        token = strtok(NULL, " \t\n");
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
    printf("execvp() failed%s\n",args[0]);
    exit(1);
}

bool command_with_redirection(char *args[], int argsc){
    for(int i = 0; i < argsc; i++){
        if(strcmp(">",args[i]) == 0 || strcmp(">>",args[i]) == 0 || strcmp("<",args[i]) == 0) return true;
    }
    return false;
}

void input_redirection(char *args[], int argsc, int idx){
    //making 100% sure
    if(idx >= argsc){
        printf("missing input file name\n");
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
    char cd[MAX_PROMPT_LEN];
    char *arg1 = args[ARG_1];

    if(arg1 == NULL) strcpy(cd, "/");
    else if(strcmp(arg1,"-") == 0) strcpy(cd, pwd);
    else strcpy(cd, arg1);

    if(chdir(cd) == -1){
        printf("change directory failed\n");
        return;
    }

    strcpy(pwd,lwd);
    init_lwd(lwd);
}

void pipe_redirection(int in_fd, int out_fd, int unused_fd){
    //wire up the pipe ends
    if((in_fd != -1) && (dup2(in_fd, STDIN_FILENO) == -1)){
        printf("dup2 failed\n");
        exit(1);
    }
    if((out_fd != -1) && (dup2(out_fd, STDOUT_FILENO) == -1)){
        printf("dup2 failed\n");
        exit(1);
    }
    //close all original pipe ports
    if(in_fd != -1) close(in_fd);
    if(out_fd != -1) close(out_fd);
    if(unused_fd != -1) close(unused_fd);
}

void single_stage_command(char *args[], int argsc, char *lwd, char *pwd,int in_fd, int out_fd, int unused_fd, int *childcnt){
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
            pipe_redirection(in_fd, out_fd, unused_fd);

            if(command_with_redirection(args, argsc)){
                child_with_redirection(args, argsc);
            }else{
                child(args, argsc);
            }
        }
        else{
            //Parent branch
            (*childcnt)++;
        }
    }
    if (in_fd != -1)  close(in_fd);
    if (out_fd != -1) close(out_fd);
}


void single_command_handler(char *args[], int argsc, char *lwd, char *pwd){
    //current and previous command
    char *cmd[MAX_ARGS];
    int cmdc = 0;
    int pre_read = -1;
    int childcnt = 0;
    for (int i = 0; i <= argsc; i++){
        bool is_last = i == argsc;
        bool is_pipe = !is_last && (strcmp("|", args[i]) == 0);
        if(!is_last && !is_pipe){
            cmd[cmdc++] = args[i];
            continue;
        }
        
        if(cmdc == 0){
            printf("pipe input/output does not exist\n");
            if(pre_read != -1) close(pre_read);
            break;
        }

        cmd[cmdc] = NULL;

        int fd[2] = {-1, -1};

        if(!is_last){
            if(pipe(fd) < 0){
                printf("pipe creation failed\n");
                exit(1);
            }   
        }

        single_stage_command(cmd, cmdc, lwd, pwd, pre_read, fd[1], fd[0], &childcnt);
        pre_read = fd[0];
        cmdc = 0;
    }

    //reap all child together
    for(int i = 0; i < childcnt; i++){
        reap();
    }
}

bool is_subshell(char line[]){
    int l_cnt = 0, r_cnt = 0;

    int len = strlen(line);
    for(int i = 0; i < len; i++){
        if(r_cnt > l_cnt){
            printf("subshell error\n");
            exit(1);
        }
        if(line[i] == '('){
            l_cnt++;
        }
        else if(line[i] == ')'){
            r_cnt++;
        }
    }
    if(l_cnt != r_cnt){
        printf("subshell error\n");
        exit(1);
    }
    return l_cnt != 0;
}

void index_finder(char line[], int *l_idx, int *r_idx){
    int l_cnt = 0, r_cnt = 0;

    int len = strlen(line);
    for(int i = 0; i < len; i++){
        if(line[i] == '('){
            l_cnt++;
            if(l_cnt == 1){
                *l_idx = i;
            }
        }
        else if(line[i] == ')'){
            r_cnt++;
            if(l_cnt == r_cnt){
                *r_idx = i;
                return;
            }
        }
    }
}

void str_div(char line[], char front[], char middle[], char back[], int l_idx, int r_idx){
    int len = strlen(line);
    int f = 0, m = 0, b = 0;
    for(int i = 0; i < len; i++){
        if(i < (l_idx-2)){ //so the ';' wont be included int the front string
            front[f++] = line[i];
        }
        else if(i < r_idx && i > l_idx){
            middle[m++] = line[i];
        }
        else if(i > (r_idx+2)){
            back[b++] = line[i];
        }
    }
    front[f] = '\0';
    middle[m] = '\0';
    strcat(middle, " ; exit"); // so the subshell can exit automatically
    back[b] = '\0';
}

void subshell_handler(char line[], char *lwd, char *pwd){
    int l_idx, r_idx;
    index_finder(line, &l_idx, &r_idx);
    char front[MAX_PROMPT_LEN], middle[MAX_PROMPT_LEN], back[MAX_PROMPT_LEN];
    str_div(line, front, middle, back, l_idx, r_idx);

    shell_handler(front, lwd, pwd);

    int rc = fork();
    if(rc < 0){
        printf("fork failed\n");
        exit(1);
    }
    else if (rc == 0){
        shell_handler(middle, lwd, pwd);
    }
    else{
        reap();
    }

    shell_handler(back, lwd, pwd);
}

void non_subshell_handler(char line[], char *lwd, char *pwd){
    char *args[MAX_ARGS];
    int argsc;
    char *args_b[MAX_ARGS];
    int argsc_b = 0;
    parse_command(line, args, &argsc);
    for(int i = 0; i < argsc; i++){
        if((strcmp(args[i],";") == 0) || (i == argsc - 1)){
            if(i == argsc - 1) args_b[argsc_b++] = args[i];
            args_b[argsc_b] = NULL;
            if(argsc_b != 0) single_command_handler(args_b, argsc_b, lwd, pwd);
            argsc_b = 0;
        }else{
            args_b[argsc_b++] = args[i];
        }
    }
}

void shell_handler(char line[], char *lwd, char *pwd){
    if(is_subshell(line)){
        subshell_handler(line, lwd, pwd);
    }else{
        non_subshell_handler(line, lwd, pwd);
    }
}

