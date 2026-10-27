#define DEC_LOCK 0
#define INC_LOCK 1

int lock = 0;
int x;

void inc(){
    while(compare_and_swap(lock, 0, 1)){
    }
    x += 1;
    lock = 0;
}

void dec(){
    while(compare_and_swap(&lock, 0, 1) != 0){
    }
    x -= 1;
    lock = 0;
}