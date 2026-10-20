# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
int main () {
    int fd = open ( " test . txt " , O_CREAT | O_TRUNC | O_WRONLY , 0644) ;
    if ( fork () == 0) {
        // Child
        close ( fd ) ;
    } else {
        // Parent
        wait ( NULL ) ;
        if ( write ( fd , " hello \ n " , 6) == -1) {
            printf ( " write failed \ n " ) ;
        } else {
            printf ( " write succeeded \ n " ) ;
        }
    }
    return 0;
}