#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#define NUM_THREADS 10
// 引入基本函式庫。定義使用10個執行緒

void *enter_wait_queue(void *thread_id){
    // 將執行緒ID透過stderr印出來，顯示目前哪個thread正在進入Wait Queue
    fprintf(stderr, "enter wait queue thread_id: %d\n", *(int *)thread_id);

    // 對應Kernel中的enter_wait_queue()。該執行緒會加入Kernel中的Wait Queue並進入休眠（直到被喚醒）
    syscall( 450 , 1);

    // 當該執行緒被喚醒後，輸出離開Wait Queue的訊息
    fprintf(stderr, "exit wait queue thread_id: %d\n", *(int *)thread_id);
}

void *clean_wait_queue(){
    // 執行Kernel端的clean_wait_queue()。該函數會依序喚醒Wait Queue中的執行緒（每次延遲100ms保證FIFO）
    syscall( 450 , 2);
}

int main(){
    void *ret;
    // 宣告pthread陣列與參數陣列（讓每個thread知道自己的編號）。
    pthread_t id[NUM_THREADS];
    int thread_args[NUM_THREADS];

    /*建立10個執行緒，每個thread都執行enter_wait_queue()，並傳入自己的ID。
      每個thread都會呼叫syscall並進入Wait Queue*/
    for (int i = 0; i < NUM_THREADS; i++){
        // 把第i條thread的編號存到thread_args[i]裡
        thread_args[i] = i;                 
        // 建立一條新thread，並執行enter_wait_queue()
        pthread_create(&id[i], NULL, enter_wait_queue, (void *)&thread_args[i]);
    }

    sleep(1); // 等待1秒，確保所有執行緒都已經進入等待狀態
    fprintf(stderr,  "start clean queue ...\n");
    clean_wait_queue();     // 開始喚醒Wait Queue。呼叫syscall清理Wait Queue，依序喚醒每個執行緒。

    // 等待所有執行緒完成（被喚醒後執行完畢）
    for (int i = 0; i < NUM_THREADS; i++){
        pthread_join(id[i], &ret);  
    }
    return 0;
}