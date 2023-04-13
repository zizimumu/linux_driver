
#include <linux/pci.h>
#include <linux/module.h> 
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/timex.h>
#include <linux/timer.h>
#include <linux/fb.h>

#define DRIVER_NAME "test_pcie"



#define DAM_BUFF_SIZE (8*1024*1024)
#define AXI_BAR_ALIGN_SIZE (4*1024*1024)
#define BITS_PER_PIX 32

struct pcie_dev_adapter{
	struct pci_dev *pdev;
	u8 __iomem *mem_space_addr;
	resource_size_t mem_space_length;
	dma_addr_t dma_ptr;
	void *cpu_ptr;
	dma_addr_t dma_ptr_unalign;
	void *cpu_ptr_unalign;
	struct fb_info *fb_info;
	u8 flag;
};	


static volatile int dma_done = 0;
struct pcie_dev_adapter *g_adapter;

ktime_t start_t;
ktime_t end_t;


#define DMA_DESC_OFFSET 0x0000

#define DMA_CTL_OFFSET 0x4000
#define PCIE_CTL_OFFSET 0x5000

#define IMAGE_WIDTH 1280 
#define IMAGE_HEIGHT 720

#define DMA_PCIE_BAR_ADDR 0x40000000
#define DMA_AXI_HDMI 0x76000000
#define DMA_DESC_ADDR 0x80000000

#define FRAME_SIZE (IMAGE_HEIGHT*IMAGE_WIDTH*( BITS_PER_PIX/8) )


static irqreturn_t dma_isr(int irq, void *dev_id)
{
/*
	u8  *dma_base = g_adapter->mem_space_addr + DMA_CTL_OFFSET;
	u32 val;
	
	end_t = ktime_get();
	printk("dma irq\n");
	val = ioread32(dma_base+4);
	if(val & (1 << 12) ){ // dma done
		printk("dma done\n");
		iowrite32(1 << 12, dma_base + 4);
		dma_done = 1;
	}
*/
	return IRQ_HANDLED;

}




void dma_init(void)
{
	void  *dma_ctl_base = g_adapter->mem_space_addr + DMA_CTL_OFFSET;
	void *dma_desc_base = g_adapter->mem_space_addr  + DMA_DESC_OFFSET;
	void *pcie_ctl_base = g_adapter->mem_space_addr + PCIE_CTL_OFFSET;
	dma_addr_t dma_addr;

	// set AXI->PCIE translation table
	dma_addr = g_adapter->dma_ptr;
	iowrite32((unsigned int)(dma_addr >> 32), pcie_ctl_base + 0x208);
	iowrite32((unsigned int)dma_addr, pcie_ctl_base + 0x20c);
		
		
		
	iowrite32(DMA_DESC_ADDR + 0x40,dma_desc_base);	// next descriptor addr
	iowrite32(DMA_PCIE_BAR_ADDR,dma_desc_base+0x8);	// source addr
	iowrite32(DMA_AXI_HDMI,dma_desc_base+0x10);		// dest addr
	iowrite32(FRAME_SIZE,dma_desc_base+0x18);		// length
	iowrite32(0,dma_desc_base+0x1c);			// status
	
	
	iowrite32(DMA_DESC_ADDR,dma_desc_base + 0x40);	// next descriptor addr
	iowrite32(DMA_PCIE_BAR_ADDR,dma_desc_base+0x48);	// source addr
	iowrite32(DMA_AXI_HDMI,dma_desc_base+0x50);		// dest addr
	iowrite32(FRAME_SIZE,dma_desc_base+0x58);		// length
	iowrite32(0,dma_desc_base+0x5c);			// status
		
	
	iowrite32(4,dma_ctl_base);				// reset dma
	iowrite32(0x48,dma_ctl_base);				// sg and cycle mode
	iowrite32(DMA_DESC_ADDR,dma_ctl_base+0x08);		// first descriptor
	iowrite32(0x55555555,dma_ctl_base+0x10);		// just trigger dma to start


}

void dma_stop(void)
{
	void  *dma_ctl_base = g_adapter->mem_space_addr + DMA_CTL_OFFSET;
	iowrite32(4,dma_ctl_base);				// reset dma
}




static struct fb_var_screeninfo pciefb_default = {
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo pciefb_fix = {
	.id = "pciefb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR, 
	.accel = FB_ACCEL_NONE,
};

static int fb_open(struct fb_info *info, int user)
{
	printk("pciefb open\n");
	if(g_adapter->flag == 0){
		printk("start fb output\n");
		dma_init();
		g_adapter->flag = 1;
	}

	
	return 0;
}
static int fb_release(struct fb_info *info, int user)
{
	printk("pciefb close\n");
	//dma_stop();
	
	return 0;
}


static struct fb_ops pciefb_ops = {
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.owner		= THIS_MODULE,
	.fb_open	= fb_open,
	.fb_release = fb_release,
};

struct fb_info * fb_init(struct device *dev, void *fbmem_virt,unsigned long phy_start)
{

	struct fb_info *info;

	int retval = -ENOMEM;

	info = framebuffer_alloc(sizeof(u32) * 256, dev);
	if (!info)
		goto error0;


	info->fbops = &pciefb_ops;
	info->var = pciefb_default;
	info->fix = pciefb_fix;

	info->var.bits_per_pixel = BITS_PER_PIX;

	info->var.xres = IMAGE_WIDTH;
	info->var.yres = IMAGE_HEIGHT;


	info->var.xres_virtual = info->var.xres,
	info->var.yres_virtual = info->var.yres;


	if(info->var.bits_per_pixel == 16) {
		info->var.red.offset = 11;
		info->var.red.length = 5;
		info->var.red.msb_right = 0;
		info->var.green.offset = 5;
		info->var.green.length = 6;
		info->var.green.msb_right = 0;
		info->var.blue.offset = 0;
		info->var.blue.length = 5;
		info->var.blue.msb_right = 0;
	} else {
	// 32 bit
		info->var.red.offset = 16;
		info->var.red.length = 8;
		info->var.red.msb_right = 0;
		info->var.green.offset = 8;
		info->var.green.length = 8;
		info->var.green.msb_right = 0;
		info->var.blue.offset = 0;
		info->var.blue.length = 8;
		info->var.blue.msb_right = 0;
	
	}

	info->fix.line_length = (info->var.xres * (info->var.bits_per_pixel >> 3));
	info->fix.smem_len = info->fix.line_length * info->var.yres;

	info->fix.smem_start = phy_start;
	info->screen_base = fbmem_virt;
	info->pseudo_palette = info->par;
	info->par = NULL;
	info->flags = FBINFO_FLAG_DEFAULT;


	retval = fb_alloc_cmap(&info->cmap, 256, 0);
	if (retval < 0){
		dev_err(dev, "unable to allocate  fb cmap\n");
		goto error1;
	}


	retval = register_framebuffer(info);
	if (retval < 0){
		dev_err(dev, "unable to register framebuffer\n");
		goto error2;
	}

	dev_info(dev, "fb%d: %s frame buffer device at 0x%x+0x%x\n",
		info->node, info->fix.id, (unsigned)info->fix.smem_start,
		info->fix.smem_len);


	info->fbops = &pciefb_ops;

	
	return info;


	// unregister_framebuffer(dev->fb_info);
error2:
	fb_dealloc_cmap(&info->cmap);
error1:
	framebuffer_release(info);

error0:
	return NULL;
}


void release_fb(struct fb_info *info)
{

	unregister_framebuffer(info);
	fb_dealloc_cmap(&info->cmap);
	framebuffer_release(info);
}


static int test_pcidev_probe(struct pci_dev *pdev,
				const struct pci_device_id *id)
{
	
	
	unsigned long bars = 0;
	resource_size_t bar_start, bar_length;
	int ret,result = -1;
	struct pcie_dev_adapter *adapter;
	u8 __iomem *address;
	void *cpu_ptr = NULL;
	unsigned int *buf32,i;
	dma_addr_t dma_ptr;

	printk("test pcie device probe\n");

	adapter = devm_kzalloc(&pdev->dev, sizeof(struct pcie_dev_adapter),
			      GFP_KERNEL);
	if(!adapter){
		return -ENOMEM;
	}
	
	pci_set_master(pdev);
	
	ret = pci_enable_device(pdev);
	if (ret){
		result = -ENOMEM;
		goto return_error;
	}

	bars = pci_select_bars(pdev, IORESOURCE_MEM);
	if (!test_bit(0, &bars))
		goto disable_device;

	ret = pci_request_selected_regions(pdev, bars, DRIVER_NAME);
	if (ret)
		goto disable_device;

	
	/*
	if (!pci_enable_msi(pdev)){
		printk("enable MSI interrupt\n");
	}
	else
		printk("enable MSI interrupt failed\n");

	*/

	//pci_keep_intx_enabled(pdev);
	
	/*
	ret = request_irq(pdev->irq,
			  dma_isr,
			  0, DRIVER_NAME, pdev);
	if(ret){
		printk("request IRQ %d,error %d\n",pdev->irq,ret);
	}
	*/
			  
			  
				  
	bar_start = pci_resource_start(pdev, 0);
	bar_length = pci_resource_len(pdev, 0);
	
	printk("bar start addr 0x%llx, len 0x%llx\n",bar_start,bar_length);
	
	
	address = devm_ioremap(&pdev->dev,bar_start, bar_length);
	if (!address) {
		result = -ENOMEM;
		goto disable_device;
	}	
	


	if (dma_set_mask_and_coherent(&pdev->dev,
				      DMA_BIT_MASK(64))) {
		if (dma_set_mask_and_coherent(&pdev->dev,
					      DMA_BIT_MASK(32))) {
			dev_warn(&pdev->dev,
				 "No suitable DMA available\n");
			ret = -ENOMEM;
			goto disable_device;
		}
	}
	
		

	cpu_ptr = dma_alloc_coherent(&pdev->dev,DAM_BUFF_SIZE, &dma_ptr,
				     GFP_KERNEL);
	if (!cpu_ptr) {
	
		dev_warn(&pdev->dev,"dma alloc failed\n");
		ret = -ENOMEM;
		goto dma_err;
	}
	

	memset(cpu_ptr,0,DAM_BUFF_SIZE);
	adapter->dma_ptr = dma_ptr & ( ~((dma_addr_t) (AXI_BAR_ALIGN_SIZE -1)) ) ;
	if( (dma_ptr & ((dma_addr_t) (AXI_BAR_ALIGN_SIZE -1))  ) != 0)
		adapter->dma_ptr += AXI_BAR_ALIGN_SIZE;
	
	
	adapter->cpu_ptr = cpu_ptr + adapter->dma_ptr - dma_ptr;
	
	adapter->dma_ptr_unalign = dma_ptr;
	adapter->cpu_ptr_unalign = cpu_ptr;
	
	
	adapter->pdev = pdev;
	adapter->mem_space_addr = address;
	adapter->mem_space_length = bar_length;
	adapter->flag = 0;

	printk("      dma memory PHY addr 0x%llx, virtual addr 0x%llx\n",dma_ptr,cpu_ptr);
	printk("fixed dma memory PHY addr 0x%llx, virtual addr 0x%llx\n",adapter->dma_ptr,(dma_addr_t)adapter->cpu_ptr);
	
	g_adapter = adapter;
	
	pci_set_drvdata(pdev, adapter);


	//misc_register(&poll_dev); 
	//if(sysfs_create_group(&poll_dev.this_device->kobj, &pcie_group) != 0)
	//{
	//	printk("create %s sys-file err \n",pcie_group.name);
	//}	

	adapter->fb_info = fb_init(&pdev->dev,adapter->cpu_ptr,adapter->dma_ptr);
	
	
	// dma_init();
	
	
	return 0;
	
dma_err:
	
	pci_release_selected_regions(pdev,pci_select_bars(pdev,
						     IORESOURCE_MEM));
	free_irq(pdev->irq,pdev);
	pci_disable_msi(pdev);
						     
disable_device:
	pci_disable_device(pdev);

return_error:	
	
	return result;
	
}				
	


static void test_pcidev_remove(struct pci_dev *pdev)
{
	struct pcie_dev_adapter *adapter = pci_get_drvdata(pdev);

	
	dma_stop();
	// devm_ioremap_release(&pdev->dev,adapter->mem_space_addr);
	if(adapter->fb_info){
		release_fb(adapter->fb_info);
	}
	
	dma_free_coherent(&pdev->dev,DAM_BUFF_SIZE, adapter->cpu_ptr_unalign,
			  adapter->dma_ptr_unalign);
				  
	pci_release_selected_regions(pdev,pci_select_bars(pdev,
						     IORESOURCE_MEM));
						     
		
	// free_irq(pdev->irq,pdev);	
	pci_disable_msi(pdev);
				     
	pci_disable_device(pdev);
	
	//sysfs_remove_group(&poll_dev.this_device->kobj, &pcie_group);
	// free(adapter);
	
	//misc_deregister(&poll_dev);
}	

static const struct pci_device_id test_pci_tbl[] = {
	{ PCI_DEVICE(0x10EE, 0x7025) },
	{ 0, }
};

static struct pci_driver test_pci_driver = {
	.name		= DRIVER_NAME,
	.probe		= test_pcidev_probe,
	.remove		= test_pcidev_remove,
	.id_table	= test_pci_tbl,
};

module_pci_driver(test_pci_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("test pcie driver");
MODULE_DEVICE_TABLE(pci, test_pci_tbl);
