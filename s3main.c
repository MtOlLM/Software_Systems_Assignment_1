#include "s3.h"

int main(int argc, char *argv[]){
    ///Stores the command line input
    char line[MAX_LINE];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN]; 
    //The working directory saved for cd - command
    char pwd[MAX_PROMPT_LEN];
    //initialize two directories
    init_lwd(lwd);///Implement this function: initializes lwd with the cwd (using getcwd)
    strcpy(pwd,lwd);

    while (1) {
        read_command_line(line, lwd); ///Notice the additional parameter (required for prompt construction)
        shell_handler(line, lwd, pwd);
    }

    return 0;
}
