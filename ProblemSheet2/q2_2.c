# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
int main ( int argc , char * argv []) {
    int rc = fork () ;
    if ( rc < 0) {
        fprintf ( stderr , " fork failed \n " ) ;
        exit (1) ;
    }
    else if ( rc == 0) {
        sleep (2) ;
        printf ( " This is C . pid : %d , Parent pid : % d \n " , getpid () ,
        getppid () ) ;
    } else {
        printf ( " This is P . pid : % d \n " , getpid () ) ;
    }
    return 0;
}