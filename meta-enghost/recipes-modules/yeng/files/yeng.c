#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>




int yeng_probe(struct platform_device *pdev) {
	struct resource *regs_resource;
	void __iomem *regs;
	int irq;

	dev_info(&pdev->dev, "probe");

	regs = devm_platform_get_and_ioremap_resource(pdev, 0, &regs_resource);
	if (IS_ERR(regs))
		return PTR_ERR(regs);
	dev_info(&pdev->dev, "registers mapped: %x-%x", regs_resource->start, regs_resource->end);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;
	dev_info(&pdev->dev, "virt irq claimed: %d", irq);

	return 0;
}

int yeng_remove(struct platform_device *pdev) {
	dev_info(&pdev->dev, "remove");
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

module_platform_driver(yeng_platform_driver);

MODULE_DESCRIPTION("PS-PL engine comms rewritten as platform device");
MODULE_AUTHOR("Liam Pribis");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
