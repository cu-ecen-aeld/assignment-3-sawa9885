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
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "aesdchar.h"
#include "aesd_ioctl.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Sawa9885");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    (void)inode;
    (void)filp;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    size_t copied = 0;
    ssize_t retval;

    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    while (copied < count) {
        struct aesd_buffer_entry *entry;
        size_t entry_offset;
        size_t bytes_to_copy;

        entry = aesd_circular_buffer_find_entry_offset_for_fpos(
            &dev->buffer, (size_t)*f_pos, &entry_offset);
        if (entry == NULL) {
            break;
        }

        bytes_to_copy = min(count - copied, entry->size - entry_offset);
        if (copy_to_user(buf + copied, entry->buffptr + entry_offset,
                         bytes_to_copy) != 0) {
            retval = copied == 0 ? -EFAULT : (ssize_t)copied;
            goto out;
        }
        copied += bytes_to_copy;
        *f_pos += bytes_to_copy;
    }

    retval = (ssize_t)copied;
out:
    mutex_unlock(&dev->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *commands = NULL;
    char *combined = NULL;
    char *new_pending = NULL;
    size_t combined_size;
    size_t command_count = 0;
    size_t command_index = 0;
    size_t start = 0;
    size_t index;
    ssize_t retval = -ENOMEM;

    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    if (count == 0) {
        return 0;
    }
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }
    if (count > SIZE_MAX - dev->pending_size) {
        retval = -EOVERFLOW;
        goto out;
    }

    combined_size = dev->pending_size + count;
    combined = kmalloc(combined_size, GFP_KERNEL);
    if (combined == NULL) {
        goto out;
    }
    if (dev->pending_size != 0) {
        memcpy(combined, dev->pending_write, dev->pending_size);
    }
    if (copy_from_user(combined + dev->pending_size, buf, count) != 0) {
        retval = -EFAULT;
        goto out;
    }

    for (index = 0; index < combined_size; index++) {
        if (combined[index] == '\n') {
            command_count++;
        }
    }
    if (command_count != 0) {
        commands = kcalloc(command_count, sizeof(*commands), GFP_KERNEL);
        if (commands == NULL) {
            goto out;
        }
    }

    for (index = 0; index < combined_size; index++) {
        if (combined[index] == '\n') {
            size_t command_size = index - start + 1;

            commands[command_index].buffptr = kmemdup(combined + start,
                                                       command_size,
                                                       GFP_KERNEL);
            if (commands[command_index].buffptr == NULL) {
                goto out;
            }
            commands[command_index].size = command_size;
            command_index++;
            start = index + 1;
        }
    }

    if (start < combined_size) {
        new_pending = kmemdup(combined + start, combined_size - start,
                              GFP_KERNEL);
        if (new_pending == NULL) {
            goto out;
        }
    }

    for (index = 0; index < command_count; index++) {
        const char *overwritten = NULL;

        if (dev->buffer.full) {
            overwritten = dev->buffer.entry[dev->buffer.in_offs].buffptr;
        }
        aesd_circular_buffer_add_entry(&dev->buffer, &commands[index]);
        kfree(overwritten);
        commands[index].buffptr = NULL;
    }

    kfree(dev->pending_write);
    dev->pending_write = new_pending;
    dev->pending_size = combined_size - start;
    new_pending = NULL;
    retval = (ssize_t)count;

out:
    for (index = 0; index < command_count; index++) {
        kfree(commands[index].buffptr);
    }
    kfree(commands);
    kfree(new_pending);
    kfree(combined);
    mutex_unlock(&dev->lock);
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry *entry;
    loff_t total_size = 0;
    loff_t new_position;
    uint8_t index;

    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &dev->buffer, index) {
        if (entry->size > (size_t)(LLONG_MAX - total_size)) {
            mutex_unlock(&dev->lock);
            return -EOVERFLOW;
        }
        total_size += entry->size;
    }

    switch (whence) {
    case SEEK_SET:
        new_position = offset;
        break;
    case SEEK_CUR:
        if ((offset > 0 && filp->f_pos > LLONG_MAX - offset) ||
            (offset < 0 && offset < -filp->f_pos)) {
            mutex_unlock(&dev->lock);
            return -EOVERFLOW;
        }
        new_position = filp->f_pos + offset;
        break;
    case SEEK_END:
        if ((offset > 0 && total_size > LLONG_MAX - offset) ||
            (offset < 0 && offset < -total_size)) {
            mutex_unlock(&dev->lock);
            return -EOVERFLOW;
        }
        new_position = total_size + offset;
        break;
    default:
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }

    if (new_position < 0 || new_position > total_size) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    filp->f_pos = new_position;
    mutex_unlock(&dev->lock);
    return new_position;
}

long aesd_unlocked_ioctl(struct file *filp, unsigned int cmd,
                         unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    uint8_t entry_count;
    uint8_t entry_index;
    uint32_t command;
    loff_t new_position = 0;
    long retval = 0;

    if (cmd != AESDCHAR_IOCSEEKTO) {
        return -ENOTTY;
    }
    if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto))) {
        return -EFAULT;
    }
    if (mutex_lock_interruptible(&dev->lock)) {
        return -ERESTARTSYS;
    }

    entry_count = dev->buffer.full ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED
                                   : dev->buffer.in_offs;
    if (seekto.write_cmd >= entry_count) {
        retval = -EINVAL;
        goto out;
    }

    entry_index = dev->buffer.out_offs;
    for (command = 0; command < seekto.write_cmd; command++) {
        new_position += dev->buffer.entry[entry_index].size;
        entry_index = (entry_index + 1) %
                      AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }
    if (seekto.write_cmd_offset >= dev->buffer.entry[entry_index].size) {
        retval = -EINVAL;
        goto out;
    }
    new_position += seekto.write_cmd_offset;
    filp->f_pos = new_position;

out:
    mutex_unlock(&dev->lock);
    return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_unlocked_ioctl,
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
    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    struct aesd_buffer_entry *entry;
    uint8_t index;

    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index) {
        kfree(entry->buffptr);
    }
    kfree(aesd_device.pending_write);
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
