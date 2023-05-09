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
#include <linux/platform_device.h>
#include <video/videomode.h>


#define debug_log trace_printk
#define DAM_BUFF_SIZE (5*1024*1024)



#define DMA_REG_START_OFF 0x4
#define DMA_REG_IRQ_STAT_OFF 0x10
#define DMA_REG_CLEAR_IRQ_OFF 0x18
#define DMA_REG_MASK_IRQ_OFF 0x14
#define DMA_REG_SRC_OFF 0x68
#define DMA_REG_DEST_OFF 0x6c
#define DMA_REG_LEN_OFF 0x64
#define DMA_REG_CFG_OFF 0x60



#define DMA_STATUS_RUN 1
#define DMA_STATUS_IDLE 0


#define LCD_REG_CTL_OFF 0
#define LCD_REG_H1_OFF 0x04
#define LCD_REG_H2_OFF 0x08
#define LCD_REG_V1_OFF 0x0c
#define LCD_REG_V2_OFF 0x10
#define LCD_REG_INFO_OFF 0x14

struct lcd_timings {
	u16 h_sync;
	u16 h_back;
	u16 h_disp;
	u16 h_front;
	
	u16 v_sync;
	u16 v_back;
	u16 v_disp;
	u16 v_front;

	u32 pix_clk;
	
};

struct lcd_timings lcd_mode[2] = {
	// 800x480
	{
		.h_sync = 2,
		.h_back = 64,
		.h_disp = 800,
		.h_front = 63,

		.v_sync = 2, 
		.v_back = 22,
		.v_disp =480,
		.v_front = 23,

		.pix_clk = 30000000,
	},

};



struct lcd_dev{
	int dma_irq;
	void *frame_buffer_cpu;
	resource_size_t frame_buffer_phy;
	resource_size_t fifo_addr;
	void *dma_base;
	void *lcd_base;
	struct fb_info *fb_info;
	u32 lcd_width;
	u32 lcd_height;
	u32 frame_size;
	u32 pix_len;
	int status;
	spinlock_t	lock;
	void *coherent_dma_cpu_ptr;
	dma_addr_t coherent_dma_phy_addr;
	dma_addr_t current_addr;
	struct lcd_timings *lcd_timing;
	wait_queue_head_t wait;
	u32 sync_count;
	
};

void reg_dump(struct lcd_dev *lcd_dev )
{
	int i;
	u32 *reg = lcd_dev->lcd_base;

	printk("lcd regs:\n");
	for(i=0;i<6;i++){
		printk("    %d: 0x%08x\n",i,reg[i]);
	}
	printk("\n");

}
void init_lcd_reg(struct lcd_dev *lcd_dev )
{
	u32 val;
	struct lcd_timings *timing;

	timing = lcd_dev->lcd_timing;

	val = ( (u32)timing->h_sync) << 16 | timing->h_back;
	iowrite32(val,lcd_dev->lcd_base+LCD_REG_H1_OFF);	

	val = ( (u32)timing->h_disp) << 16 | timing->h_front;
	iowrite32(val,lcd_dev->lcd_base+LCD_REG_H2_OFF);

	val = ( (u32)timing->v_sync) << 16 | timing->v_back;
	iowrite32(val,lcd_dev->lcd_base+LCD_REG_V1_OFF);

	val = ( (u32)timing->v_disp) << 16 | timing->v_front;
	iowrite32(val,lcd_dev->lcd_base+LCD_REG_V2_OFF);

	// start LCD
	iowrite32(0x1,lcd_dev->lcd_base);

	// reg_dump(lcd_dev);
}
static void dma_init(struct lcd_dev *lcd_dev)
{
	void *dma_base = lcd_dev->dma_base;


	// mask IRQ
	iowrite32(0x1,dma_base+DMA_REG_MASK_IRQ_OFF);	
	// clear IRQ
	iowrite32(0xF,dma_base+DMA_REG_CLEAR_IRQ_OFF);	
	// source addr
	iowrite32((u32)lcd_dev->current_addr,dma_base+DMA_REG_SRC_OFF);	
	// dest addr
	iowrite32((u32)lcd_dev->fifo_addr,dma_base+DMA_REG_DEST_OFF);	
	// len
	
	iowrite32(lcd_dev->frame_size,dma_base+DMA_REG_LEN_OFF);	
	// config
	iowrite32(0x0000F005,dma_base+DMA_REG_CFG_OFF);	


}


static irqreturn_t dma_irq_hanlder(int irqno, void *dev_id)
{
	struct lcd_dev *lcd_dev= dev_id;

	static int flag = 0;


	// debug_log("irq\n");
	//if(flag == 0){
	//	debug_log("dma irq\n");
	//	flag = 1;
	//}
	// iowrite32(0xF,lcd_dev->dma_base+DMA_REG_CLEAR_IRQ_OFF);	
	
	if(ioread32(lcd_dev->dma_base+DMA_REG_IRQ_STAT_OFF) & 0x1){

		spin_lock(&lcd_dev->lock);
		
		dma_init(lcd_dev);
		// enable IRQ
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
		// start
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_START_OFF);

		lcd_dev->sync_count++;
		wake_up(&lcd_dev->wait);
		
		spin_unlock(&lcd_dev->lock);

		
		
	}
	
	
	return IRQ_HANDLED;
}



static struct fb_var_screeninfo axi_lcd_fb_default = {
	.activate = FB_ACTIVATE_NOW,
	.height = -1,
	.width = -1,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct fb_fix_screeninfo axi_lcd_fb_fix = {
	.id = "axi_lcd_fb",
	.type = FB_TYPE_PACKED_PIXELS,
	.visual = FB_VISUAL_TRUECOLOR, 
	.accel = FB_ACCEL_NONE,
};

static int fb_open(struct fb_info *info, int user)
{
	struct lcd_dev *lcd_dev = info->par;
	debug_log("fb_open open\n");


	spin_lock(&lcd_dev->lock);
	if(lcd_dev->status != DMA_STATUS_RUN){

		debug_log("start DMA\n");

		// enable LCD controller
		init_lcd_reg(lcd_dev);
	
		dma_init(lcd_dev);
		
		// enable IRQ
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
		// start
		iowrite32(0x1,lcd_dev->dma_base+DMA_REG_START_OFF);

		lcd_dev->status = DMA_STATUS_RUN;

	}

	spin_unlock(&lcd_dev->lock);
	
	return 0;
}
static int fb_release(struct fb_info *info, int user)
{
	//struct lcd_dev *lcd_dev = info->par;

	
	debug_log("pciefb close\n");

	//iowrite32(0x0,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);
	//iowrite32(0xF,lcd_dev->dma_base+DMA_REG_CLEAR_IRQ_OFF);	
	
	return 0;
}


#if 1

// fb_setcolreg is must-be if framebuffer console enabled
/*
In true color mode (which is what our example LCD controller supports),
modifiable palettes are not relevant. However, you still have to satisfy 
the demands of the frame buffer console driver, which uses only 16 colors.
For this, you have to create a pseudo palette by encoding the corresponding 16 raw RGB values 
into bits that can be directly fed to the hardware

*/
static int altfb_setcolreg(unsigned regno, unsigned red, unsigned green,
			   unsigned blue, unsigned transp, struct fb_info *info)
{
	/*
	 *  Set a single color register. The values supplied have a 32/16 bit
	 *  magnitude.
	 *  Return != 0 for invalid regno.
	 */

	//debug_log("set\n");
	

	if (regno > 255)
		return 1;

    if(info->var.bits_per_pixel == 16) {
	    red >>= 11;
	    green >>= 10;
	    blue >>= 11;

	    if (regno < 16) {
		    ((u32 *) info->pseudo_palette)[regno] = ((red & 31) << 11) |
		        ((green & 63) << 5) | (blue & 31);
	    }
    } else {
	    red >>= 8;
	    green >>= 8;
	    blue >>= 8;

	    if (regno < 16) {
		    ((u32 *) info->pseudo_palette)[regno] = ((red & 255) << 16) |
		        ((green & 255) << 8) | (blue & 255);
	    }
    }

	return 0;
}
#endif



static int mx_fb_pan_display(struct fb_var_screeninfo *var,
							struct fb_info *info)
{
	//unsigned long irq_flags;
	struct lcd_dev *lcd_dev = info->par;
	dma_addr_t current_addr;
	u32 sync_count;

	
	current_addr = info->fix.smem_start + info->fix.line_length  * var->yoffset;

	// only wait for sync if dma buf is not the current addr
	if(current_addr != lcd_dev->current_addr ){

		spin_lock(&lcd_dev->lock);

		lcd_dev->current_addr = current_addr;
		sync_count = lcd_dev->sync_count + 1;

		spin_unlock(&lcd_dev->lock);

		
		wait_event_timeout(lcd_dev->wait,
				sync_count == lcd_dev->sync_count, HZ / 15);

		
	}
	else{

	}

	
		
	debug_log("yoffset %d, xoffset %d\n",var->yoffset,var->xoffset);
	return 0;
}


static int mx_fb_ioctl(struct fb_info *info, unsigned int cmd,
			 unsigned long arg)
{
	int retval = 0;
	struct lcd_dev *lcd_dev = info->par;
	dma_addr_t current_addr;
	u32 sync_count;

	switch (cmd) {
		case FBIO_WAITFORVSYNC:
			debug_log("wait sync\n");
			spin_lock(&lcd_dev->lock);
			
			sync_count = lcd_dev->sync_count + 1;
			
			spin_unlock(&lcd_dev->lock);
			
			
			wait_event_timeout(lcd_dev->wait,
					sync_count == lcd_dev->sync_count, HZ / 15);


		
			break;
		default:
			break;
	}

	return retval;
}



static struct fb_ops axi_lcd_fb_ops = {
	.fb_fillrect = cfb_fillrect,
	.fb_copyarea = cfb_copyarea,
	.fb_imageblit = cfb_imageblit,
	.owner		= THIS_MODULE,
	.fb_open	= fb_open,
	.fb_release = fb_release,
	.fb_setcolreg = altfb_setcolreg,
	.fb_pan_display = mx_fb_pan_display,
	.fb_ioctl =		mx_fb_ioctl,
};



void init_video_mode(struct videomode *vm,struct lcd_timings *lcd_timing)
{
	vm->pixelclock = lcd_timing->pix_clk;
	vm->hactive = lcd_timing->h_disp;
	vm->hfront_porch = lcd_timing->h_front;
	vm->hback_porch = lcd_timing->h_back;
	vm->hsync_len = lcd_timing->h_sync;

	vm->vactive = lcd_timing->v_disp;
	vm->vfront_porch = lcd_timing->v_front;
	vm->vback_porch = lcd_timing->v_back;
	vm->vsync_len = lcd_timing->v_sync;
}


static int videomode_from_videomode(const struct videomode *vm,
				struct fb_videomode *fbmode)
{
	unsigned int htotal, vtotal;

	fbmode->xres = vm->hactive;
	fbmode->left_margin = vm->hback_porch;
	fbmode->right_margin = vm->hfront_porch;
	fbmode->hsync_len = vm->hsync_len;

	fbmode->yres = vm->vactive;
	fbmode->upper_margin = vm->vback_porch;
	fbmode->lower_margin = vm->vfront_porch;
	fbmode->vsync_len = vm->vsync_len;

	/* prevent division by zero in KHZ2PICOS macro */
	fbmode->pixclock = vm->pixelclock ?
			KHZ2PICOS(vm->pixelclock / 1000) : 0;

	fbmode->sync = 0;
	fbmode->vmode = 0;
	if (vm->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		fbmode->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (vm->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		fbmode->sync |= FB_SYNC_VERT_HIGH_ACT;
	if (vm->flags & DISPLAY_FLAGS_INTERLACED)
		fbmode->vmode |= FB_VMODE_INTERLACED;
	if (vm->flags & DISPLAY_FLAGS_DOUBLESCAN)
		fbmode->vmode |= FB_VMODE_DOUBLE;
	fbmode->flag = 0;

	htotal = vm->hactive + vm->hfront_porch + vm->hback_porch +
		 vm->hsync_len;
	vtotal = vm->vactive + vm->vfront_porch + vm->vback_porch +
		 vm->vsync_len;
	/* prevent division by zero */
	if (htotal && vtotal) {
		fbmode->refresh = vm->pixelclock / (htotal * vtotal);
	/* a mode must have htotal and vtotal != 0 or it is invalid */
	} else {
		fbmode->refresh = 0;
		return -EINVAL;
	}

	return 0;
}

static int fb_init(struct device *dev, void *fbmem_virt,unsigned long phy_start, struct lcd_dev *lcd_dev)
{

	struct fb_info *info;
	int ret;
	int retval = -ENOMEM;
	struct fb_videomode fb_vm;
	struct videomode vm;
	// struct fb_modelist *modelist;

	info = framebuffer_alloc(sizeof(u32) * 256, dev);
	if (!info)
		goto error0;

	INIT_LIST_HEAD(&info->modelist);
	
	info->fbops = &axi_lcd_fb_ops;
	info->var = axi_lcd_fb_default;
	info->fix = axi_lcd_fb_fix;

	info->var.bits_per_pixel = lcd_dev->pix_len;
	info->var.xres = lcd_dev->lcd_width;
	info->var.yres = lcd_dev->lcd_height;

	info->var.xres_virtual = info->var.xres,
	info->var.yres_virtual = info->var.yres*2;

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

		info->var.transp.offset = 24;
		info->var.transp.length = 8;
	
	}

	info->fix.line_length = (info->var.xres * (info->var.bits_per_pixel >> 3));
	info->fix.smem_len = info->fix.line_length * info->var.yres * 2;

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

	info->par = lcd_dev;

	//modelist = list_first_entry(&info->modelist,
	//		struct fb_modelist, list);
	

	init_video_mode(&vm,lcd_dev->lcd_timing);
	videomode_from_videomode(&vm, &fb_vm);
	fb_add_videomode(&fb_vm, &info->modelist);


	//fb_videomode_to_var(&info->var, &modelist->mode);


	
	retval = register_framebuffer(info);
	if (retval < 0){
		dev_err(dev, "unable to register framebuffer\n");
		goto error2;
	}

	dev_info(dev, "fb%d: %s frame buffer device at 0x%x+0x%x, resolution %dx%d\n",
		info->node, info->fix.id, (unsigned)info->fix.smem_start,
		info->fix.smem_len,info->var.xres,info->var.yres);

	
	return 0;


	// unregister_framebuffer(dev->fb_info);
error2:
	fb_dealloc_cmap(&info->cmap);
error1:
	framebuffer_release(info);

error0:
	return -1;
}


static void release_fb(struct fb_info *info)
{

	unregister_framebuffer(info);
	fb_dealloc_cmap(&info->cmap);
	framebuffer_release(info);
}


static int lcd_probe(struct platform_device *pdev)
{
	
	struct lcd_dev *lcd_dev;
	struct resource *mem,*dma,*fifo,*lcd_reg;
	void *frame_buffer,*dma_base,*lcd_base;
	int irq,ret;
	dma_addr_t dma_ptr;
	void *cpu_ptr;

	lcd_dev = devm_kzalloc(&pdev->dev, sizeof(*lcd_dev), GFP_KERNEL);
	if (!lcd_dev)
		return -ENOMEM;

	dma = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma_reg");
	if(!dma){
		dev_err(&pdev->dev, "no DMA addr found\n");
		return -ENODEV;
	}
	dma_base = devm_ioremap_resource(&pdev->dev, dma);
	if (IS_ERR(dma_base))
		return PTR_ERR(dma_base);
	

	irq = platform_get_irq(pdev, 0);
	if (irq < 0){
		dev_err(&pdev->dev, "get irq faild\n");
		return irq;
	}
	
	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "memory");
	if(!mem)
		dev_err(&pdev->dev, "no memory reg found\n");

	frame_buffer = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(frame_buffer))
		return PTR_ERR(frame_buffer);

	fifo = platform_get_resource_byname(pdev, IORESOURCE_MEM, "fifo");
	if(!fifo){
		dev_err(&pdev->dev, "no fifo addr found\n");
		return -ENODEV;
	}

	lcd_reg = platform_get_resource_byname(pdev, IORESOURCE_MEM, "lcd_reg");
	if(!lcd_reg){
		dev_err(&pdev->dev, "no lcd_reg addr found\n");
		return -ENODEV;
	}
	lcd_base = devm_ioremap_resource(&pdev->dev, lcd_reg);
	if (IS_ERR(lcd_base))
		return PTR_ERR(lcd_base);

	lcd_dev->frame_buffer_cpu = frame_buffer;
	lcd_dev->frame_buffer_phy = mem->start;
	lcd_dev->current_addr = mem->start;
	lcd_dev->dma_base = dma_base;
	lcd_dev->fifo_addr = fifo->start;
	lcd_dev->dma_irq = irq;
	lcd_dev->lcd_base = lcd_base;
	
	lcd_dev->lcd_timing = &lcd_mode[0];
	lcd_dev->lcd_width = lcd_dev->lcd_timing->h_disp;
	lcd_dev->lcd_height = lcd_dev->lcd_timing->v_disp;
	lcd_dev->pix_len = ioread32(lcd_base + LCD_REG_INFO_OFF);

	if(lcd_dev->pix_len == 16 || lcd_dev->pix_len == 32){
		dev_info(&pdev->dev, "hardware support pix len %d\n",lcd_dev->pix_len);
	}
	else{
		dev_info(&pdev->dev, "could not hardware pix len, set to 32bit\n");
		lcd_dev->pix_len = 32;
	}
	
/*
	if (dma_set_mask_and_coherent(&pdev->dev,
				      DMA_BIT_MASK(32))) {
						  
		debug_log("get 32bit DMA failed, trying DMA64\n");
		if (dma_set_mask_and_coherent(&pdev->dev,
					      DMA_BIT_MASK(64))) {
			dev_warn(&pdev->dev,
				 "No suitable DMA available\n");
			ret = -ENOMEM;
			goto dma_err;
		}
	}
	
	cpu_ptr = dma_alloc_coherent(&pdev->dev,DAM_BUFF_SIZE, &dma_ptr,
				     GFP_KERNEL);
	if (!cpu_ptr) {
	
		dev_warn(&pdev->dev,"dma alloc failed\n");
		ret = -ENOMEM;
		goto dma_err;
	}
	dev_info(&pdev->dev,"get dma memory at 0x%llx\n",dma_ptr);
	lcd_dev->coherent_dma_cpu_ptr = cpu_ptr;
	lcd_dev->coherent_dma_phy_addr = dma_ptr;
	*/

	memset(frame_buffer,0,mem->end - mem->start + 1);

	lcd_dev->frame_size = lcd_dev->lcd_width * lcd_dev->lcd_height * (lcd_dev->pix_len / 8) ;
	lcd_dev->status = DMA_STATUS_IDLE;

	init_waitqueue_head(&lcd_dev->wait);
	
	ret = devm_request_irq(&pdev->dev,irq, dma_irq_hanlder, 0, "axi-lcd", lcd_dev);
	if (ret < 0) {
		pr_err("fail to claim dam irq , ret = %d\n", ret);
	}

	spin_lock_init(&lcd_dev->lock);

	platform_set_drvdata(pdev, lcd_dev);
	// the register_framebuffer should be set to be the last, since  register_framebuffer may open the framebuffer device
	fb_init(&pdev->dev,frame_buffer,mem->start,lcd_dev);

	printk("fb init OK, frame size 0x%x\n",lcd_dev->frame_size);

dma_err:
	
	return ret;
	
}				
	

static int lcd_remove(struct platform_device *pdev)
{
	struct lcd_dev *lcd_dev;

	lcd_dev = platform_get_drvdata(pdev);
	
	
	// mask IRQ
	iowrite32(0x0,lcd_dev->dma_base+DMA_REG_MASK_IRQ_OFF);	
	// clear IRQ
	iowrite32(0xF,lcd_dev->dma_base+DMA_REG_CLEAR_IRQ_OFF);		

	// disable LCD
	iowrite32(0x0,lcd_dev->lcd_base);
	
	release_fb(lcd_dev->fb_info);
	
	if(lcd_dev->coherent_dma_cpu_ptr != NULL){
		dma_free_coherent(&pdev->dev,DAM_BUFF_SIZE, lcd_dev->coherent_dma_cpu_ptr,
			lcd_dev->coherent_dma_phy_addr);
	}
			  
	return 0;
}


static const struct of_device_id axi_lcd_of_match[] = {
	{ .compatible = "mx,axi-lcd", },
	{},
};
MODULE_DEVICE_TABLE(of, axi_lcd_of_match);

static struct platform_driver mx_lcd_driver = {
	.driver = {
		.name   = "axi-lcd",
		.of_match_table = axi_lcd_of_match,
	},
	.probe	  = lcd_probe,
	.remove	 = lcd_remove,
};

module_platform_driver(mx_lcd_driver);


MODULE_DESCRIPTION("AXI LCD driver");
MODULE_AUTHOR("Murphy xu");
MODULE_LICENSE("GPL v2");
