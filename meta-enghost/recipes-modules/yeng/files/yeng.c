#include <linux/cdev.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#define DRIVER_NAME "yeng"
#define CDEV_NAME_PREFIX "xengstream"
#define MAX_DEVICES 16

static DEFINE_IDA(yeng_ida);
static DEFINE_MUTEX(yeng_ida_lock);

static struct class *yeng_class;
static dev_t yeng_cdev_first;

struct yeng_device {
	struct cdev cdev;
	struct device *dev;
	void __iomem *iomem;
	dev_t cdev_id;
};


static ssize_t yeng_cdev_read(struct file *file, char __user *buf, size_t count, loff_t *fpos) {
	struct yeng_device *yeng;

	yeng = file->private_data;
	dev_info(yeng->dev, "read count=%d", count);
	return count;
}

static ssize_t yeng_cdev_write(struct file *file, const char __user *buf, size_t count, loff_t *fpos) {
	struct yeng_device *yeng;

	yeng = file->private_data;
	dev_info(yeng->dev, "write count=%d", count);
	return count;
}

static int yeng_cdev_open(struct inode *inode, struct file *file) {
	struct yeng_device *yeng;

	yeng = container_of(inode->i_cdev, struct yeng_device, cdev);
	file->private_data = yeng;
	dev_info(yeng->dev, "open");
	return 0;
}

static int yeng_cdev_release(struct inode *inode, struct file *file) {
	struct yeng_device *yeng;

	yeng = container_of(inode->i_cdev, struct yeng_device, cdev);
	dev_info(yeng->dev, "close");
	return 0;
}

static struct file_operations yeng_fops = {
	.owner = THIS_MODULE,
	.read = yeng_cdev_read,
	.write = yeng_cdev_write,
	.open = yeng_cdev_open,
	.release = yeng_cdev_release,
};


int yeng_probe(struct platform_device *pdev) {
	struct resource *regs_resource;
	void __iomem *iomem;
	int irq;
	int minor;
	struct yeng_device *yeng;
	int res;
	struct device *dev_create;

	dev_info(&pdev->dev, "probe");

	yeng = kzalloc(sizeof(struct yeng_device), GFP_KERNEL);
	if (!yeng)
		return -ENOMEM;
	yeng->dev = &pdev->dev;

	iomem = devm_platform_get_and_ioremap_resource(pdev, 0, &regs_resource);
	if (IS_ERR(iomem)) {
		res = PTR_ERR(iomem);
		goto out_free_private;
	}
	yeng->iomem = iomem;
	dev_info(&pdev->dev, "registers mapped: %x-%x", regs_resource->start, regs_resource->end);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		res = irq;
		goto out_free_private;
	}
	dev_info(&pdev->dev, "virt irq claimed: %d", irq);

	// TODO(liam) claim clock

	cdev_init(&yeng->cdev, &yeng_fops);
	yeng->cdev.owner = THIS_MODULE;

	mutex_lock(&yeng_ida_lock);
	minor = ida_simple_get(&yeng_ida, 0, 0, GFP_KERNEL);
	mutex_unlock(&yeng_ida_lock);
	if (minor >= MAX_DEVICES) {
		// ran out of minors
		res = -ENODEV;
		goto out_remove_ida;
	}

	yeng->cdev_id = MKDEV(MAJOR(yeng_cdev_first), MINOR(minor));
	res = cdev_add(&yeng->cdev, yeng->cdev_id, 1);
	if (res < 0)
		goto out_free_private;
	dev_info(&pdev->dev, "added cdev, major=%d minor=%d", MAJOR(yeng_cdev_first), MINOR(minor));

	dev_create = device_create(yeng_class, NULL, yeng->cdev_id, NULL, CDEV_NAME_PREFIX "%d", minor);
	if (IS_ERR(dev_create)) {
		res = PTR_ERR(dev_create);
		goto out_del_cdev;
	}
	dev_info(&pdev->dev, "added cdev, major=%d minor=%d", MAJOR(yeng_cdev_first), MINOR(minor));

	platform_set_drvdata(pdev, yeng);

	return 0;

out_destroy_device:
	device_destroy(yeng_class, yeng->cdev_id);
out_del_cdev:
	cdev_del(&yeng->cdev);
out_remove_ida:
	mutex_lock(&yeng_ida_lock);
	ida_simple_remove(&yeng_ida, minor);
	mutex_unlock(&yeng_ida_lock);
out_free_private:
	kfree(yeng);
	return res;
}

int yeng_remove(struct platform_device *pdev) {
	struct yeng_device *yeng;

	dev_info(&pdev->dev, "remove device (cdev major=%d minor=%d)", MAJOR(yeng->cdev_id), MINOR(yeng->cdev_id));
	yeng = platform_get_drvdata(pdev);

	device_destroy(yeng_class, yeng->cdev_id);
	cdev_del(&yeng->cdev);
	mutex_lock(&yeng_ida_lock);
	ida_simple_remove(&yeng_ida, MINOR(yeng->cdev_id));
	mutex_unlock(&yeng_ida_lock);
	return 0;
}

static const struct of_device_id yeng_of_match[] = {
	{ .compatible = "digico,yeng", },
	{}
};
MODULE_DEVICE_TABLE(of, yeng_of_match);

static struct platform_driver yeng_platform_driver = {
	.probe = yeng_probe,
	.remove = yeng_remove,
	.driver = {
		.name = "yeng",
		.of_match_table = yeng_of_match,
	},
};

static int __init yeng_init(void) {
	int res = 0;

	yeng_class = class_create(THIS_MODULE, DRIVER_NAME);
	if (IS_ERR(yeng_class))
		return PTR_ERR(yeng_class);

	res = alloc_chrdev_region(&yeng_cdev_first, 0, MAX_DEVICES, "yeng");
	if (res < 0)
		goto out_destroy_class;

	res = platform_driver_register(&yeng_platform_driver);
	if (res < 0)
		goto out_unregister_chrdev;

	return 0;

out_unregister_chrdev:
	unregister_chrdev_region(yeng_cdev_first, MAX_DEVICES);
out_destroy_class:
	class_destroy(yeng_class);
	return res;
}
module_init(yeng_init);


static void __exit yeng_exit(void) {
	platform_driver_unregister(&yeng_platform_driver);
	unregister_chrdev_region(yeng_cdev_first, MAX_DEVICES);
	class_destroy(yeng_class);
}
module_exit(yeng_exit);

MODULE_DESCRIPTION("PS-PL engine comms rewritten as platform device");
MODULE_AUTHOR("Liam Pribis");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
