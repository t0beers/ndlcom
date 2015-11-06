#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/device.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#include "../../ndlcom/ndlcom.sdk/fsbl_bsp/ps7_cortexa9_0/include/xparameters.h"

#define DEBUG
#ifdef DEBUG
#    define PDEBUG(fmt, args...) printk(KERN_INFO "ndlcom: " fmt, ## args)
#else
#    define PDEBUG(fmt, args...)
#endif

MODULE_AUTHOR("Tobix");
MODULE_LICENSE("GPL");

#define NDLC_IRQ 61 // TODO: use define from xparameters.h

// address returned by ioremap()
static void *fpga_mem_base = 0;

// Major device number (TODO: dynamic device number)
int dev_major = 124;
struct cdev *ndlc_cdev = NULL;

// set to automatically create entry "/dev/NDLCom", spares us the "mknod" step
static struct class* ndlc_class = NULL;
static struct device* ndlc_device = NULL;

// Wait queues for read and write
static DECLARE_WAIT_QUEUE_HEAD(read_wait_queue);

// a linked-list for multiple receive buffers
#define RECV_BUF_SIZE 4096
struct recv_buf_t {
    char data[RECV_BUF_SIZE];
    size_t wpos;
    size_t rpos;

    struct recv_buf_t *next;
};

static struct recv_buf_t *recv_buf_head = NULL;

static struct file* exclusive_writer = NULL;

ssize_t ndlcom_read(struct file *fp, char __user *buf, size_t buf_size, loff_t *f_pos)
{
    // TODO: implement O_NDELAY, use "fp->f_flags"

    struct recv_buf_t *recv_buf = fp->private_data;

    // when we are asked to do "non-blocking read" return EAGAIN if there is no data
    if (fp->f_flags & O_NDELAY) {
        if (recv_buf->wpos == recv_buf->rpos) {
            return -EAGAIN;
        }
    }

    // wait for data if buffer is empty
    while (recv_buf->wpos == recv_buf->rpos) {
        if (wait_event_interruptible(read_wait_queue, recv_buf->wpos != recv_buf->rpos)) {
            return -ERESTARTSYS;
        }
    }

    // TODO: copy more than one byte here...
    if (copy_to_user(buf,&recv_buf->data[recv_buf->rpos], 1)) {
        return -EFAULT;
    }

    // forward the read-pointer
    recv_buf->rpos = (recv_buf->rpos+1) % RECV_BUF_SIZE;

    return 1;
}

ssize_t ndlcom_write(struct file *fp, const char __user *buf, size_t count, loff_t *f_pos)
{
    // TODO: implement O_NDELAY, use "fp->f_flags"

    // there _should_ only be one fp with writable permission in userspace at
    // any given time, so we don't need locking.
    //
    // directly copying into the "fpga_mem_base" memory region of the fpga. the rest
    // be done automatically. no additional synchronization is needed. this
    // causes "write" to be called manymany times, but makes the vhdl-part much
    // simpler.
    if (copy_from_user(fpga_mem_base, buf, 1)) {
        return -EFAULT;
    }
    // TODO: add memory barrier here

    return 1;
}

int ndlcom_open(struct inode *inode, struct file *fp)
{
    // TODO: prevent writing from multiple interleaving writers -- only allow
    // one open with "WRITE" permission!

    struct recv_buf_t *act_recv_buf = recv_buf_head;
    struct recv_buf_t *new_recv_buf;

    if (((fp->f_flags & O_ACCMODE) == O_WRONLY) || ((fp->f_flags & O_ACCMODE) == O_RDWR)) {
        // only one single writer at a time
        if (exclusive_writer != NULL) {
            printk(KERN_ERR "ndlcom: cannot open, "
                            "0x%p already has exclusive write access\n", exclusive_writer);
            return -EPERM;
        }
        PDEBUG("new exclusive writer 0x%p\n", fp);
        exclusive_writer = fp;
    }

    if (((fp->f_flags & O_ACCMODE) == O_RDONLY) || ((fp->f_flags & O_ACCMODE) == O_RDWR)) {
        // malloc memory for the datastructure of the new receive buffer
        new_recv_buf = kmalloc(sizeof(struct recv_buf_t), GFP_KERNEL);
        if (!new_recv_buf) {
            printk(KERN_ERR "ndlcom: open can't kmalloc new_recv_buf\n");
            return -ENOMEM;
        }

        // store the pointer to the buffer into the private data field
        fp->private_data = new_recv_buf;

        PDEBUG("new_recv_buf: 0x%p\n", new_recv_buf);

        new_recv_buf->wpos = 0;
        new_recv_buf->rpos = 0;
        new_recv_buf->next = NULL;

	// put the new "new_recv_buf" into the linked-list containing all known
	// receive buffers
        if (recv_buf_head == NULL) {
            recv_buf_head = new_recv_buf;
        } else {
            while (act_recv_buf->next != NULL)
                act_recv_buf = act_recv_buf->next;
            act_recv_buf->next = new_recv_buf;
        }

    } else {
        // somebody does not want to read, so we don't need a "recv_buffer" here
        fp->private_data = NULL;
    }

    PDEBUG("opened with new fp 0x%p\n", fp);

    return 0;
}

int ndlcom_release(struct inode *inode, struct file *fp)
{
    struct recv_buf_t *act_recv_buf = recv_buf_head;
    struct recv_buf_t *old_recv_buf = fp->private_data;

    if (old_recv_buf) {
        // remove the old_recv_buf out of the receive buffer queue
        if (recv_buf_head == old_recv_buf) {
            recv_buf_head = recv_buf_head->next;
        } else {
            while (act_recv_buf->next != NULL) {
                if (act_recv_buf->next == old_recv_buf) {
                    act_recv_buf->next = act_recv_buf->next->next;
                    break;
                }
                act_recv_buf = act_recv_buf->next;
            }
        }

        PDEBUG("freeing 0x%p\n", old_recv_buf);
        // free the buffer
        kfree(old_recv_buf);
    }

    if (exclusive_writer == fp) {
        PDEBUG("no exclusive writer anymore\n");
        exclusive_writer = NULL;
    }

    PDEBUG("release called for 0x%p\n", fp);

    return 0;
}

irqreturn_t irq_handler(int irq, void * dev_id)
{
    // copying the data from fpga-memory will automatically clear the pending
    // interrupt which caused us to be triggered
    uint8_t data = (uint8_t)(*((volatile uint32_t *)fpga_mem_base));
    // TODO: add memory barrier here

    // pointer to entry in linked-list of buffers
    struct recv_buf_t *act_recv_buf = recv_buf_head;

    printk(KERN_INFO "irq_handler read: 0x%2x\n",data);

    // save the incoming byte for all readers
    while (act_recv_buf != NULL) {

        act_recv_buf->data[act_recv_buf->wpos] = data;
        act_recv_buf->wpos = (act_recv_buf->wpos+1) % RECV_BUF_SIZE;

        // check if buffer is full
        if (act_recv_buf->wpos == act_recv_buf->rpos) {
             // the kernel will throw away the error-message if too many are printed
             printk_ratelimited(KERN_ERR "ndlcom: recv-buffer for 0x%p full"
                                         ", overwriting old data\n", act_recv_buf);
             act_recv_buf->rpos = (act_recv_buf->rpos+1) % RECV_BUF_SIZE;
         }

         // proceed with next receive buffer
         act_recv_buf = act_recv_buf->next;
    }

    // wake up waiting processes (just in case also if the buffer is full)
    wake_up_interruptible_all(&read_wait_queue);

    return IRQ_HANDLED;
}

// ----------------------------- module initialization and destruction -----------------------------

struct file_operations ndlcom_fops = {
    .owner   = THIS_MODULE,
    .read    = ndlcom_read,
    .write   = ndlcom_write,
    .open    = ndlcom_open,
    .release = ndlcom_release,
};

static int ndlcom_init(void)
{
    dev_t dev = 0;
    int ret;

    printk(KERN_INFO "ndlcom: init called.\n");

    recv_buf_head = NULL;
    exclusive_writer = NULL;

    // prepare entries in /dev
    dev = MKDEV(dev_major, 0);
    register_chrdev_region(dev, 1, "NDLCom");
    ndlc_cdev = cdev_alloc();
    cdev_init(ndlc_cdev, &ndlcom_fops);
    cdev_add(ndlc_cdev, dev, 1);

    // automatically create the "/dev/NDLCom" entry if the system comes with udev
    ndlc_class = class_create(THIS_MODULE, "NDLCom");
    if (IS_ERR(ndlc_class)) {
        printk(KERN_ERR "ndlcom: init failed to create class\n");

        unregister_chrdev_region(MKDEV(dev_major,0), 1);
        cdev_del(ndlc_cdev);
        return -ENODEV;
    }
    ndlc_device = device_create(ndlc_class, NULL, dev, NULL, "NDLCom");
    if (IS_ERR(ndlc_device)) {
        printk(KERN_ERR "ndlcom: init failed to create device\n");

        class_unregister(ndlc_class);
        class_destroy(ndlc_class);
        unregister_chrdev_region(MKDEV(dev_major,0), 1);
        cdev_del(ndlc_cdev);
        return -ENODEV;
    }

    // remap the cs address
    fpga_mem_base = ioremap(XPAR_NDLCOM_0_S00_AXI_BASEADDR,128);
    printk(KERN_INFO "ndlcom: fpga_mem_base: 0x%p\n",fpga_mem_base);

    // request irq
    irq_set_irq_type(NDLC_IRQ, IRQ_TYPE_LEVEL_HIGH);
    if ((ret = request_irq(NDLC_IRQ, irq_handler, 0, "ndlcom", 0))!=0) {
        printk(KERN_WARNING "ndlcom: request_irq failed %d)\n",ret);

        iounmap(fpga_mem_base);
        device_destroy(ndlc_class, MKDEV(dev_major, 0));
        class_unregister(ndlc_class);
        class_destroy(ndlc_class);
        unregister_chrdev_region(MKDEV(dev_major,0), 1);
        cdev_del(ndlc_cdev);
        return -ENODEV;
    }

    printk(KERN_INFO "ndlcom: interrupt is %d\n",NDLC_IRQ);

    printk(KERN_INFO "ndlcom: init succeeded\n");

    return 0;
}

static void ndlcom_exit(void)
{
    printk(KERN_INFO "ndlcom: exit\n");

    // free irq
    free_irq(NDLC_IRQ, 0);

    // unmap memory region
    iounmap(fpga_mem_base);

    // remove the "/dev/NDLCom" entry and cleanup
    device_destroy(ndlc_class, MKDEV(dev_major, 0));
    class_unregister(ndlc_class);
    class_destroy(ndlc_class);

    // remove entry in /dev
    unregister_chrdev_region(MKDEV(dev_major,0), 1);
    cdev_del(ndlc_cdev);
}

module_init(ndlcom_init);
module_exit(ndlcom_exit);
