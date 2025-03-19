#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/wait.h>
#include <linux/list.h>
#include <asm/current.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>

struct my_data {
    int pid;
    struct list_head list;
};

DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);
LIST_HEAD(my_list);
static DEFINE_MUTEX(my_mutex); 

static int condition = 0;
static int wait_time = 0;
static int enter_wait_queue(void){
    int pid = (int)current->pid;
    struct my_data *entry;
    printk("add to wait queue\n");
    entry = kmalloc(sizeof(*entry), GFP_KERNEL);
    entry->pid = pid;
    list_add_tail(&entry->list, &my_list);
    printk("Added process with pid=%d to my_list\n", entry->pid);
    wait_event_interruptible(my_wait_queue, condition == pid);  
    return 0;
}

static int clean_wait_queue(void){
    struct my_data *entry;
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;
        printk("wake up pid=%d\n", condition);
        wake_up_interruptible(&my_wait_queue);
        msleep(100);
    }

    return 0;
}

SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            mutex_lock(&my_mutex); 
            enter_wait_queue();
            mutex_unlock(&my_mutex);
            break;
        case 2:
            clean_wait_queue();
            break;
    }
    return 0;
}