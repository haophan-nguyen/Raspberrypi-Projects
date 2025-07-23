#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/delay.h>

static struct task_struct *write_thread, *read_thread1, *read_thread2;

static struct semaphore my_sem;
static int shared_data = 0;
static bool warned = false;

static int write_thread_fn(void *data){
    int i = 0;
    while(!kthread_should_stop()){
        if(down_trylock(&my_sem) == 0){
            shared_data = i++;
            printk(KERN_INFO "%s wrote: %d\n", __func__, shared_data);
            msleep(5000);
            up(&my_sem); // Release semaphore
            warned = false;
        }else{
            if(!warned){
                printk(KERN_INFO "%s: semaphore is locked, can't write now\n", __func__);
                msleep(1000);
                warned = true;
            }
        }
    }
    
    printk(KERN_INFO "%s is stopping...\n", __func__);
    return 0;
}

static int read_thread1_fn(void *data){
    while(!kthread_should_stop()){
        if(down_trylock(&my_sem) == 0){
                printk(KERN_INFO "%s read: %d\n", __func__, shared_data);
                msleep(5000);
                up(&my_sem); // Release semaphore
                warned = false;
        }else{
            if(!warned){
                printk(KERN_INFO "%s: semaphore is locked, can't read now\n", __func__);
                msleep(1000);
                warned = true;
            }
        }
    }

    printk(KERN_INFO "%s is stopping...\n", __func__);
    return 0;
}

static int read_thread2_fn(void *data){
    while(!kthread_should_stop()){
        if(down_trylock(&my_sem) == 0){
                printk(KERN_INFO "%s read: %d\n", __func__, shared_data);
                msleep(5000);
                up(&my_sem); // Release semaphore
                warned = false;
        }else{
            if(!warned){
                printk(KERN_INFO "%s: semaphore is locked, can't read now\n", __func__);
                msleep(1000);
                warned = true;
            }
        }
    }

    printk(KERN_INFO "%s is stopping...\n", __func__);
    return 0;
}

static int __init my_module_init(void){
    printk(KERN_INFO "Module is loaded\n");
    sema_init(&my_sem, 2);

    write_thread = kthread_create(write_thread_fn, NULL, "write thread");
    if(IS_ERR(write_thread)){
        printk(KERN_ERR "Failed to create %s thread\n", "write_thread");
        return PTR_ERR(write_thread);
    }

    read_thread1 = kthread_create(read_thread1_fn, NULL, "read thread1");
    if(IS_ERR(read_thread1)){
        printk(KERN_ERR "Failed to create %s thread\n", "read_thread1");
        return PTR_ERR(read_thread1);
    }

    read_thread2 = kthread_create(read_thread2_fn, NULL, "read thread2");
    if(IS_ERR(read_thread2)){
        printk(KERN_ERR "Failed to create %s thread\n", "read_thread2");
        return PTR_ERR(read_thread2);
    }

    wake_up_process(write_thread);
    wake_up_process(read_thread1);
    wake_up_process(read_thread2);

    return 0;
}

static void __exit my_module_exit(void) {
    if (write_thread){
        kthread_stop(write_thread);
    }

    if (read_thread1){
        kthread_stop(read_thread1);
    }

    if (read_thread2){
        kthread_stop(read_thread2);
    }
    printk(KERN_INFO "Semaphore module unloaded.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("Example using semaphore with thread");

module_init(my_module_init);
module_exit(my_module_exit);