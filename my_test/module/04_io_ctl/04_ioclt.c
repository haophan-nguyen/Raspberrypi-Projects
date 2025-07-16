#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ioctl.h>
#include "ioctl.h"

#define GPIO_LED 539    //GPIO27

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("A character driver control led using ioctl");

/* Struct device */
typedef struct led_device {
    char *device_name;
    char *class_name;
    dev_t device_number;
    struct class *device_class;
    struct cdev device_cdev;
} led_device;

static led_device led = {
    .device_name = "led_blink",
    .class_name  = "led_class"
};

/* Dummy file operations */
static int dev_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    blink blink_time;
    switch (cmd)
    {
    case IOCTL_LED_ON:
        gpio_set_value(GPIO_LED, 1);
        printk(KERN_INFO "Led on\n");
        break;
    case IOCTL_LED_OFF:
        gpio_set_value(GPIO_LED, 0);
        printk(KERN_INFO "Led off\n");
        break;
    case IOCTL_LED_TOGGLE:
        if(gpio_get_value(GPIO_LED)){
            gpio_set_value(GPIO_LED, 0);
        }else{
            gpio_set_value(GPIO_LED, 1);
        }
        printk(KERN_INFO "Led toggle\n");
        break;
    case IOCTL_LED_BLINK:
        if (blink_time.time < 0 || blink_time.on_time_ms < 0 || blink_time.off_time_ms < 0 || blink_time.time > 100) {
            printk(KERN_ERR "Invalid blink config values\n");
            return -EINVAL;
        }
        if(copy_from_user(&blink_time, (blink __user*)arg, sizeof(blink))){
            printk(KERN_ERR "Fail to copy blink times from user\n");
            return -EFAULT;
        }
        printk(KERN_INFO "Led blink %d time\n", blink_time.time);
        for(int i = 0; i < blink_time.time; i++){
            gpio_set_value(GPIO_LED, 1);
            msleep(blink_time.on_time_ms);
            gpio_set_value(GPIO_LED, 0);
            msleep(blink_time.off_time_ms);
        }
        break;
    default:
        printk(KERN_ERR "Invalid value\n");
        break;
    }

    return 0;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .unlocked_ioctl = led_ioctl,
};

static int __init led_driver_init(void) {
    printk("Module is loaded\n");

    // Allocate device number
    if (alloc_chrdev_region(&led.device_number, 0, 1, led.device_name) < 0) {
        printk(KERN_ERR "ERROR: Allocate driver number failed\n");
        return -1;
    }

    // Create device class
    led.device_class = class_create(led.class_name);
    if (IS_ERR(led.device_class)) {
        printk(KERN_ERR "ERROR: Create class failed\n");
        goto class_error;
    }

    // Create device file
    if (device_create(led.device_class, NULL, led.device_number, NULL, led.device_name) == NULL) {
        printk(KERN_ERR "ERROR: Create device failed\n");
        goto device_error;
    }

    // Map cdev with file operations
    cdev_init(&led.device_cdev, &fops);
    if (cdev_add(&led.device_cdev, led.device_number, 1) == -1) {
        printk(KERN_ERR "ERROR: Add cdev failed\n");
        goto cdev_error;
    }

    /* Set up led */
    // request GPIO
    if(gpio_request(GPIO_LED, "led_gpio")){
        printk(KERN_ERR "ERROR: Fail to request GPIO %d\n", GPIO_LED);
        goto gpio_error;
    }

    // set led is output
    if(gpio_direction_output(GPIO_LED, 0)){
        printk(KERN_ERR "ERROR: Fail to set GPIO %d as output\n", GPIO_LED);
        goto dir_error;
    }else{
        gpio_set_value(GPIO_LED, 1);
        msleep(1000);
        gpio_set_value(GPIO_LED, 0);
    }

    return 0;

dir_error:
    gpio_free(GPIO_LED);
gpio_error:
    cdev_del(&led.device_cdev);
cdev_error:
    device_destroy(led.device_class, led.device_number);
device_error:
    class_destroy(led.device_class);
class_error:
    unregister_chrdev_region(led.device_number, 1);
    return -1;
}

static void __exit led_driver_exit(void) {
    device_destroy(led.device_class, led.device_number);
    class_destroy(led.device_class);
    cdev_del(&led.device_cdev);
    unregister_chrdev_region(led.device_number, 1);
    printk(KERN_INFO "Module is removed\n");
}

module_init(led_driver_init);
module_exit(led_driver_exit);