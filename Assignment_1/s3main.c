#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN-6]; 
    //The working directory saved for cd - command
    char pwd[MAX_PROMPT_LEN-6];
    //initialize two directories
    init_lwd(lwd);///Implement this function: initializes lwd with the cwd (using getcwd)
    strcpy(pwd,lwd);
    
    //Stores pointers to command arguments.
    //The first element of the array is the command name.
    char *args[MAX_ARGS];
    ///Stores the number of arguments
    int argsc;

    //for batched commands
    char *args_b[MAX_ARGS];
    int argsc_b = 0;

    while (1) {
        init_lwd(lwd);
        read_command_line(line, lwd); ///Notice the additional parameter (required for prompt construction)
        parse_command(line, args, &argsc);
        for(int i = 0; i < argsc; i++){
            if((strcmp(args[i],";") == 0) || (i == argsc - 1)){
                if(i == argsc - 1) args_b[argsc_b++] = args[i];
                args_b[argsc_b] = NULL;
                single_command_handler(args_b, argsc_b, lwd, pwd);
                argsc_b = 0;
            }else{
                args_b[argsc_b++] = args[i];
            }
        }
    }

    return 0;
}
