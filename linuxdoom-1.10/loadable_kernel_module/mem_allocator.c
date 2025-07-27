#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/kobject.h>

static unsigned long buf_size = 6*1024*1024;
module_param(buf_size, ulong, 0444);
MODULE_PARM_DESC(buf_size, "Size of shared buffer in bytes");

static void *cpu_addr;
static dma_addr_t phys_addr;
static struct kobject *kobj;

static ssize_t phys_show(struct kobject *k, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "0x%llx\n", (unsigned long long)phys_addr);
}
static struct kobj_attribute phys_attr = __ATTR_RO(phys);

static int __init fpga_shm_init(void) {
    cpu_addr = dma_alloc_coherent(NULL, buf_size, &phys_addr, GFP_KERNEL);
    if (!cpu_addr) return -ENOMEM;
    kobj = kobject_create_and_add("fpga_shared", kernel_kobj);
    if (!kobj) { dma_free_coherent(NULL, buf_size, cpu_addr, phys_addr); return -ENOMEM; }
    sysfs_create_file(kobj, &phys_attr.attr);
    pr_info("fpga_shm: reserved %lu bytes @ phys 0x%llx\n", buf_size,
            (unsigned long long)phys_addr);
    return 0;
}

static void __exit fpga_shm_exit(void) {
    sysfs_remove_file(kobj, &phys_attr.attr);
    kobject_put(kobj);
    dma_free_coherent(NULL, buf_size, cpu_addr, phys_addr);
}

module_init(fpga_shm_init);
module_exit(fpga_shm_exit);
MODULE_LICENSE("GPL");
