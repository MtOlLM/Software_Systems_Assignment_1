typedef struct node{
    struct node *next;
    data_t d
} node_t;


int lock = 0;
void insert_at_head( node_t **list_head, node_t *newnode){
    while(test_and_set(&lock, 0, 1)){
        node_t *tmp = *list_head;
        newnode -> next = tmp;
        lock = 0;
    }
}