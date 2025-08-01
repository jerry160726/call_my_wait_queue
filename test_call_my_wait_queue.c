#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#define NUM_THREADS 10
// 引入基本函式庫。定義使用10個執行緒

void *enter_wait_queue(void *thread_id){
    fprintf(stderr, "enter wait queue thread_id: %d\n", *(int *)thread_id);

    // your syscall here 
    syscall( 450 , 1);

    fprintf(stderr, "exit wait queue thread_id: %d\n", *(int *)thread_id);
}

void *clean_wait_queue(){
    // your syscall here 
    syscall( 450 , 2);
    
}

int main(){
    void *ret;
    pthread_t id[NUM_THREADS];
    int thread_args[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++){
        thread_args[i] = i;
        pthread_create(&id[i], NULL, enter_wait_queue, (void *)&thread_args[i]);
    }
    sleep(1); // Pause briefly to ensure other threads have entered the wait 
    fprintf(stderr,  "start clean queue ...\n");
    clean_wait_queue();
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(id[i], &ret);
    }
    return 0;
}