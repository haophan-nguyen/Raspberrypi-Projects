#include<linux/module.h>
#include<linux/init.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include<linux/gpio.h>
#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/delay.h>
#include<linux/interrupt.h>

typedef struct mydevice {
    char *device_name;
    char *class_name;
    dev_t dev_nr;
    struct class *dev_class;
    struct cdev cdev;
    int led_gpio;
    int button_gpio;
    int irq_nr;
} mydevice;

static mydevice mydev = {
    .device_name = "led_control",
    .class_name = "led_class",
    .led_gpio = 539,     // GPIO27
    .button_gpio = 529 // GPIO17
};

static int dev_open(struct inode *inode, struct file *file){
    printk("INFO: Device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file){
    printk("INFO: Device closed\n");
    return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset){
    char value;
    if (copy_from_user(&value, buf, 1)){
        printk("ERROR: Fail to copy data from user\n");
        return -1;
    }

    switch(value){
        case '0':
            gpio_set_value(mydev.led_gpio, 0);
            printk("LED OFF\n");
            break;

        case '1':
            gpio_set_value(mydev.led_gpio, 1);
            printk("LED ON");
            break;

        default:
            printk("ERROR: Invalid value: %c\n", value);
            break;
    }

    return count;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset){
    char tmp[2];
    int value = gpio_get_value(mydev.led_gpio);
    tmp[0] = value ? '1' : '0';
    tmp[1] = '\n';
    if (copy_to_user(buf, tmp, 2)){
        printk("ERROR: Faile to copy data to user\n");
        return -1;
    }

    return 2;
}

static irqreturn_t irq_callback(int irq, void *dev_id) {
    printk("*********************\n");
    printk("\t\t%s\t\t\n", __func__);
    printk("*********************\n");


    if(gpio_get_value(mydev.button_gpio) == 0){
        gpio_set_value(mydev.led_gpio, 0);
        printk("Button pressed! LED OFF\n");
    }else{
        gpio_set_value(mydev.led_gpio, 1);
        printk("Button pressed! LED ON\n");
    }

    return IRQ_HANDLED;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read,
};

static int __init my_device_init(void){
    printk("INFO: Module is loaded\n");

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
    } else{
        // Get irq number form btn
        mydev.irq_nr = gpio_to_irq(mydev.button_gpio);
        if (mydev.irq_nr < 0){
            printk("ERROR: Fai to get IRQ for GPIO %d\n", mydev.button_gpio);
            return -1;
        }

        // Register irq handler
        if (request_irq(mydev.irq_nr, irq_callback, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "btn_irq", NULL)){
            printk("ERROR: Fail to register irq handler\n");
            goto irq_err;
        }
    }

    /* LED */
    if (gpio_request(mydev.led_gpio, "led_gpio")){
        printk("ERROR: Fail to request gpio %d\n", mydev.led_gpio);
        goto gpio_err;
    }

    if (gpio_direction_output(mydev.led_gpio, 0)){
        printk("ERROR: Fail to set gpio %d as output\n", mydev.led_gpio);
        goto led_dir_err;
    } else{
        gpio_set_value(mydev.led_gpio, 1);
        mdelay(1000);
        gpio_set_value(mydev.led_gpio, 0);
    }


    return 0;

led_dir_err:
    gpio_free(mydev.led_gpio);
irq_err:
    free_irq(mydev.irq_nr, NULL);
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

static void __exit my_device_exit(void){
    gpio_free(mydev.led_gpio);
    cdev_del(&mydev.cdev);
    device_destroy(mydev.dev_class, mydev.dev_nr);
    class_destroy(mydev.dev_class);
    unregister_chrdev_region(mydev.dev_nr, 1);
    printk("INFO: Module is removed\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("A driver led control using irq from button");

module_init(my_device_init);
module_exit(my_device_exit);