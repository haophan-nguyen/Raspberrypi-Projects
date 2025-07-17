#include<linux/module.h>
#include<linux/init.h>
#include<linux/kernel.h>
#include<linux/kthread.h>
#include<linux/delay.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/poll.h>

typedef struct mydevice {
    char *device_name;
    char *class_name;
    dev_t dev_nr;
    struct class *dev_class;
    struct cdev cdev;
    int button_gpio;
} mydevice;

static mydevice mydev = {
    .device_name = "my_btn",
    .class_name = "btn_class",
    .button_gpio = 529 // GPIO17
};

/* Variable */
static struct task_struct *my_thread;   //thread
static DECLARE_WAIT_QUEUE_HEAD(wq);     //waitqueue
static int btn_flag = 0;

__poll_t btn_poll(struct file *filp, poll_table *wait){
    poll_wait(filp, &wq, wait);
    if(btn_flag){
        btn_flag = 0;
        return  POLLIN | POLLRDNORM;
    }

    return  0;
}

int thread_fn(void *data){
    printk(KERN_INFO "%s is running ....\n", __func__);
    while (!kthread_should_stop()){
        if(gpio_get_value(mydev.button_gpio)){
            btn_flag = 1;
            wake_up_interruptible(&wq);     // wake up
        }
        ssleep(1);
    }

    printk(KERN_INFO "%s is stoping ....\n", __func__);
    return 0;
}

static int dev_open(struct inode *inode, struct file *file){
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file){
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .poll = btn_poll,
};

static int __init thread_device_init(void){
    printk(KERN_INFO "Module is loaded\n");
        // Allocate device number
    if (alloc_chrdev_region(&mydev.dev_nr, 0, 1, mydev.device_name) < 0){
        printk("ERROR: Fail to allocate device number\n");
        return -1;
    }

    // Create device class
    mydev.dev_class = class_create(mydev.class_name);
    if (IS_ERR(mydev.dev_class)){
        printk("ERROR: Fail to create device class\n");
        goto class_err;
    }

    // Create device
    if (device_create(mydev.dev_class, NULL, mydev.dev_nr, NULL, mydev.device_name) == NULL){
        printk("ERROR: Fail to create device\n");
        goto dev_err;
    }

    // Map cdev with file operations
    cdev_init(&mydev.cdev, &fops);
    if (-1 == cdev_add(&mydev.cdev, mydev.dev_nr, 1)){
        printk("ERROR: Fail to add cdev\n");
        goto cdev_err;
    }

    // Set up GPIO
    /* BUTTON */
    if (gpio_request(mydev.button_gpio, "btn_gpio")){
        printk("ERROR: Fail to request gpio %d\n", mydev.button_gpio);
        goto gpio_err;
    }

    if (gpio_direction_input(mydev.button_gpio)){
        printk("ERROR: Fail to set gpio %d as input\n", mydev.button_gpio);
        goto btn_dir_err;
    }

    /* Create thread */
    my_thread = kthread_create(thread_fn, NULL, "my_thread");
    if(IS_ERR(my_thread)){
        printk(KERN_ERR "Fail to create thread %s\n", "my_thread");
        return PTR_ERR(my_thread);
    }

    wake_up_process(my_thread);
    return 0;

btn_dir_err:
    gpio_free(mydev.button_gpio);
gpio_err:
    cdev_del(&mydev.cdev);
cdev_err:
    device_destroy(mydev.dev_class, mydev.dev_nr);
dev_err:
    class_destroy(mydev.dev_class);
class_err:
    unregister_chrdev_region(mydev.dev_nr, 1);
    return -1;
}

static void __exit thread_device_exit(void){
    if(my_thread){
        kthread_stop(my_thread);
    }
    gpio_free(mydev.button_gpio);
    cdev_del(&mydev.cdev);
    device_destroy(mydev.dev_class, mydev.dev_nr);
    class_destroy(mydev.dev_class);
    unregister_chrdev_region(mydev.dev_nr, 1);
    printk(KERN_INFO "Module is removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("A sample driver using thread function + poll + waitqueue");

module_init(thread_device_init);
module_exit(thread_device_exit);