/*
 * File : akashi_reg.c
 * Created : April 2022
 * Authors : Martin Siu
 * Synopsis: Akashi Register Interface device driver.
 *
 * Copyright 2022 Audinate Pty Ltd and/or its licensors
 *
 */

/*
 * Change History:
 *  4.0.1: Get the FPGA space size from DTS
 *  4.0.0: Driver support for 5.X kernel and 4.X backward compatibiilty
 *  3.2.0: Add SPI slave event for interfacing with host processor.
 *  3.1.0: Add 64-bit architecture support.
 *  3.0.0: Unified akashi register driver for all Zynq products.
 */

#include <asm/pgalloc.h>
#include <asm/uaccess.h>
#include <generated/autoconf.h>
#ifdef CONFIG_AKASHI_64BIT_ARCH
#include <linux/arm-smccc.h>
#endif
#include <linux/if_ether.h>
#include <linux/inetdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ip.h>
#include <linux/irqdomain.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/udp.h>
#include <linux/version.h>
#include <uapi/linux/sockios.h>

#define DRIVER_AUTHOR                    "Audinate Pty Ltd <oem-support@audinate.com>"
#define DRIVER_NAME                      "Dante Akashi Register driver"
#define DRIVER_VERSION                   "4.0.1"
#define DRIVER_LICENSE                   "GPL"

#ifdef SYDREG_DEBUG
#define SYDREG_PRINT(x...) printk(x)
#else
#define SYDREG_PRINT(x...)
#endif

typedef struct akashi_reg
{
    /* IRQ flags */
    int     ape_flow_irq;
    int     ape_act_irq;
    int     ape_ka_miss_irq;
    int     metering_irq;
    int     ape_ka_irq;
    int     spi_hp_irq;
    /* Device major number */
    int     dev_major;
} akashi_reg_t;

#define VM_RESERVED        0 //!!!
static uint8_t *virtual_base_addr;
static resource_size_t physical_base_addr;
static unsigned long remap_size;
#ifdef CONFIG_AKASHI_REG_SPI_HP
static uint8_t *spi_slave_virtual_base_addr;
#endif

//=====================================================================
// DEFINES
//=====================================================================

#include "akashi_reg.h"

#define DRIVER_NAME "Dante Akashi Register driver"
#define DANTEREG_MAJOR 243

#ifdef CONFIG_AKASHI_REG_SPI_HP
// SPI Interface write event for host processor
#define DANTEREG_SPI_HP_WRITE_MINOR 246
#endif
#ifdef CONFIG_AKASHI_REG_METERINGEV
    #define DANTEREG_METER_MINOR 249
#endif
#ifdef CONFIG_AKASHI_REG_KAEV
    #define DANTEREG_KAEV_MINOR 251
#endif
#ifdef CONFIG_AKASHI_REG_AERR
    #define DANTEREG_AERR_MINOR 254
#endif
#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
    #define DANTEREG_RX_BUNDLE_ACTIVITY_MINOR 247
#endif

#ifdef CONFIG_AKASHI_REG_KA_MISS
    #define DANTEREG_KA_MISS_MINOR 248
#endif

#define DANTEREG_NAME "syd_reg"

#define KA_MISS_DUMMY_BYTE 0x4B
#define RX_BUNDLE_ACTIVITY_DUMMY_BYTE 0x52

#define XPS_DEV_CFG_APB_BASEADDR     (0xf8007000)
#define XDCFG_CTRL_OFFSET            (0x0)
#define XDCFG_CTRL_PCAP_PR_MASK      (1 << 27)

#define SYD_REG_PROC_ROOT_NAME        ("syd_reg")

/* Match table for of_platform binding */
static const struct of_device_id akashi_reg_of_match[] =
{
    { .compatible = "xlnx,syd_reg", },
    {}
};
MODULE_DEVICE_TABLE(of, akashi_reg_of_match);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_NAME);
MODULE_LICENSE(DRIVER_LICENSE);
MODULE_VERSION(DRIVER_VERSION);
/* To avoid tainting the kernel on module loading */
MODULE_INFO(intree, "Y");

//=====================================================================
// LOCAL FUNCTION DECLARATIONS
//=====================================================================
static int akashi_reg_mmap(struct file *file, struct vm_area_struct * vma);
static int akashi_reg_open (struct inode *inode, struct file *file);
static int akashi_reg_release (struct inode *inode, struct file *file);
static ssize_t akashi_reg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static unsigned int akashi_reg_poll(struct file *filp, poll_table *wait);

//=====================================================================
// TYPES
//=====================================================================
#ifdef CONFIG_AKASHI_REG_KAEV
typedef struct ka_info_s
{
    unsigned char   data[50];
    int             data_len;
}ka_info_t;

typedef struct
{
    ka_info_t   item[KA_INFO_QUEUE_SIZE];
    int         head; // write pt
    int         tail; // read pt
}ka_info_queue_t;
#endif /* CONFIG_AKASHI_REG_KAEV */

//=====================================================================
// LOCAL VARIABLES
//=====================================================================

#ifdef CONFIG_AKASHI_REG_AERR
static DECLARE_WAIT_QUEUE_HEAD(aerrqueue);
static int pending_aerr;
#endif /*CONFIG_AKASHI_REG_AERR */

#ifdef CONFIG_AKASHI_REG_KAEV
// ka events
static ka_info_queue_t ka_info_queue;
static DECLARE_WAIT_QUEUE_HEAD(in_queue);
#endif /* CONFIG_AKASHI_REG_KAEV */

#ifdef CONFIG_AKASHI_REG_METERINGEV
static DECLARE_WAIT_QUEUE_HEAD(meterqueue);
static int pending_meter;
#endif /* CONFIG_AKASHI_REG_METERINGEV */

#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
static DECLARE_WAIT_QUEUE_HEAD(rx_bundle_activity_queue);
static uint32_t volatile pending_rx_bundle_activity;
#endif /* CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY */

#ifdef CONFIG_AKASHI_REG_KA_MISS
static DECLARE_WAIT_QUEUE_HEAD(ka_miss_queue);
static uint32_t volatile pending_ka_miss;
#endif /* CONFIG_AKASHI_REG_KA_MISS */

#ifdef CONFIG_AKASHI_REG_SPI_HP
// SPI Interface write event
static DECLARE_WAIT_QUEUE_HEAD(spi_hp_write_queue);
static uint32_t volatile pending_spi_hp_write;
#endif /* CONFIG_AKASHI_REG_SPI_HP */

// define which file operations are supported
struct file_operations akashi_reg_fops =
{
    .owner    = THIS_MODULE,
    .mmap     = akashi_reg_mmap,
    .open     = akashi_reg_open,
    .read     = akashi_reg_read,
    .release  = akashi_reg_release,
    .poll     = akashi_reg_poll,
};

static struct proc_dir_entry *proc_root;
static struct proc_dir_entry *proc_device_id;

static u32 akashi_reg_read_device_id(void);

static int akashi_reg_proc_device_id_show(struct seq_file *m, void *v)
{
    u32 dev_id = akashi_reg_read_device_id();

    seq_printf(m, "%08x\n", dev_id);

    return 0;
}

static int akashi_reg_proc_device_id_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,17,0)
    return single_open(file, akashi_reg_proc_device_id_show, PDE_DATA(inode));
#else
    return single_open(file, akashi_reg_proc_device_id_show, pde_data(inode));
#endif
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
static const struct file_operations proc_device_id_fops =
{
    .owner      = THIS_MODULE,
    .open       = akashi_reg_proc_device_id_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};
#else
static const struct proc_ops proc_device_id_fops = {
    .proc_open       = akashi_reg_proc_device_id_open,
    .proc_read       = seq_read,
    .proc_lseek     = seq_lseek,
    .proc_release    = single_release,
};
#endif

static void akashi_reg_create_proc_entries(void)
{
    char *entry_name;

    proc_root = proc_mkdir(SYD_REG_PROC_ROOT_NAME, NULL);
    if (proc_root)
    {
        entry_name = "device_id";
        proc_device_id = proc_create_data(entry_name, 0, proc_root, &proc_device_id_fops, NULL);
        if (!proc_device_id)
        {
            printk(KERN_ERR "Error: failed to create /proc/%s/%s\n", SYD_REG_PROC_ROOT_NAME, entry_name);
        }
    }
    else
    {
        printk(KERN_ERR "Error: failed to create /proc/%s\n", SYD_REG_PROC_ROOT_NAME);
    }
}

static void akashi_reg_destroy_proc_entries(void)
{
    if (proc_root)
    {
        if (proc_device_id)
        {
            proc_remove(proc_device_id);
            proc_device_id = NULL;
        }

        proc_remove(proc_root);
        proc_root = NULL;
    }
}

#ifdef CONFIG_AKASHI_REG_KAEV
//=====================================================================
// KEEP ALIVE INFO QUEUE FUNCTIONS
//=====================================================================
//-------------------------------------------------------------------
// Init circular queue
static void ka_evt_queue_init(void)
{
    ka_info_queue.head = 0;
    ka_info_queue.tail = 0;
}

//-------------------------------------------------------------------
// Get item count from circular queue
static int ka_evt_queue_get_count(void)
{
    /*SYDREG_PRINT("SYD_REG: ka_evt_queue_get_count() ka_info_queue.head=%d, ka_info_queue.tail=%d\n",
      ka_info_queue.head, ka_info_queue.tail);*/
    if (ka_info_queue.head >= ka_info_queue.tail)
    {
        return ka_info_queue.head - ka_info_queue.tail;
    }
    else
    {
        int queue_size = sizeof(ka_info_queue.item) / sizeof(ka_info_queue.item[0]);
        return (queue_size - ka_info_queue.tail) + ka_info_queue.head;
    }
}

//-------------------------------------------------------------------
// Instead of push item, get head for adding new item and increment head; avoid memcpy.
static void ka_evt_queue_inc_head(void)
{
    int queue_size = sizeof(ka_info_queue.item) / sizeof(ka_info_queue.item[0]);

    ka_info_queue.head++;
    if (ka_info_queue.head >= queue_size)
    {
        ka_info_queue.head = 0;
    }
}

static void ka_evt_wakeup(void)
{
    wake_up_interruptible(&in_queue);
}

#endif /* CONFIG_AKASHI_REG_KAEV */

//=====================================================================
// Interrupt handlers
//=====================================================================

#ifdef CONFIG_AKASHI_REG_METERINGEV

static void meter_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{
    // Read status & if it is 1, then give event to app.
    if(AUD_METERING_INT)
    {
        //Tell Userland
        pending_meter = 1;
        wake_up_interruptible(&meterqueue);
    }
}

static irqreturn_t meter_interrupt(int irq, void *dev_id)
{

    meter_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif

#ifdef CONFIG_AKASHI_REG_AERR
static void aerr_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{

    //Tell Userland
    pending_aerr = 1;
    wake_up_interruptible(&aerrqueue);
}

static irqreturn_t aerr_interrupt(int irq, void *dev_id)
{
    //SYDREG_PRINT("\n\n %s: Got interrupt. \n\n ", __func__);
    aerr_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif


#ifdef CONFIG_AKASHI_REG_KAEV
static void akashi_reg_interrupt_set(int enable)
{
    if (enable)
    {
        AUD_SYD_SCHED_AUDIO_FIFO_CONTROL |= AUD_SYD_SCHED_RX_HEADER_AUDIO_INT_ENABLE;
    }
    else
    {
        AUD_SYD_SCHED_AUDIO_FIFO_CONTROL &= ~AUD_SYD_SCHED_RX_HEADER_AUDIO_INT_ENABLE;
    }
}

//-------------------------------------------------------------------
// K.A  Interrupt handler - CONFIG_DANTE_APE_O_KA_IRQ
// First audio packet is received. Send event to user-app.
//

static inline int audio_queued(void)
{
    return ((AUD_SYD_SCHED_FIFO_OCC & AUD_SYD_SCHED_FIFO_AUDIO_NOT_EMPTY_MASK));
}

static void ka_evt_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{
    int i;
    //SYDREG_PRINT("%s: enter... \n ", __func__);
    // disable intr.
    akashi_reg_interrupt_set(0);
    while(audio_queued())
    {
        ka_info_t *p_ka_info;
        // Insert received packet header to queue.
        p_ka_info = &ka_info_queue.item[ka_info_queue.head];
        p_ka_info->data_len = 0;
        for (i = 0; i < (RX_AUD_DATA_HDR_SIZE/4); i++)
        {
            uint32_t reg_read = AUD_SYD_SCHED_RX_AUDIO_FIFO;
            memcpy(&p_ka_info->data[p_ka_info->data_len], &reg_read, sizeof(reg_read));
            p_ka_info->data_len += sizeof(reg_read);
        }
        ka_evt_queue_inc_head();
    }
    ka_evt_wakeup();

    // enable intr.
    akashi_reg_interrupt_set(1);
}

static irqreturn_t ka_evt_interrupt(int irq, void *dev_id)
{
    ka_evt_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif /* CONFIG_AKASHI_REG_KAEV */

#ifdef CONFIG_AKASHI_REG_KA_MISS

static void ka_miss_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef CONFIG_SYDNEY_REGS
    uint32_t ka_miss_irq;

    // Read the register to explicitly clear the IRQ bit
    ka_miss_irq = AUD_SYD_SCHED_RX_KA_IRQ;
#endif

    pending_ka_miss = 1;
    wake_up_interruptible(&ka_miss_queue);
}

static irqreturn_t ka_miss_interrupt(int irq, void *dev_id)
{
    ka_miss_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif

#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY

static void rx_bundle_activity_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef CONFIG_SYDNEY_REGS
    uint32_t bundle_activity_irq;

    // Read the register to explicitly clear the IRQ bit
    bundle_activity_irq = AUD_SYD_SCHED_RX_BUNDLE_ACTIVITY_IRQ;
#endif

    pending_rx_bundle_activity = 1;
    wake_up_interruptible(&rx_bundle_activity_queue);
}

static irqreturn_t rx_bundle_activity_interrupt(int irq, void *dev_id)
{
    rx_bundle_activity_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif //CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY

#ifdef CONFIG_AKASHI_REG_SPI_HP
static void spi_hp_write_interrupt_core(int irq, void *dev_id, struct pt_regs *regs)
{
    // clear interrupt
    AKASHI_SPI_INT_STATUS_REG = 1;

    pending_spi_hp_write = 1;
    wake_up_interruptible(&spi_hp_write_queue);
}

static irqreturn_t spi_hp_write_interrupt(int irq, void *dev_id)
{
    spi_hp_write_interrupt_core(irq, dev_id, NULL);
    return IRQ_HANDLED;
}
#endif //CONFIG_AKASHI_REG_SPI_HP

//=====================================================================
// LOCAL FUNCTIONS
//=====================================================================

// open function - called when the "file" /dev/dreg is opened in userspace
static int akashi_reg_open(struct inode *inode, struct file *file)
{
    return 0;
}

// close function - called when the "file" /dev/dreg is closed in userspace
static int akashi_reg_release(struct inode *inode, struct file *file)
{
    return 0;
}

static int akashi_reg_mmap(struct file *file, struct vm_area_struct *vma)
{
    unsigned long vm_size;
    int ret;

    vm_size = vma->vm_end - vma->vm_start;

    /* execution from dreg is not permitted, and nor are private mappings */
    if (vma->vm_flags & VM_EXEC)
    {
        return -EINVAL;
    }

    if (!(vma->vm_flags & VM_SHARED))
    {
        return -EINVAL;
    }

    if ( (vm_size < PAGE_SIZE) ||
            (vm_size % PAGE_SIZE != 0) )
    {
        return -EINVAL;
    }

    /* we are not swappable and it's not represented by a struct page */
    vma->vm_flags |= VM_RESERVED | VM_IO;

    /*
     * DANTEIP-505:
     * This is critical to enable mmap attributes to non-cache so that user space I/O will have
     * no cache delays.
     */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    ret = vm_iomap_memory(vma, physical_base_addr, remap_size);
    if (ret)
    {
#ifdef CONFIG_AKASHI_64BIT_ARCH
        printk("vm_iomap_memory %llx size %ld - failed: %d\n",  physical_base_addr, remap_size, ret);
#else
        printk("vm_iomap_memory %x size %ld - failed: %d\n",  physical_base_addr, remap_size, ret);
#endif
        return -EAGAIN;
    }

    return 0;
}

#ifdef CONFIG_AKASHI_REG_KAEV

static unsigned int ka_evt_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &in_queue,  wait);

    if (ka_evt_queue_get_count() > 0)
    {
        mask = POLLIN | POLLRDNORM;
    }
    return mask;
}
#endif /* CONFIG_AKASHI_REG_KAEV */

#ifdef CONFIG_AKASHI_REG_AERR
static ssize_t aerr_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    static const char data[] = "r";
    int evt;
    u32 ret;

    evt = wait_event_interruptible(aerrqueue, pending_aerr);
    if(evt)
        return evt;

    ret = copy_to_user(buf, data, 1);

    pending_aerr = 0;

    return 1;
}

static unsigned int aerr_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    if(!pending_aerr)
        poll_wait(filp, &aerrqueue,  wait);

    if (pending_aerr)
        mask |= POLLIN | POLLRDNORM;    /* readable */
    return mask;
}
#endif /* CONFIG_AKASHI_REG_AERR */

#ifdef CONFIG_AKASHI_REG_METERINGEV
static ssize_t meter_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    static const char data[] = "m";
    u32 ret;

    ret = copy_to_user(buf, data, 1);

    pending_meter = 0;

    return 1;
}

static unsigned int meter_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    if (!pending_meter)
        poll_wait(filp, &meterqueue,  wait);

    if (pending_meter)
        mask |= POLLIN | POLLRDNORM;    /* readable */
    return mask;
}
#endif /* CONFIG_AKASHI_REG_METERINGEV */

#ifdef CONFIG_AKASHI_REG_SPI_HP
static ssize_t spi_hp_write_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    static const char data[] = "K";
    u32 ret;

    ret = copy_to_user(buf, data, 1);

    pending_spi_hp_write = 0;

    return 1;
}

static unsigned int spi_hp_write_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    if (!pending_spi_hp_write)
        poll_wait(filp, &spi_hp_write_queue,  wait);

    if (pending_spi_hp_write)
        mask |= POLLIN | POLLRDNORM;    /* readable */
    return mask;
}
#endif /* CONFIG_AKASHI_REG_SPI_HP */


#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
static ssize_t rx_bundle_activity_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    uint8_t const data = RX_BUNDLE_ACTIVITY_DUMMY_BYTE;
    int evt;
    u32 ret;

    evt = wait_event_interruptible(rx_bundle_activity_queue, (pending_rx_bundle_activity == 1));
    if (evt) {
        return evt;
    }

    ret = copy_to_user(buf, &data, sizeof(uint8_t));
    pending_rx_bundle_activity = 0;

    return sizeof(uint8_t);
}

static unsigned int rx_bundle_activity_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    if (pending_rx_bundle_activity == 0) {
        poll_wait(filp, &rx_bundle_activity_queue, wait);
    }

    if (pending_rx_bundle_activity != 0) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}
#endif /* CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY */

#ifdef CONFIG_AKASHI_REG_KA_MISS
static ssize_t ka_miss_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    uint8_t const data = KA_MISS_DUMMY_BYTE;
    int evt;
    u32 ret;

    evt = wait_event_interruptible(ka_miss_queue, (pending_ka_miss == 1));
    if (evt) {
        return evt;
    }

    ret = copy_to_user(buf, &data, sizeof(uint8_t));
    pending_ka_miss = 0;

    return sizeof(uint8_t);
}

static unsigned int ka_miss_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    if (pending_ka_miss == 0) {
        poll_wait(filp, &ka_miss_queue, wait);
    }

    if (pending_ka_miss != 0) {
        mask |= POLLIN | POLLRDNORM;
    }

    return mask;
}
#endif /* CONFIG_AKASHI_REG_KA_MISS */

static ssize_t akashi_reg_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{

    dev_t i_rdev = filp->f_inode->i_rdev;

#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
    if (MINOR(i_rdev) == DANTEREG_RX_BUNDLE_ACTIVITY_MINOR)
    {
        return rx_bundle_activity_read(filp, buf, count, f_pos);
    }
#endif /* CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY */
#ifdef CONFIG_AKASHI_REG_KA_MISS
    if (MINOR(i_rdev) == DANTEREG_KA_MISS_MINOR)
    {
        return ka_miss_read(filp, buf, count, f_pos);
    }
#endif /* CONFIG_AKASHI_REG_KA_MISS */
#ifdef CONFIG_AKASHI_REG_AERR
    if(MINOR(i_rdev) == DANTEREG_AERR_MINOR)
    {
        return aerr_read(filp, buf, count, f_pos);
    }
#endif /* CONFIG_AKASHI_REG_AERR */
#ifdef CONFIG_AKASHI_REG_METERINGEV
    if(MINOR(i_rdev) == DANTEREG_METER_MINOR)
    {
        return meter_read(filp, buf, count, f_pos);
    }
#endif /* CONFIG_AKASHI_REG_METERINGEV */
#ifdef CONFIG_AKASHI_REG_SPI_HP
    if(MINOR(i_rdev) == DANTEREG_SPI_HP_WRITE_MINOR)
    {
        return spi_hp_write_read(filp, buf, count, f_pos);
    }
#endif /* CONFIG_AKASHI_REG_SPI_HP */

    return -ENODEV;
}

static unsigned int akashi_reg_poll(struct file *filp, poll_table *wait)
{

    dev_t i_rdev = filp->f_inode->i_rdev;

#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
    if (MINOR(i_rdev) == DANTEREG_RX_BUNDLE_ACTIVITY_MINOR)
    {
        return rx_bundle_activity_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY */
#ifdef CONFIG_AKASHI_REG_KA_MISS
    if (MINOR(i_rdev) == DANTEREG_KA_MISS_MINOR)
    {
        return ka_miss_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_KA_MISS */
#ifdef CONFIG_AKASHI_REG_KAEV
    if(MINOR(i_rdev) == DANTEREG_KAEV_MINOR)
    {
        return ka_evt_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_KAEV */
#ifdef CONFIG_AKASHI_REG_AERR
    if(MINOR(i_rdev) == DANTEREG_AERR_MINOR)
    {
        return aerr_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_AERR */
#ifdef CONFIG_AKASHI_REG_METERINGEV
    if(MINOR(i_rdev) == DANTEREG_METER_MINOR)
    {
        return meter_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_METERINGEV */
#ifdef CONFIG_AKASHI_REG_SPI_HP
    if(MINOR(i_rdev) == DANTEREG_SPI_HP_WRITE_MINOR)
    {
        return spi_hp_write_poll(filp, wait);
    }
#endif /* CONFIG_AKASHI_REG_SPI_HP */
    return 0;
}

struct device_file_list{
    char name[25];
    unsigned int minor;
};

#define NUM_SYD_REG_DEV_FILES                14
struct device_file_list sydreg_dev_file[]={
        {"syd_reg0",                0},
        {"syd_reg1",                1},
        {"syd_reg2",                2},
        {"syd_reg3",                3},
        {"syd_reg4",                4},
        {"syd_hp_write",            246},
        {"syd_rx_bundle_activity",  247},
        {"syd_ka_miss",             248}, //1
        {"syd_meter_event",         249}, //1
        {"syd_rx_err",              250},
        {"syd_ka_event",            251},
        {"syd_sr_event",            252},
        {"syd_hm_event",            253},
        {"syd_a_err",               254} //1
};

struct class * sydreg_class;
dev_t sydreg_devt[NUM_SYD_REG_DEV_FILES];
struct device * sydreg_devfs[NUM_SYD_REG_DEV_FILES];


static int register_sydreg_char_device(akashi_reg_t *reg, const char *name,
        const struct file_operations *fops)
{
    int res;
    int i;

    /* create and register a cdev occupying a range of minors */
#ifdef CONFIG_AKASHI_REG_DYNAMIC_MAJOR
    reg->dev_major = 0;
    reg->dev_major  = register_chrdev(reg->dev_major, name, fops);
    if (reg->dev_major < 0)
    {
        printk(KERN_ERR "failed to register chrdev %s dynamic major number\n", name);
        return -EIO;
    }
#else
    reg->dev_major = DANTEREG_MAJOR;
    res  = register_chrdev(reg->dev_major, name, fops);
    if (res < 0)
    {
        printk(KERN_ERR "failed to register chrdev %s major %d\n", name, reg->dev_major);
        return -EIO;
    }
#endif

    printk(KERN_INFO "Registered char device %s, major %d\n", name, reg->dev_major);

    /* Create class */
    sydreg_class = class_create(THIS_MODULE, "syd-reg-class");
    if (IS_ERR(sydreg_class))
    {
        printk(KERN_ERR "Cannot create class");
        unregister_chrdev(reg->dev_major, name);
        return PTR_ERR(sydreg_class);
    }

    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
    for(i = 0; i< (NUM_SYD_REG_DEV_FILES); i++)
    {
        /* create /dev/syd_reg%d files */
        sydreg_devt[i] = MKDEV(reg->dev_major, sydreg_dev_file[i].minor);
        sydreg_devfs[i] = device_create(sydreg_class, NULL, sydreg_devt[i], NULL, sydreg_dev_file[i].name);
        res = PTR_ERR_OR_ZERO(sydreg_devfs[i]);
        if (res) {
            printk("%s: ERROR: Device '%s' created with minor %d failed. \n", __func__, \
                    dev_name(sydreg_devfs[i]), MINOR(sydreg_devt[i]));
            return res;
        }
    }
    return res;
}

static u32 akashi_reg_read_device_id(void)
{
    u32 device_id = 0;
#ifdef CONFIG_AKASHI_64BIT_ARCH

    u32 cmd;
    struct arm_smccc_res res;
    cmd = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, ARM_SMCCC_OWNER_SIP, 0x82000018);
    arm_smccc_smc(cmd, 0, 0, 0, 0, 0, 0, 0, &res);

    device_id = res.a0 >> 32;
#else
    u32 val;
    int retries;
    u32 dev_cfg_virt_addr;

    dev_cfg_virt_addr = (u32) ioremap(XPS_DEV_CFG_APB_BASEADDR, PAGE_SIZE);

    /* Select ICAP for reconfiguration */
    val = *((volatile u32 *)dev_cfg_virt_addr);
    val &= ~XDCFG_CTRL_PCAP_PR_MASK;
    *((volatile u32 *)dev_cfg_virt_addr) = val;

    /* Do a dummy read on response buffer */
    val = AUD_SYD_ICAP_DEVICE_ID_READ_CMD_RSP;

    /* Write value '1' to response buffer */
    AUD_SYD_ICAP_DEVICE_ID_READ_CMD_RSP = 1;

    val = 0;
    retries = 32;
    /* Keep polling until response value returned by FPGA becomes non-zero */
    while (!val)
    {
        val = AUD_SYD_ICAP_DEVICE_ID_READ_CMD_RSP;
        retries--;

        if (retries <= 0)
        {
            printk(KERN_ERR "%s: give up on tries\n", __func__);
            goto read_device_id_exit;
        }
    }

    /* Finally read the IDCODE from FPGA */
    device_id = AUD_SYD_INFO_DEVICE_IDCODE;

read_device_id_exit:
    iounmap((void *)dev_cfg_virt_addr);
#endif
    return device_id;
}

static __inline__ uint8_t akashi_reg_bcd_to_uint(uint8_t bcd)
{
    uint8_t hi, lo;
    uint8_t num;

    hi = (bcd & 0xf0) >> 4;
    if (hi > 9)
    {
        /* error case, set it to zero */
        hi = 0;
        printk(KERN_ERR "%s: invalid bcd 0x%x\n", __func__, bcd);
        return 0;
    }

    lo = bcd & 0x0f;
    if (lo > 9)
    {
        /* error case, set it to zero */
        lo = 0;
        printk(KERN_ERR "%s: invalid bcd 0x%x\n", __func__, bcd);
        return 0;
    }

    num = (hi * 10) + lo;
    return num;
}

static int akashi_reg_probe(struct platform_device *pdev)
{
    int retval;
    const struct of_device_id *match;
    struct resource *res;
    u8 *name;
    akashi_reg_t *reg;
#ifdef CONFIG_AKASHI_REG_SPI_HP
    int r;
    u32 spi_slave_hw_addr;
#endif

    match = of_match_node(akashi_reg_of_match, pdev->dev.of_node);
    if (!match)
    {
        printk(KERN_ERR "Failed to find matching akashi_reg from device tree\n");
        return -ENODEV;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res)
    {
        printk(KERN_ERR "Failed to find resource for akashi_reg\n");
        return -ENODEV;
    }

    /* Allocate our driver context */
    reg = kmalloc(sizeof(akashi_reg_t), GFP_KERNEL);
    if (!reg)
    {
        printk(KERN_ERR "Failed to allocate driver context\n");
        return -ENOMEM;
    }
    memset(reg, 0, sizeof(akashi_reg_t));
    /* Save it onto the platform private data for later use */
    platform_set_drvdata(pdev, reg);

    physical_base_addr = res->start;
    remap_size = res->end - res->start + 1;
    virtual_base_addr = (uint8_t *) ioremap(physical_base_addr, remap_size);
#ifdef CONFIG_AKASHI_64BIT_ARCH
    printk(KERN_INFO "Resource address: 0x%016llx, size: 0x%08lx\n", physical_base_addr, remap_size);
#else
    printk(KERN_INFO "Resource address: 0x%08x, size: 0x%08lx\n", physical_base_addr, remap_size);
#endif

    name = (u8*)(AUD_SYD_BASEADDR);
    printk(KERN_INFO "Platform Name <%c%c%c%c>, FPGA version <%d.%d.%d-rc%d>", \
            name[3],name[2],name[1],name[0], \
            akashi_reg_bcd_to_uint(AUD_SYD_FPGA_VERSION_MAJOR),
            akashi_reg_bcd_to_uint(AUD_SYD_FPGA_VERSION_MINOR),
            akashi_reg_bcd_to_uint(AUD_SYD_FPGA_VERSION_PATCH),
            akashi_reg_bcd_to_uint(AUD_SYD_FPGA_VERSION_RC));

    retval = register_sydreg_char_device(reg, DANTEREG_NAME, &akashi_reg_fops);
    if (retval != 0)
    {
        return -EIO;
    }

#ifdef CONFIG_AKASHI_REG_KAEV
    // Initialise ka events
    ka_evt_queue_init();
    reg->ape_ka_irq = platform_get_irq_byname(pdev, "daif");
    if (reg->ape_ka_irq <= 0)
    {
        printk(KERN_ERR "%s: failed to get platform irq for APE KA\n", __func__);
        return -ENXIO;
    }

    printk(KERN_INFO "%s: request KA_EVT irq %d\n", __func__, reg->ape_ka_irq);
    retval = request_irq(reg->ape_ka_irq, &ka_evt_interrupt, 0, "KA Event", NULL);
    if (retval != 0)
    {
        printk(KERN_ALERT "KA_EVT: Could not register interrupt %d.\n", reg->ape_ka_irq);
        return retval;
    }

    AUD_SYD_APE_CONTROL |= AUD_SYD_APE_INT_ENABLE;
#endif //CONFIG_AKASHI_REG_KAEV

#ifdef CONFIG_AKASHI_REG_AERR
    reg->ape_flow_irq = platform_get_irq_byname(pdev, "ape");
    if (reg->ape_flow_irq <= 0)
    {
        printk(KERN_ERR "%s: failed to get platform irq for APE FLOW\n", __func__);
        return -ENXIO;
    }

    printk(KERN_INFO "%s: request AERR irq %d\n", __func__, reg->ape_flow_irq);
    retval = request_irq(reg->ape_flow_irq, &aerr_interrupt, 0, "Audio Err", NULL);
    if (retval != 0)
    {
        printk(KERN_ALERT "AERR: Could not register interrupt %d.\n", reg->ape_flow_irq);
        return retval;
    }
    pending_aerr = 0;
#endif //CONFIG_AKASHI_REG_AERR

#ifdef CONFIG_AKASHI_REG_METERINGEV
    reg->metering_irq = platform_get_irq_byname(pdev, "meter");
    if (reg->metering_irq <= 0)
    {
        printk(KERN_ERR "%s: failed to get platform irq for METERING\n", __func__);
        return -ENXIO;
    }

    printk(KERN_INFO "%s: request Metering ERR irq %d\n", __func__, reg->metering_irq);
    retval = request_irq(reg->metering_irq, &meter_interrupt, 0, "Metering Ready", NULL);
    if (retval != 0)
    {
        printk(KERN_ALERT "Metering ERR: Could not register interrupt %d.\n", reg->metering_irq);
        return retval;
    }
    // Clear the interrupt has occured flag
    pending_meter = 0;
#endif //CONFIG_AKASHI_REG_METERINGEV

#ifdef CONFIG_AKASHI_REG_KAEV
    // Enable Audio interrupt and clear FIFO
    AUD_SYD_SCHED_AUDIO_FIFO_CONTROL = AUD_SYD_SCHED_RX_HEADER_AUDIO_INT_ENABLE | AUD_SYD_SCHED_RX_HEADER_AUDIO_FIFO_CLEAR;
#endif //CONFIG_AKASHI_REG_KAEV

#ifdef CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY
    reg->ape_act_irq = platform_get_irq_byname(pdev, "rx_act");
    if (reg->ape_act_irq <= 0)
    {
        printk(KERN_ERR "%s: failed to get platform irq for APE ACT\n", __func__);
        return -ENXIO;
    }

    printk(KERN_INFO "%s: request Rx Bundle Activity irq %d\n", __func__, reg->ape_act_irq);

    retval = request_irq(reg->ape_act_irq, &rx_bundle_activity_interrupt, 0, "Rx Bundle Activity", NULL);
    if (retval != 0)
    {
        printk(KERN_ALERT "Rx Bundle Activity: Could not register interrupt %d.\n", reg->ape_act_irq);
        return retval;
    }

    AUD_SYD_SCHED_RX_BUNDLE_ACTIVITY_IRQ_MASK |= AUD_SYD_SCHED_RX_BUNDLE_ACTIVITY_IRQ_ENABLE;

    pending_rx_bundle_activity = 0;
#endif //CONFIG_AKASHI_REG_RX_BUNDLE_ACTIVITY

#ifdef CONFIG_AKASHI_REG_KA_MISS
    reg->ape_ka_miss_irq = platform_get_irq_byname(pdev, "rx_ka");
    if (reg->ape_ka_miss_irq <= 0)
    {
        printk(KERN_ERR "%s: failed to get platform irq for APE KA MISS\n", __func__);
        return -ENXIO;
    }

    printk(KERN_INFO "%s: request KA Miss irq %d\n", __func__, reg->ape_ka_miss_irq);
    retval = request_irq(reg->ape_ka_miss_irq, &ka_miss_interrupt, 0, "KA Miss", NULL);
    if (retval != 0)
    {
        printk(KERN_ALERT "KA Miss: Could not register interrupt %d.\n", reg->ape_ka_miss_irq);
        return retval;
    }

    AUD_SYD_SCHED_RX_KA_IRQ_MASK |= AUD_SYD_SCHED_RX_KA_MISS_IRQ_ENABLE;
    pending_ka_miss = 0;
#endif //CONFIG_AKASHI_REG_KA_MISS

#ifdef CONFIG_AKASHI_REG_SPI_HP
    r = of_property_read_variable_u32_array(pdev->dev.of_node,
                    "spi-slave-addr", &spi_slave_hw_addr, 1, 1);
    if (r == 1)
    {
        spi_slave_virtual_base_addr = (uint8_t *) ioremap(spi_slave_hw_addr, 0x10);
        printk("spi_slave_hw_addr 0x%08x -> 0x%08x\n", (u32)spi_slave_hw_addr, (u32)spi_slave_virtual_base_addr);

        reg->spi_hp_irq = platform_get_irq_byname(pdev, "hp_write");
        if (reg->spi_hp_irq <= 0)
        {
            printk(KERN_ERR "%s: failed to get platform irq for SPI HP write\n", __func__);
            return -ENXIO;
        }

        printk(KERN_INFO "%s: request SPI HP write irq %d\n", __func__, reg->spi_hp_irq);
        retval = request_irq(reg->spi_hp_irq, &spi_hp_write_interrupt, 0, "HP write", NULL);
        if (retval != 0)
        {
            printk(KERN_ALERT "SPI HP write: Could not register interrupt %d.\n", reg->spi_hp_irq);
            return retval;
        }

        // enabling spi HP write interrupt
        AKASHI_SPI_INT_MASK_REG = 1;
    }
    else
    {
        printk(KERN_ALERT "SPI slave address not available\n");
    }
#endif // CONFIG_AKASHI_REG_SPI_HP

    /* Read device ID at least once on startup */
    printk(KERN_INFO "Device ID: %08x\n", akashi_reg_read_device_id());

    /* Create proc entry */
    akashi_reg_create_proc_entries();

    return 0;
}

static void unregister_sydreg_char_device(akashi_reg_t *reg, const char *name)
{
    int i;

    /* Remove proc entry */
    akashi_reg_destroy_proc_entries();

    for(i = 0; i < (NUM_SYD_REG_DEV_FILES); i++)
    {
        device_destroy(sydreg_class, sydreg_devt[i]);
    }

    class_destroy(sydreg_class);
    unregister_chrdev(reg->dev_major, name);
}

static int akashi_reg_remove(struct platform_device *pdev)
{
    akashi_reg_t *reg = platform_get_drvdata(pdev);

    if (reg->ape_act_irq > 0)
        free_irq(reg->ape_act_irq, NULL);

    if (reg->metering_irq > 0)
        free_irq(reg->metering_irq, NULL);

    if (reg->ape_ka_irq > 0)
        free_irq(reg->ape_ka_irq, NULL);

    if (reg->ape_ka_miss_irq > 0)
        free_irq(reg->ape_ka_miss_irq, NULL);

    if (reg->ape_flow_irq > 0)
        free_irq(reg->ape_flow_irq, NULL);

    if (reg->spi_hp_irq > 0)
        free_irq(reg->spi_hp_irq, NULL);

    unregister_sydreg_char_device(reg, DANTEREG_NAME);
    iounmap(virtual_base_addr);
#ifdef CONFIG_AKASHI_REG_SPI_HP
    iounmap(spi_slave_virtual_base_addr);
#endif

    /* Free up our driver context */
    kfree(reg);

    return 0;
}

static struct platform_driver akashi_reg_platform_driver =
{
    .probe   = akashi_reg_probe,
    .remove  = akashi_reg_remove,
    .driver  =
    {
        .name = "syd_reg",
        .of_match_table = akashi_reg_of_match,
    },
};

static int __init akashi_reg_init (void)
{
    int retval = 0;

    printk(KERN_INFO "%s, driver version %s\n", DRIVER_NAME, DRIVER_VERSION);
    printk(KERN_INFO "%s\n", DRIVER_AUTHOR);

    /* Register the platform driver */
    retval = platform_driver_register(&akashi_reg_platform_driver);
    if (retval)
    {
        printk(KERN_ERR "Failed to register akashi_reg platform driver\n");
    }

    return retval;
}

static void __exit akashi_reg_cleanup(void)
{
    /* Unregister the platform driver */
    platform_driver_unregister(&akashi_reg_platform_driver);
}

//-------------------------------------------------------------------
// Module declarations
module_init(akashi_reg_init);
module_exit(akashi_reg_cleanup);
