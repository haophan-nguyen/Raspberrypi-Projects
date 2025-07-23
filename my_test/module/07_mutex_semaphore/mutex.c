#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/delay.h>

static struct task_struct *write_thread;   // write thread
static struct task_struct *read_thread;    // read thread
static DEFINE_MUTEX(my_mutex);
static int share_mem = 0;
static int prev_locked = 0;  // 0: unlocked, 1: locked

int write_thread_fn(void *data){
    int i = 0;
    printk(KERN_INFO "%s is running ....\n", __func__);
    while (!kthread_should_stop()){
        if (mutex_trylock(&my_mutex)){
            share_mem = i++;
            if(prev_locked){
                printk(KERN_INFO "%s wrote: data = %d\n", __func__, share_mem);
                prev_locked = 0;
            }
            msleep(5000);
            mutex_unlock(&my_mutex);    // unlock mutex
        } else {
            if (!prev_locked) {
                printk(KERN_INFO "%s: mutex is locked, cannot write now\n", __func__);
                prev_locked = 1;
            }
        }
    }

    printk(KERN_INFO "%s is stoping ....\n", __func__);
    return 0;
}

int read_thread_fn(void *data){
    printk(KERN_INFO "%s is running ....\n", __func__);
    while(!kthread_should_stop()){
        if(mutex_trylock(&my_mutex)){
            if (prev_locked) {
                printk(KERN_INFO "%s read: data = %d\n", __func__, share_mem);
                prev_locked = 0;
            }
            msleep(5000);
            mutex_unlock(&my_mutex);    // unlock mutex
        } else {
            if (!prev_locked) {
                printk(KERN_INFO "%s: mutex is locked, cannot read now\n", __func__);
                prev_locked = 1;
            }
        }
    }
    printk(KERN_INFO "%s is stoping ....\n", __func__);
    return 0;
}

static int __init thread_device_init(void){
    printk(KERN_INFO "Module is loaded\n");

    /* Create thread */
    write_thread = kthread_create(write_thread_fn, NULL, "write_thread");
    if (IS_ERR(write_thread)) {
        printk(KERN_ERR "Failed to create write_thread\n");
        return PTR_ERR(write_thread);
    }

    read_thread = kthread_create(read_thread_fn, NULL, "read_thread");
    if (IS_ERR(read_thread)) {
        printk(KERN_ERR "Failed to create write_thread\n");
        return PTR_ERR(read_thread);
    }

    wake_up_process(write_thread);
    wake_up_process(read_thread);

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
MODULE_DESCRIPTION("Example using mutex with thread");

module_init(thread_device_init);
module_exit(thread_device_exit);