/*
 * Copyright (C) 2012-2013, Analog Devices Inc.
 * Author: Lars-Peter Clausen <lars@metafoo.de>
 *
 * Licensed under the GPL-2.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <asm/io.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <linux/dma-mapping.h>


#define SPDIF_REG_RST_OFFSET 0x40
#define SPDIF_REG_CTRL_OFFSET 0x44

#define debug_log printk

#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)



struct idma_ctrl {
	spinlock_t	lock;
	int		state;
	dma_addr_t	start;
	dma_addr_t	pos;
	dma_addr_t	end;
	dma_addr_t	period;
	dma_addr_t	periodsz;
	u32 sz;
	void		*token;
	void		(*cb)(void *dt, int bytes_xfer);
};

struct xilinx_spdif {
	void __iomem *reg_base;
	void __iomem *dma_reg_base;
	struct clk *clk;
	int dma_irq;
	struct idma_ctrl *prtd;
};

struct axi_dma{
	void __iomem *reg_base;
	int irq;	
};

struct axi_dma dma_ctl;

static int xilinx_spdif_trigger(struct snd_pcm_substream *substream, int cmd,
	struct snd_soc_dai *dai)
{
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(dai);


	debug_log("Entered %s ,cmd is %d\n", __func__,cmd);
	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		writel(0x3, spdif->reg_base + SPDIF_REG_CTRL_OFFSET);	//reset fifo,and enable spdif
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		writel(0x2, spdif->reg_base + SPDIF_REG_CTRL_OFFSET);	//reset fifo,and disenable spdif
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int xilinx_spdif_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	debug_log("xilinx_spdif_hw_params Entered \n");
    
	return 0;
}

static int xilinx_spdif_dai_probe(struct snd_soc_dai *dai)
{
    struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(dai);

	//snd_soc_dai_init_dma_data(dai, &spdif->dma_data, NULL);
	debug_log("xilinx_spdif_dai_probe Entered \n");
    
	return 0;
}

static int xilinx_spdif_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(dai);


	debug_log("Entered %s\n", __func__);
	writel(0xa,spdif->reg_base + SPDIF_REG_RST_OFFSET);	//reset 

	return 0;
}

static void xilinx_spdif_shutdown(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(dai);

	debug_log("Entered %s\n", __func__);
	writel(0x2, spdif->reg_base + SPDIF_REG_CTRL_OFFSET);	//reset fifo,and disenable spdif

}

int xilinx_spdif_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    debug_log("Entered %s\n", __func__);
    return 0;
}


static const struct snd_soc_dai_ops xilinx_spdif_dai_ops = {
	.startup = xilinx_spdif_startup,
	.shutdown = xilinx_spdif_shutdown,
	.trigger = xilinx_spdif_trigger,
	.hw_params = xilinx_spdif_hw_params,
	.set_fmt = xilinx_spdif_set_fmt,
};

static struct snd_soc_dai_driver xilinx_spdif_dai = {
    //.name = "xilinx-spdif-rx",
	.probe = xilinx_spdif_dai_probe,
	.capture = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_192000,
		.formats = SNDRV_PCM_FMTBIT_S32_LE,
	},
	.ops = &xilinx_spdif_dai_ops,
};

static const struct snd_soc_component_driver xilinx_spdif_component = {
	.name = "xilinx-spdif-rx",
};

// ---------------------------------------------------------------

#define DMA_REG_DEST_OFF 0x48
#define DMA_REG_LEN_OFF 0x58
#define DMA_REG_CTRL_OFF 0x30
#define DMA_REG_STAT_OFF 0x34

#define DMA_RST_SET (1<<2)
#define DMA_START_SET (1)
#define DMA_IOC_INT_SET (1<<12)


#define MAX_IDMA_PERIOD (128 * 1024)
#define MAX_IDMA_BUFFER (256 * 1024)

static const struct snd_pcm_hardware idma_hardware = {
	.info =  SNDRV_PCM_INFO_INTERLEAVED |
		    /*SNDRV_PCM_INFO_NONINTERLEAVED |*/
		    SNDRV_PCM_INFO_BLOCK_TRANSFER |
		    SNDRV_PCM_INFO_MMAP |
		    SNDRV_PCM_INFO_MMAP_VALID |
		    SNDRV_PCM_INFO_PAUSE |
		    SNDRV_PCM_INFO_RESUME,
		    
	.formats = SNDRV_PCM_FMTBIT_S32_LE,
	.buffer_bytes_max = MAX_IDMA_BUFFER,
	.period_bytes_min = PAGE_SIZE,
	.period_bytes_max = MAX_IDMA_PERIOD,
	.periods_min = 1,
	.periods_max = 4,
};


static void spdif_start_transfer(struct idma_ctrl *prtd,
	struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(cpu_dai);

	unsigned long count;
	
	if(prtd->pos >= prtd->end)
		prtd->pos = prtd->start;
		
	if(prtd->pos + prtd->period > prtd->end)
		count = prtd->end - prtd->pos;
	else
		count = prtd->period;

	prtd->sz = count;
	writel(DMA_RST_SET, spdif->dma_reg_base + DMA_REG_CTRL_OFF);
	writel(DMA_START_SET|DMA_IOC_INT_SET, spdif->dma_reg_base + DMA_REG_CTRL_OFF); //start enable,interrupt on complete enable
	
	writel(prtd->pos, spdif->dma_reg_base + DMA_REG_DEST_OFF);
	
	
	writel(count, spdif->dma_reg_base + DMA_REG_LEN_OFF);//set length to start dma
	//prtd->pos +=count;
	
	
}

static int idma_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	debug_log("Entered %s\n", __func__);

	return dma_mmap_writecombine(substream->pcm->card->dev, vma,
				     runtime->dma_area,
				     runtime->dma_addr,
				     runtime->dma_bytes);
}


static int idma_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd = substream->runtime->private_data;

	debug_log("%s enter\n",__FUNCTION__);
	/* copy the dma info into runtime management */
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	/* get the max dma buffer size */
	runtime->dma_bytes = params_buffer_bytes(params);

	prtd->start = prtd->pos = runtime->dma_addr;
	prtd->period = params_period_bytes(params);
	prtd->end = runtime->dma_addr + runtime->dma_bytes;
	
	return 0;
}
static int idma_hw_free(struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}
static int idma_prepare(struct snd_pcm_substream *substream)
{

	struct idma_ctrl *prtd = substream->runtime->private_data;
	debug_log("%s enter\n",__FUNCTION__);

	prtd->pos = prtd->start;

	return 0;
}



static snd_pcm_uframes_t
	idma_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd = runtime->private_data;
	unsigned long byte_offset;
	snd_pcm_uframes_t frames;
	unsigned long flags;

	spin_lock_irqsave(&prtd->lock, flags);

	byte_offset = prtd->pos - prtd->start;
	//byte_offset -= readl(idma.regs + I2S_IDMA_XFER_SIZE);
	frames = bytes_to_frames(substream->runtime, byte_offset);
	if(frames >= runtime->buffer_size)
		frames = 0;
	
	spin_unlock_irqrestore(&prtd->lock, flags);
	return frames;
}



static int idma_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct idma_ctrl *prtd = substream->runtime->private_data;
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(cpu_dai);

	int ret = 0;

	debug_log("%s enter,cmd is %d\n",__FUNCTION__,cmd);
	
	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
		/* it will tranfer  */
		spdif_start_transfer(prtd, substream);
		break;

	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
		
		writel(0x0, spdif->dma_reg_base + DMA_REG_CTRL_OFF);	//stop
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}


static void idma_done(void *id, int bytes_xfer)
{
	struct snd_pcm_substream *substream = id;
	struct idma_ctrl *prtd = substream->runtime->private_data;

	if (prtd && (prtd->state & ST_RUNNING)){
		prtd->pos += prtd->sz;
		snd_pcm_period_elapsed(substream);
		spdif_start_transfer(prtd, substream);

	}
}

static int idma_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd;
	
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(cpu_dai);

	debug_log("%s enter\n",__FUNCTION__);

	snd_soc_set_runtime_hwparams(substream, &idma_hardware);

	prtd = kzalloc(sizeof(struct idma_ctrl), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);
	runtime->private_data = prtd;
	spdif->prtd = prtd;
	prtd->token = (void *) substream;

    debug_log("%s end\n",__FUNCTION__);

	return 0;
}

static int idma_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd = runtime->private_data;

	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	struct xilinx_spdif *spdif = snd_soc_dai_get_drvdata(cpu_dai);
	

	debug_log("%s enter\n",__FUNCTION__);

	writel(0x0, spdif->dma_reg_base + DMA_REG_CTRL_OFF);	//stop
	
	if (!prtd)
		pr_err("idma_close called with prtd == NULL\n");

	kfree(prtd);

	return 0;
}


static struct snd_pcm_ops axi_dma_ops = {
	.open		= idma_open,
	.close		= idma_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.trigger	= idma_trigger,
	.pointer	= idma_pointer,
	.mmap		= idma_mmap,
	.hw_params	= idma_hw_params,
	.hw_free	= idma_hw_free,
	.prepare	= idma_prepare,
};



static void axi_dma_free(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	substream = pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

	dma_free_writecombine(pcm->card->dev, buf->bytes,
				      buf->area, buf->addr);

	buf->area = NULL;
	buf->addr = 0;
}

static int preallocate_idma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size = idma_hardware.buffer_bytes_max;

	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	/* Assign PCM buffer pointers, dma_alloc_writecombine will return physical address */
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
	buf->area = dma_alloc_writecombine(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
	if(!buf->area)
		return -ENOMEM;
	buf->bytes = size;
	
	return 0;
}

static u64 idma_mask = DMA_BIT_MASK(32);

static int axi_dma_new(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;

	debug_log("%s enter\n",__FUNCTION__);
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &idma_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);

	if (pcm->streams[SNDRV_PCM_STREAM_CAPTURE].substream) {
		ret = preallocate_idma_buffer(pcm,
				SNDRV_PCM_STREAM_CAPTURE);
	}
	debug_log("%s exit\n",__FUNCTION__);
	return 0;
}

static irqreturn_t idma_irq_hanlder(int irqno, void *dev_id)
{
	struct xilinx_spdif *spdif = (struct xilinx_spdif *)dev_id;
	struct idma_ctrl *prtd = spdif->prtd;

	/* clear idma irq */
	writel(DMA_IOC_INT_SET, spdif->dma_reg_base + DMA_REG_STAT_OFF);

	idma_done(prtd->token,0);
	
	return IRQ_HANDLED;
}


static struct snd_soc_platform_driver xilinx_axi_dma_platform = {
	.ops = &axi_dma_ops,
	.pcm_new = axi_dma_new,
	.pcm_free = axi_dma_free,
};




static int axi_spdif_probe(struct platform_device *pdev)
{
	struct xilinx_spdif *spdif;
	struct resource *res1,*res2;
	void __iomem *base;
	int ret  = 0;
	int irq;

	debug_log("%s enter\n",__FUNCTION__);
	
	spdif = devm_kzalloc(&pdev->dev, sizeof(*spdif), GFP_KERNEL);
	if (!spdif)
		return -ENOMEM;

	platform_set_drvdata(pdev, spdif);

	res1 = platform_get_resource_byname(pdev, IORESOURCE_MEM, "SPDIF");
	if(!res1)
		dev_err(&pdev->dev, "no SPDIF reg found\n");
	base = devm_ioremap_resource(&pdev->dev, res1);
	if (IS_ERR(base))
		return PTR_ERR(base);

	spdif->reg_base = base;
	irq = platform_get_irq(pdev, 0);
	if (irq < 0){
		dev_err(&pdev->dev, "get irq faild\n");
		return irq;
	}
	spdif->clk = devm_clk_get(&pdev->dev, "axi");
	if (IS_ERR(spdif->clk))
		return PTR_ERR(spdif->clk);


	ret = clk_prepare_enable(spdif->clk);
	if (ret)
		return ret;



	//get dma info
	spdif->dma_irq = irq;
	res2 = platform_get_resource_byname(pdev, IORESOURCE_MEM, "DMA");
	if(!res2)
		dev_err(&pdev->dev, "no DMA reg found\n");

	base = devm_ioremap_resource(&pdev->dev, res2);
	if (IS_ERR(base))
		return PTR_ERR(base);
	spdif->dma_reg_base = base;
	
	ret = request_irq(spdif->dma_irq , idma_irq_hanlder, 0, "xlinx-spdif", spdif);
	if (ret < 0) {
		pr_err("fail to claim i2s irq , ret = %d\n", ret);
		return ret;
	}	
	debug_log("xilinx_spdif_probe: foud reg spdif base 0x%x,dma base 0x%x,irq %d\n",(u32)res1->start,(u32)res2->start,irq);


	

	ret = devm_snd_soc_register_component(&pdev->dev, &xilinx_spdif_component,
					 &xilinx_spdif_dai, 1);
	if (ret)
		goto err_clk_disable;


	ret = snd_soc_register_platform(&pdev->dev, &xilinx_axi_dma_platform);
	if(ret){
		dev_err(&pdev->dev, "register alsa soc platform driver failed\n");
		return -EINVAL;
	}			

    printk("adv7611 probe func is %x\n",(u32)xilinx_spdif_dai_probe);
	return 0;

err_clk_disable:
	clk_disable_unprepare(spdif->clk);
	kfree(spdif);
	
	return ret;
}

static int axi_spdif_dev_remove(struct platform_device *pdev)
{
	struct xilinx_spdif *spdif = platform_get_drvdata(pdev);

	clk_disable_unprepare(spdif->clk);

	return 0;
}

static const struct of_device_id xilinx_spdif_of_match[] = {
	{ .compatible = "xilinx,spdif-rx", },
	{},
};
MODULE_DEVICE_TABLE(of, xilinx_spdif_of_match);

static struct platform_driver xilinx_spdif_driver = {
	.driver = {
		.name = "xilinx-spdif-rx",
		.of_match_table = xilinx_spdif_of_match,
	},
	.probe = axi_spdif_probe,
	.remove = axi_spdif_dev_remove,
};
module_platform_driver(xilinx_spdif_driver);

MODULE_AUTHOR("MurphyXu");
MODULE_DESCRIPTION("xilinx SPDIF rx driver");
MODULE_LICENSE("GPL");
