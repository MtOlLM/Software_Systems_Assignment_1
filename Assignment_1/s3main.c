#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN-6]; 
    //The working directory saved for cd - command
    char pwd[MAX_PROMPT_LEN-6];
    init_lwd(lwd);///Implement this function: initializes lwd with the cwd (using getcwd)
    strcpy(pwd,lwd);
    //Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///Stores the number of arguments
    int argsc;

    while (1) {
        init_lwd(lwd);
        read_command_line(line, lwd); ///Notice the additional parameter (required for prompt construction)
        parse_command(line, args, &argsc);
        if(is_cd(args, argsc)){///Implement this function 
            run_cd(args, argsc, lwd, pwd); ///Implement this function
        }
        else if(command_with_redirection(line)){
            ///Command with redirection
           launch_program_with_redirection(args, argsc);
           reap();
       }
       else ///Basic command
       {
           launch_program(args, argsc);
           reap();
       }
    }

    return 0;
}
