#include <linux/init.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/sched/signal.h>
#include <linux/signal.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Hao");
MODULE_DESCRIPTION("A misc driver that demonstrates sending signal to user-space");

#define DEV_NAME "my_misc"
#define BUF_SIZE 128

static char kernel_buf[BUF_SIZE];      // Temporary buffer for write input
static char display_buf[BUF_SIZE];     // Buffer to store message for read
static int user_pid = -1;              // User-space process PID

/**
 * @brief Write handler - Handles data written from user-space
 *
 * If the written string is a PID (number), it stores the PID.
 * If the string is "trigger", it sends a signal to the stored PID.
 * Otherwise, it stores the string for reading via read().
 */
static ssize_t misc_write(struct file *file, const char __user *buf,
                          size_t len, loff_t *ppos)
{
    if (len >= BUF_SIZE){
        len = BUF_SIZE -1;
    }

    memset(kernel_buf, 0, BUF_SIZE);

    if (copy_from_user(kernel_buf, buf, len)) {
        pr_err("misc_write: Failed to copy data from user\n");
        return -EFAULT;
    }

    kernel_buf[len] = '\0';
    *ppos = 0;

    pr_info("misc_write: Received string: '%s' (len = %zu)\n", kernel_buf, len);

    // Case: PID input
    if (kernel_buf[0] >= '0' && kernel_buf[0] <= '9') {
        if (kstrtoint(kernel_buf, 10, &user_pid) == 0) {
            pr_info("misc_write: PID stored: %d\n", user_pid);
        } else {
            pr_err("misc_write: Invalid PID format\n");
            return -EINVAL;
        }
    }
    // Case: Trigger signal
    else if (strncmp(kernel_buf, "trigger", 7) == 0) {
        pr_info("misc_write: Trigger received. Attempting to send signal to PID %d...\n", user_pid);

        struct pid *pid_struct = find_get_pid(user_pid);
        if (!pid_struct) {
            pr_err("misc_write: PID not found\n");
            return -ESRCH;
        }

        struct task_struct *task = get_pid_task(pid_struct, PIDTYPE_PID);
        if (!task) {
            pr_err("misc_write: Task not found for PID %d\n", user_pid);
            return -ESRCH;
        }

        struct kernel_siginfo info = {
            .si_signo = SIGUSR1,
            .si_code = SI_QUEUE,
            .si_int  = 1234,
        };

        if (send_sig_info(SIGUSR1, &info, task) < 0) {
            pr_err("misc_write: Failed to send signal to user process\n");
            return -EFAULT;
        }

        pr_info("misc_write: Signal sent successfully to PID %d\n", user_pid);
    }
    // Case: Store general message
    else {
        snprintf(display_buf, BUF_SIZE, "%s", kernel_buf);
        pr_info("misc_write: Stored message: '%s'\n", display_buf);
        pr_info("misc_write: Unrecognized input. Treated as a message\n");
    }

    return len;
}

/**
 * @brief Read handler - Returns last stored message from write()
 */
static ssize_t misc_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    if (*off > 0 || len < strlen(display_buf))
        return 0;

    if (copy_to_user(buf, display_buf, strlen(display_buf)))
        return -EFAULT;

    *off += strlen(display_buf);
    pr_info("misc_read: Returned string to user: '%s'\n", display_buf);
    return strlen(display_buf);
}

static struct file_operations misc_fops = {
    .owner = THIS_MODULE,
    .read  = misc_read,
    .write = misc_write,
};

static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEV_NAME,
    .fops  = &misc_fops,
};

/**
 * @brief Module initialization
 */
static int __init dev_init(void)
{
    pr_info("misc_dev: Registering device '%s'\n", DEV_NAME);
    return misc_register(&misc_dev);
}

/**
 * @brief Module cleanup
 */
static void __exit dev_exit(void)
{
    pr_info("misc_dev: Unregistering device '%s'\n", DEV_NAME);
    misc_deregister(&misc_dev);
}

module_init(dev_init);
module_exit(dev_exit);
