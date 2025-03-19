�޳N���O
---
https://hackmd.io/eGiLmu2fRj6tork3b0hi5g?view

����y�{���z
---

>User ���ε{���ϥ� `syscall(XXX, 1)` �N process �[�J wait queue�C
>���ݱ��󺡨���Aprocess �Q����C
>�D�{���ϥ� `syscall(XXX, 2)` �̦�������ݤ��� process�A���� FIFO ���ǰh�X�C

System call ������\��� Kernel ���� `SYSCALL_DEFINE` macro ��{�G
`SYSCALL_DEFINE1(call_my_wait_queue, int, id)`

* call_my_wait_queue �O System call ���W�١C

* int id �O�q User space �ǻ��L�Ӫ��ѼơC

�ھڶǤJ���Ѽ� `id`�A����������\��G

* id == 1�G�N��e process �[�J wait queue�C
* id == 2�G��� queue �����Ҧ� process�C

Context switch �^ User space�G�t�ΩI�s������A�����^ User space�A�{���i�H�~�����C

�Ӹ`����
---
```c
DECLARE_WAIT_QUEUE_HEAD(my_wait_queue); // �ŧi�@�ӵ��ݦ�C
LIST_HEAD(my_list);                     // �w�q�@�� list head�A�x�s process ���
static DEFINE_MUTEX(my_mutex);          // �w�q������A�O�@�@�ɸ귽
static int condition = 0;               // ���ݦ�C������
```
* my_wait_queue : �ϥ� Kernel ���Ѫ� macro DECLARE_WAIT_QUEUE_HEAD�A�ŧi�@�ӦW�� my_wait_queue �� wait queue�C

* my_list : �ŧi�@����� my_list�A�Ψ��x�s process ��T�]�p PID�^�Cmy_list �O�@�� Kernel ���Ѫ� Doubly Linked List ���c�C

* my_mutex : �ŧi�ê�l�Ƥ@�ӦW�� my_mutex ��������A�Ω�O�@�@�ɸ귽�]�p my_list�^�H���� race condition�C

* condition : �w�q�@�ӱ����ܼ� `condition`�A�Ψӱ��� wait queue �� process �O�_���ӳQ����C�� condition ���� process �����ݱ���ɡA�� process �|�Q����C

```c
static int enter_wait_queue(void){
    int pid = (int)current->pid;                    // �����e process �� PID
    struct my_data *entry;

    entry = kmalloc(sizeof(*entry), GFP_KERNEL);    // ���t�O����
    entry->pid = pid;                               // �O�s process �� PID
    list_add_tail(&entry->list, &my_list);          // �N��ƥ[�J linked list

    // �� condition == pid ���~�����
    wait_event_interruptible(my_wait_queue, condition == pid);  
    return 0;
}
```

>�� process �I�s����ƮɡA���|�Q�K�[�� wait queue `my_wait_queue`�A�öi�J�ίv���A�A������� `condition == pid` ��������C
>`wait_event_interruptible` �O Kernel ���Ѫ� API�A�Ω󵥫ݬY�ӱ���C

* �ϥ� `current` ���������e process �� PID�C

* �ŧi�@�ӫ��V `my_data` ���c������ `entry`�A�Ψ��x�s��e process ��������T�C

* �ϥ� `kmalloc` ��Ƭ� `my_data` ���c���t�O����C`GFP_KERNEL` �O���t�O���骺�лx�A��ܸӾާ@�O�b kernel space ����A���\�ίv���ݡC

> ```c
> void *kmalloc(size_t size, gfp_t flags);
> ```
> `flags`�G���t�O���骺�覡�α���]�p�O�_���\�ίv���ݵ��^�Ckmalloc ��^���O����a�}�Q��ȵ� entry�A�Ϩ���V�s���t���O����ϰ�C

����{���X���ܼ� `entry` ���t�@���j�p�� `sizeof(*entry)` ���O����Ŷ��A�Ω��x�s `struct my_data` ����ơC
`entry` �O `struct my_data` �����СA���t�o�q�O����O���F�b `list_add_tail` ���N `entry` �@������`�I�x�s process ��ơ]�p PID�^�C��^���O����a�}��ȵ� `entry`�A�H�K����ϥΡC
* `entry->pid = pid` �N��e process �� PID �s�x�� `my_data` ���c���C

```c
list_add_tail(&entry->list, &my_list);
```
> * `entry->list` ��ܷs�`�I�C
> * `my_list` �O�D���C
> * �N `entry->list` �@���s�`�I�[�J�� `my_list` �������C
> * �B���A`my_list` �����`�I�|��s�� `entry->list`�A�O�ҫ���s�W���`�I�~��K�[������A���� FIFO ���ǡC

* ��e process �i�J�ίv�A���� `condition == pid` �ɳQ����C`my_wait_queue` �O wait queue�A`condition == pid` �O process ���������C

```c
static int clean_wait_queue(void){
    struct my_data *entry;
    list_for_each_entry(entry, &my_list, list) {
        condition = entry->pid;                     // �]�m����
        wake_up_interruptible(&my_wait_queue);      // ������ݤ��� process 
        msleep(100);                                // ����100ms -> FIFO
    }
    return 0;
}
```
>�M�� `my_list`�A�̦��]�m `condition` �� process �� PID�A�ó�������� process�C
>`wake_up_interruptible` �O kernel ���Ѫ� API�A�Ω󵥫ݬY�ӱ���C
* �ŧi�@�ӫ��V `my_data` ������ `*entry`�A�Ω�M�� `my_list` �����C�� process ��ơC
* �ϥ� kernel ���Ѫ� `list_for_each_entry` marco�A�M�� `my_list` ������C�Ӹ`�I�C`entry` �O��e�`�I�����СC
* �C�����N�G
`entry` �̦����V `my_list` �����U�@�Ӹ`�I�C
�ھ� `entry->pid` �]�w����A�ó������ process�C
�M���L�{��` Doubly Linked List �����ǡA�O�Ҹ`�I�� FIFO ���ǳB�z�C
* `condition = entry->pid`: �N�����ܼ� condition �]�m����e process �� PID�A���� process ���������C
* ��� wait queue `my_wait_queue` ���ŦX���� process�C�Q����� process �|�q `wait_event_interruptible` ��ƪ�^���~�����C
* �ϥ� `msleep` ��Ʃ��� 100 �@��A���� FIFO ��������ǡC

```c
SYSCALL_DEFINE1(call_my_wait_queue, int, id){
    switch (id){
        case 1:
            mutex_lock(&my_mutex);          // ���������
            enter_wait_queue();             // �N process �[�J���ݦ�C
            mutex_unlock(&my_mutex);        // ���񤬥���
            break;
        case 2:
            clean_wait_queue();             // �D�{�ǩI�s�A�M�z���ݦ�C
            break;
    }
    return 0;
}
```

>* `id == 1` �ɡA�ե� `enter_wait_queue()`�A�N��e process �[�J wait queue�C
>* `id == 2` �ɡA�ե� `clean_wait_queue()`�A�̦�������� queue ���� process�C
* `id == 1`�G��������� `my_mutex`�A���� Multi processing �P�ɾާ@ my_list�C
    * �I�s `enter_wait_queue()`�A�N��e process �[�J wait queue�C
    * �̫����񤬥���C

* `id == 2`�G
�I�s `clean_wait_queue()`�A�̦���� wait queue �����Ҧ� process�C