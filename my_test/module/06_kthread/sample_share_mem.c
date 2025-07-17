#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include <linux/wait.h>
#include <linux/sched.h>

/* Variable */
static struct task_struct *write_thread;   // write thread
static struct task_struct *read_thread;    // read thread
static DECLARE_WAIT_QUEUE_HEAD(wq);        // waitqueue
static int condition = 0;
static int share_mem = 0;

int write_thread_fn(void *data){
    int i = 0;
    printk(KERN_INFO "%s is running ....\n", __func__);
    while (!kthread_should_stop()){
        ssleep(3);
        share_mem = i++;
        condition = 1;
        printk(KERN_INFO "%s wrote: data = %d\n", __func__, share_mem);
        wake_up_interruptible(&wq);
    }

    printk(KERN_INFO "%s is stoping ....\n", __func__);
    return 0;
}

int read_thread_fn(void *data){
    printk(KERN_INFO "%s is running ....\n", __func__);
    while(!kthread_should_stop()){
        wait_event_interruptible(wq, condition != 0);
        printk(KERN_INFO "%s read: data = %d\n", __func__, share_mem);
        condition = 0;
    }
    printk(KERN_INFO "%s is stoping ....\n", __func__);
    return 0;
}


static int __init thread_device_init(void){
    printk(KERN_INFO "Module is loaded\n");
    /* init waitqueue */
    init_waitqueue_head(&wq);

    /* Create thread */
    write_thread = kthread_run(write_thread_fn, NULL, "write_thread");
    read_thread = kthread_run(read_thread_fn, NULL, "read_thread");

    return 0;
}

static void __exit thread_device_exit(void){
    if(write_thread){
        kthread_stop(write_thread);
    }

    if(read_thread){
        kthread_stop(read_thread);
    }
    printk(KERN_INFO "Module is removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("A sample driver using thread function to share data");

module_init(thread_device_init);
module_exit(thread_device_exit);