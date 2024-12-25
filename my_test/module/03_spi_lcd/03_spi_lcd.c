/**
 * This is a Driver to comunicate with LCD 16x2
 * When user write a string to device, It will display on LCD
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/string.h>

/* Define for LCD */
#define I2C_ADDR 0x27
#define ENABLE 0x04
#define BACKLIGHT 0x08

static struct i2c_client *lcd_client; 
static struct i2c_adapter *lcd_adapter;

//##################### LCD FUNCTION #####################
/* Send enable signal to LCD */
static void lcd_toggle_enable(u8 data) {
    i2c_smbus_write_byte(lcd_client, data | ENABLE);
    udelay(500);
    i2c_smbus_write_byte(lcd_client, data & ~ENABLE);
    udelay(500);
}

/* Send command to LCD */
static void lcd_send_command(u8 cmd) {
    u8 high = cmd & 0xF0;
    u8 low = (cmd << 4) & 0xF0;

    i2c_smbus_write_byte(lcd_client, high | BACKLIGHT);
    lcd_toggle_enable(high | BACKLIGHT);

    i2c_smbus_write_byte(lcd_client, low | BACKLIGHT);
    lcd_toggle_enable(low | BACKLIGHT);
}

/* Send data to LCD */
static void lcd_send_data(u8 data) {
    u8 high = data & 0xF0;
    u8 low = (data << 4) & 0xF0;

    i2c_smbus_write_byte(lcd_client, high | BACKLIGHT | 0x01);
    lcd_toggle_enable(high | BACKLIGHT | 0x01);			// Make sure this is a data not a command

    i2c_smbus_write_byte(lcd_client, low | BACKLIGHT | 0x01);
    lcd_toggle_enable(low | BACKLIGHT | 0x01);
}

/* Print data on LCD */
static void lcd_print(const char *str) {
    int char_count = 0;  // Counter to track the number of printed characters
    lcd_send_command(0x01);  // Clear the screen

    while (*str != '\0') {
        if (char_count < 16) {  // Print characters on line 1
            lcd_send_data(*str++);
            char_count++;
        } else if (char_count < 32) {  // Print characters on line 2
            if (char_count == 16) {
                lcd_send_command(0xC0);  // Move the cursor to the beginning of line 2
            }
            lcd_send_data(*str++);
            char_count++;
        } else {
            break;  // Stop printing once 32 characters (16 on each line) are printed
        }
    }
}



/* Init LCD */
static void lcd_init(void) {
    lcd_send_command(0x33); // 4-bits mode
    lcd_send_command(0x32); // 4-bits mode
    lcd_send_command(0x28); // 2 line, 5x8 pixel
    lcd_send_command(0x0C); // Do not show the pointer
    lcd_send_command(0x01); // Clean screen
    // lcd_send_command(0xC0); // Put the pointer at header of line 2
    msleep(2);
}


//##########################################################

/* Meta info */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Phan Hao");
MODULE_DESCRIPTION("A driver to communicate with LCD 16x2 thougth I2C protocol");

/*Buffer for data*/
static char buffer[30];
static size_t buff_p = 0;

/*Variable for driver and driver class*/
static dev_t device_nr;		// device number (major and minor)
static struct class *my_class;
static struct cdev my_device;

#define DRIVER_NAME "lcd_device"
#define DRIVER_CLASS "myClass"
#define DEBUG 1

/**
 * @brief This function is called when the device is opened
 */
static int driver_open(struct inode *device_file, struct file *instance){
	printk(KERN_INFO "lcd - The lcd device is opened\n");
	return 0;
}

/**
 * @brief This function is called when the device is closed
 */
static int driver_close(struct inode *device_file, struct file *instance){
	printk(KERN_INFO "lcd - The lcd device is closed\n");
	return 0;
}

/**
 * @brief This function is called when user want to read data
 * Read data to buffer
 */
static ssize_t driver_read(struct file *File, char *usr_buffer, size_t count, loff_t *offset){
	int amount, cp, del;

	/*Get amount of data to copy*/
	amount = min(count, buff_p);
	if (DEBUG){
		printk ("Amount of data to read: %d\n", amount);
	}

	/*Copy data to user*/
	cp = copy_to_user(usr_buffer, buffer, amount);
	if (DEBUG){
		printk ("usr_buffer: %s\n", usr_buffer);
	}

	/*Caculate data*/
	del = amount - cp;
	if (DEBUG){
		printk ("delta of read: %d\n", del);
	}

	return del;
}

/**
 * @brief This function is called when user want to write data
 * Write data to buffer
 */
static ssize_t driver_write(struct file *File, const char *usr_buffer, size_t count, loff_t *offset) {
    int amount, cp, del;

    /* Get the amount of data to copy */
    amount = min(count, sizeof(buffer));

    /* Copy data from user space */
    cp = copy_from_user(buffer, usr_buffer, amount);
    if (cp) {
        printk(KERN_ERR "Failed to copy data from user space\n");
        return -EFAULT;
    }
    buff_p = amount - cp;

    if (strlen(buffer) < 31) {  // Check if the string length is less than 31 characters
        /* Print string on LCD */
        lcd_print(buffer);
        printk(KERN_INFO "lcd - LCD Display: %s\n", buffer);
    } else {
        printk(KERN_INFO "lcd - The size of string is out of range: %s: %d\n", buffer, strlen(buffer));
    }


	del = amount - cp;
    return del;
}


static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
	.write = driver_write
};

/**
 * @brief This function is called when the driver is loaded into kernel
 */
static int __init ModuleInit(void) {
    printk(KERN_INFO "Hello, this is lcd driver\n");

    // Allocate a device number
    if (alloc_chrdev_region(&device_nr, 0, 1, DRIVER_NAME) < 0) {
        printk(KERN_ERR "Could not allocate device number\n");
        return -1;
    }
    printk(KERN_INFO "Device %s was registered with Major: %d, Minor: %d\n", DRIVER_NAME, MAJOR(device_nr), MINOR(device_nr));

    // Create device class
    if ((my_class = class_create(DRIVER_CLASS)) == NULL) {
        printk(KERN_ERR "Device class cannot be created!\n");
        goto classError;
    }

    // Create device file
    if (device_create(my_class, NULL, device_nr, NULL, DRIVER_NAME) == NULL) {
        printk(KERN_ERR "Device file cannot be created!\n");
        goto fileError;
    }

    // Initialize device file
    cdev_init(&my_device, &fops);

    // Register device to kernel
    if (cdev_add(&my_device, device_nr, 1) == -1) {
        printk(KERN_ERR "Register device to kernel failed!\n");
        goto addError;
    }

    // Init i2c
    lcd_adapter = i2c_get_adapter(1); // use bus i2c number 1
    if (IS_ERR(lcd_adapter)) {
        printk(KERN_ERR "Failed to get I2C adapter\n");
        goto addError;
    }

    lcd_client = i2c_new_dummy_device(lcd_adapter, I2C_ADDR);
    if (IS_ERR(lcd_client)) {
        printk(KERN_ERR "Failed to create I2C client\n");
        goto adapterError;
    }

    // init LCD
    lcd_init();

    printk(KERN_INFO "Character driver with LCD support loaded successfully\n");
    return 0;

adapterError:
    i2c_put_adapter(lcd_adapter);
addError:
    device_destroy(my_class, device_nr);
fileError:
    class_destroy(my_class);
classError:
    unregister_chrdev_region(device_nr, 1);
    return -1;
}


/**
 * @brief This function is called when the driver is removed from kernel
 */
static void __exit ModuleExit(void) {
    printk(KERN_INFO "Goodbye kernel!\n");

    // Giải phóng tài nguyên I2C
    i2c_unregister_device(lcd_client);
    i2c_put_adapter(lcd_adapter);

    cdev_del(&my_device);
    device_destroy(my_class, device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(device_nr, 1);
}


module_init(ModuleInit);
module_exit(ModuleExit);
