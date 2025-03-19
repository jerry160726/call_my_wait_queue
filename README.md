技術筆記
---
https://hackmd.io/eGiLmu2fRj6tork3b0hi5g?view

整體流程概述
---

>User 應用程式使用 `syscall(XXX, 1)` 將 process 加入 wait queue。
>等待條件滿足後，process 被喚醒。
>主程式使用 `syscall(XXX, 2)` 依次喚醒等待中的 process，按照 FIFO 順序退出。

System call 的具體功能由 Kernel 中的 `SYSCALL_DEFINE` macro 實現：
`SYSCALL_DEFINE1(call_my_wait_queue, int, id)`

* call_my_wait_queue 是 System call 的名稱。

* int id 是從 User space 傳遞過來的參數。

根據傳入的參數 `id`，執行相應的功能：

* id == 1：將當前 process 加入 wait queue。
* id == 2：喚醒 queue 中的所有 process。

Context switch 回 User space：系統呼叫完成後，控制返回 User space，程式可以繼續執行。

細節解釋
---
```c
DECLARE_WAIT_QUEUE_HEAD(my_wait_queue); // 宣告一個等待佇列
LIST_HEAD(my_list);                     // 定義一個 list head，儲存 process 資料
static DEFINE_MUTEX(my_mutex);          // 定義互斥鎖，保護共享資源
static int condition = 0;               // 等待佇列的條件
```
* my_wait_queue : 使用 Kernel 提供的 macro DECLARE_WAIT_QUEUE_HEAD，宣告一個名為 my_wait_queue 的 wait queue。

* my_list : 宣告一個鏈表 my_list，用來儲存 process 資訊（如 PID）。my_list 是一個 Kernel 提供的 Doubly Linked List 結構。

* my_mutex : 宣告並初始化一個名為 my_mutex 的互斥鎖，用於保護共享資源（如 my_list）以防止 race condition。

* condition : 定義一個條件變數 `condition`，用來控制 wait queue 中 process 是否應該被喚醒。當 condition 滿足 process 的等待條件時，該 process 會被喚醒。

```c
static int enter_wait_queue(void){
    int pid = (int)current->pid;                    // 獲取當前 process 的 PID
    struct my_data *entry;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);    // 分配記憶體
    entry->pid = pid;                               // 保存 process 的 PID
    list_add_tail(&entry->list, &my_list);          // 將資料加入 linked list

    // 當 condition == pid 時繼續執行
    wait_event_interruptible(my_wait_queue, condition == pid);  
    return 0;
}
```

>當 process 呼叫此函數時，它會被添加到 wait queue `my_wait_queue`，並進入睡眠狀態，直到條件 `condition == pid` 滿足為止。
>`wait_event_interruptible` 是 Kernel 提供的 API，用於等待某個條件。

* 使用 `current` 指標獲取當前 process 的 PID。

* 宣告一個指向 `my_data` 結構的指標 `entry`，用來儲存當前 process 的相關資訊。

* 使用 `kmalloc` 函數為 `my_data` 結構分配記憶體。`GFP_KERNEL` 是分配記憶體的標誌，表示該操作是在 kernel space 執行，允許睡眠等待。

> ```c
> void *kmalloc(size_t size, gfp_t flags);
> ```
> `flags`：分配記憶體的方式或條件（如是否允許睡眠等待等）。kmalloc 返回的記憶體地址被賦值給 entry，使其指向新分配的記憶體區域。

此行程式碼為變數 `entry` 分配一塊大小為 `sizeof(*entry)` 的記憶體空間，用於儲存 `struct my_data` 的資料。
`entry` 是 `struct my_data` 的指標，分配這段記憶體是為了在 `list_add_tail` 中將 `entry` 作為鏈表的節點儲存 process 資料（如 PID）。返回的記憶體地址賦值給 `entry`，以便後續使用。
* `entry->pid = pid` 將當前 process 的 PID 存儲到 `my_data` 結構中。

```c
list_add_tail(&entry->list, &my_list);
```
> * `entry->list` 表示新節點。
> * `my_list` 是主鏈表。
> * 將 `entry->list` 作為新節點加入到 `my_list` 的尾部。
> * 運行後，`my_list` 的尾節點會更新為 `entry->list`，保證後續新增的節點繼續添加到尾部，維持 FIFO 順序。

* 當前 process 進入睡眠，等待 `condition == pid` 時被喚醒。`my_wait_queue` 是 wait queue，`condition == pid` 是 process 的喚醒條件。

```c
static int clean_wait_queue(void){
    struct my_data *entry;
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;                     // 設置條件
        wake_up_interruptible(&my_wait_queue);      // 喚醒等待中的 process 
        msleep(100);                                // 延遲100ms -> FIFO
    }
    return 0;
}
```
>遍歷 `my_list`，依次設置 `condition` 為 process 的 PID，並喚醒對應的 process。
>`wake_up_interruptible` 是 kernel 提供的 API，用於等待某個條件。
* 宣告一個指向 `my_data` 的指標 `*entry`，用於遍歷 `my_list` 中的每個 process 資料。
* 使用 kernel 提供的 `list_for_each_entry` marco，遍歷 `my_list` 鏈表中的每個節點。`entry` 是當前節點的指標。
* 每次迭代：
`entry` 依次指向 `my_list` 中的下一個節點。
根據 `entry->pid` 設定條件，並喚醒對應 process。
遍歷過程遵循 Doubly Linked List 的順序，保證節點按 FIFO 順序處理。
* `condition = entry->pid`: 將條件變數 condition 設置為當前 process 的 PID，滿足 process 的喚醒條件。
* 喚醒 wait queue `my_wait_queue` 中符合條件的 process。被喚醒的 process 會從 `wait_event_interruptible` 函數返回並繼續執行。
* 使用 `msleep` 函數延遲 100 毫秒，模擬 FIFO 的喚醒順序。

```c
SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            mutex_lock(&my_mutex);          // 獲取互斥鎖
            enter_wait_queue();             // 將 process 加入等待佇列
            mutex_unlock(&my_mutex);        // 釋放互斥鎖
            break;
        case 2:
            clean_wait_queue();             // 主程序呼叫，清理等待佇列
            break;
    }
    return 0;
}
```

>* `id == 1` 時，調用 `enter_wait_queue()`，將當前 process 加入 wait queue。
>* `id == 2` 時，調用 `clean_wait_queue()`，依次喚醒等待 queue 中的 process。
* `id == 1`：獲取互斥鎖 `my_mutex`，防止 Multi processing 同時操作 my_list。
    * 呼叫 `enter_wait_queue()`，將當前 process 加入 wait queue。
    * 最後釋放互斥鎖。

* `id == 2`：
呼叫 `clean_wait_queue()`，依次喚醒 wait queue 中的所有 process。