/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("hoikeung"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    struct aesd_dev *dev = filp->private_data;
    
    if (mutex_lock_interruptible(&dev->lock)) 
    {
    	return -ERESTARTSYS;
    }

    if (*f_pos < dev->count) 
    {
        PDEBUG("*f_pos = %lld; dev->count = %d", *f_pos, dev->count);
        
        int index = (dev->head + *f_pos) % BUFFER_SIZE;
        struct entry read_entry = dev->buffer[index];

        unsigned long copy = count;
        
        if (read_entry.size < count) 
        {
            copy = read_entry.size;
        }
        retval = copy;
        
        do 
        {
            copy = copy_to_user(buf, read_entry.ptr, copy);
        } while (copy);

        *f_pos = *f_pos + 1;
    }
    mutex_unlock(&dev->lock);
    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */

    struct aesd_dev *dev = filp->private_data;

    if (mutex_lock_interruptible(&dev->lock)) 
    {
        return -ERESTARTSYS;
    }

    dev->entry_buffer.ptr = krealloc(dev->entry_buffer.ptr, dev->entry_buffer.size + count, GFP_KERNEL);
    if (dev->entry_buffer.ptr == NULL) 
    {
        PDEBUG("Failed to allocate buffer");
        return -ENOMEM;
    }
    dev->entry_buffer.size += count;

    unsigned long copy = count;
    int idx;
    while (copy) 
    {
        idx = dev->entry_buffer.size - copy;
        copy = copy_from_user(dev->entry_buffer.ptr + idx, buf, copy);
    }
    retval = count;

    if (dev->entry_buffer.ptr[dev->entry_buffer.size - 1] == '\n') 
    {
        if (dev->count == BUFFER_SIZE) 
        {
            kfree(dev->buffer[dev->write_pos].ptr);
            dev->head = (dev->head + 1) % BUFFER_SIZE;
        }
        dev->buffer[dev->write_pos].ptr = dev->entry_buffer.ptr;
        dev->buffer[dev->write_pos].size = dev->entry_buffer.size;
        dev->write_pos = (dev->write_pos + 1) % BUFFER_SIZE;

        if (dev->count < BUFFER_SIZE) 
        {
            dev->count++;
        }
        dev->entry_buffer.ptr = NULL;
        dev->entry_buffer.size = 0;
    }
    mutex_unlock(&dev->lock);

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * TODO: initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.lock);
    aesd_device.count = 0;
    aesd_device.write_pos = 0;
    aesd_device.head = 0;
    aesd_device.entry_buffer.ptr = NULL;
    aesd_device.entry_buffer.size = 0;

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    for (int i = 0; i < BUFFER_SIZE; i++) 
    {
        if (aesd_device.buffer[i].ptr != NULL) 
        {
            kfree(aesd_device.buffer[i].ptr);
        }
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
