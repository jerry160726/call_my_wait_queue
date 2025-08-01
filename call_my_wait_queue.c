#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <asm/current.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>

// 定義my_data結構來記錄等待中的Process。pid：儲存process ID。
struct my_data {
    int pid;
    struct list_head list;      // list_head結構，使這個節點可以加入Linux的linked list
};

// 宣告一個名為my_wait_queue的wait queue。所有等待的process都會在這裡進入休眠
DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);

// 宣告並初始化一個雙向linked list的list head（起始節點），名稱叫my_list。儲存所有等待的Process資訊。
LIST_HEAD(my_list);

// 宣告並初始化一個互斥鎖，用來保護像my_list這種多執行緒共享資源
static DEFINE_MUTEX(my_mutex); 

static int condition = 0;       // 作為喚醒等待process的條件變數
static int wait_time = 0;
static int enter_wait_queue(void){
    int pid = (int)current->pid;    // 取得當前執行緒（process）的PID
    struct my_data *entry;
    printk("add to wait queue\n");
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);    // 為entry分配記憶體。大小是sizeof(*entry)，就是struct my_data的大小
    entry->pid = pid;                               // 將分配的entry結構中的pid欄位填入當前process的PID
    list_add_tail(&entry->list, &my_list);          // 把entry加進kernel的linked list my_list的尾端（FIFO）
    printk("Added process with pid=%d to my_list\n", entry->pid);

    // 讓目前的Process 睡在my_wait_queue中，直到condition == pid成立，或是收到interrupt為止
    wait_event_interruptible(my_wait_queue, condition == pid);  
    return 0;
}

static int clean_wait_queue(void){
    struct my_data *entry;
    // 遍歷linked list中的每一個節點。
    // 依序取出之前加入list的每個my_data節點（每個等待的process）
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;                     // 將目前這個節點（等待的process）的pid設定給全域變數condition
        printk("wake up pid=%d\n", condition);

        // 喚醒睡在my_wait_queue裡的所有process。每個被喚醒的Process都會重新檢查這個條件：condition == pid
        wake_up_interruptible(&my_wait_queue);
        msleep(100);    // 等待100ms，讓喚醒動作能以FIFO進行
    }

    return 0;
}

SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            // 保證多個thread同時執行enter_wait_queue() 時，不會同時修改my_list，避免race condition
            mutex_lock(&my_mutex);      // 嘗試取得my_mutex鎖，如果目前鎖已被其他thread/process持有，就會sleep等待鎖釋放
            enter_wait_queue();
            mutex_unlock(&my_mutex);    // 讓其他process可以繼續呼叫enter_wait_queue()
            break;
        case 2:
            clean_wait_queue();
            break;
    }
    return 0;
}