#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>

#define DEV_NAME "led_control"
#define PROC_NAME "led_control"
#define GPIO_LED 539  // GPIO27 BCM

static int led_status = 0;
static struct proc_dir_entry *proc_entry;

/* File operations for /dev/led_control */
static int dev_open(struct inode *inode, struct file *file)
{
    pr_info("%s: Device opened\n", DEV_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    pr_info("%s: Device closed\n", DEV_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    char tmp[2];

    if (*offset > 0)
        return 0;

    tmp[0] = gpio_get_value(GPIO_LED) ? '1' : '0';
    tmp[1] = '\n';

    if (copy_to_user(buf, tmp, sizeof(tmp)))
        return -EFAULT;

    *offset += sizeof(tmp);
    return sizeof(tmp);
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    char value;

    if (copy_from_user(&value, buf, 1))
        return -EFAULT;

    switch (value) {
        case '0':
            gpio_set_value(GPIO_LED, 0);
            led_status = 0;
            break;
        case '1':
            gpio_set_value(GPIO_LED, 1);
            led_status = 1;
            break;
        default:
            pr_warn("%s: Invalid value written: %c\n", DEV_NAME, value);
            return -EINVAL;
    }

    return count;
}

static const struct file_operations dev_fops = {
    .owner   = THIS_MODULE,
    .open    = dev_open,
    .release = dev_release,
    .read    = dev_read,
    .write   = dev_write,
};

/* File operations for /proc/led_control */
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    char tmp[64];
    int len;

    if (*offset > 0)
        return 0;

    len = snprintf(tmp, sizeof(tmp), "LED is %s\n", led_status ? "ON" : "OFF");

    if (copy_to_user(buf, tmp, len))
        return -EFAULT;

    *offset += len;
    return len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

/* Define misc device */
static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEV_NAME,
    .fops  = &dev_fops,
};

/* Module init */
static int __init led_init(void)
{
    int ret;

    pr_info("%s: Initializing module\n", DEV_NAME);

    ret = misc_register(&misc_dev);
    if (ret) {
        pr_err("%s: Failed to register misc device\n", DEV_NAME);
        goto err_misc;
    }

    proc_entry = proc_create(PROC_NAME, 0444, NULL, &proc_fops);
    if (!proc_entry) {
        pr_err("%s: Failed to create /proc/%s\n", DEV_NAME, PROC_NAME);
        ret = -ENOMEM;
        goto err_proc;
    }

    ret = gpio_request(GPIO_LED, "led_gpio");
    if (ret) {
        pr_err("%s: Failed to request GPIO\n", DEV_NAME);
        goto err_gpio;
    }

    ret = gpio_direction_output(GPIO_LED, 0);
    if (ret) {
        pr_err("%s: Failed to set GPIO direction\n", DEV_NAME);
        goto err_dir;
    }

    pr_info("%s: Driver loaded\n", DEV_NAME);
    return 0;

/* Cleanup blocks */
err_dir:
    gpio_free(GPIO_LED);
err_gpio:
    proc_remove(proc_entry);
err_proc:
    misc_deregister(&misc_dev);
err_misc:
    return ret;
}

/* Module exit */
static void __exit led_exit(void)
{
    gpio_set_value(GPIO_LED, 0);
    gpio_free(GPIO_LED);
    proc_remove(proc_entry);
    misc_deregister(&misc_dev);
    pr_info("%s: Module removed\n", DEV_NAME);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hao Phan");
MODULE_DESCRIPTION("A simple LED driver using /dev and /proc interface");
MODULE_VERSION("1.0");

module_init(led_init);
module_exit(led_exit);