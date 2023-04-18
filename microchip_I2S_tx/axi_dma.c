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
	int dma_irq;
	dma_addr_t fifo_base;
	void __iomem *dma_base;
};


static int dma_irq;
static dma_addr_t fifo_base;
static void __iomem *dma_base;
static irqreturn_t idma_irq_hanlder(int irqno, void *dev_id);
static int preallocate_idma_buffer(struct snd_pcm *pcm, int stream);

#define debug_log trace_printk

#define ST_RUNNING		(1<<0)
#define ST_OPENED		(1<<1)

#define MAX_IDMA_PERIOD (64 * 1024)
#define MAX_IDMA_BUFFER (64 * 1024)

static const struct snd_pcm_hardware idma_hardware = {
	.info =  SNDRV_PCM_INFO_INTERLEAVED |
		    /*SNDRV_PCM_INFO_NONINTERLEAVED |*/
		    SNDRV_PCM_INFO_BLOCK_TRANSFER |
		    SNDRV_PCM_INFO_MMAP |
		    SNDRV_PCM_INFO_MMAP_VALID |
		    SNDRV_PCM_INFO_PAUSE |
		    SNDRV_PCM_INFO_RESUME,
		    
	.formats = SNDRV_PCM_FMTBIT_S16_LE,
	.buffer_bytes_max = MAX_IDMA_BUFFER,
	.period_bytes_min = PAGE_SIZE,
	.period_bytes_max = MAX_IDMA_PERIOD,
	.periods_min = 1,
	.periods_max = 4,
};


#define DMA_CTR_REG_OFFSET 0x0
#define DMA_LEN_REG_OFFSET 0x8
#define DMA_SRC_REG_OFFSET 0x18
#define DMA_DEST_REG_OFFSET 0x10
#define DMA_CFG_REG_OFFSET 0x4
#define DMA_EXE_BYTES_OFFSET 0x108



static void spdif_start_transfer(struct idma_ctrl *prtd,
	struct snd_pcm_substream *substream)
{

	//struct idma_ctrl *prtd = substream->runtime->private_data;
	unsigned long count;
	
	if(prtd->pos >= prtd->end)
		prtd->pos = prtd->start;
		
	if(prtd->pos + prtd->period > prtd->end)
		count = prtd->end - prtd->pos;
	else
		count = prtd->period;

	prtd->sz = count;

	
	debug_log("pos 0x%llx, count %d\n",prtd->pos,count);

	
	// clear all configuration
	writel(0, prtd->dma_base + DMA_CFG_REG_OFFSET);
	// clear  channel
	writel(0, prtd->dma_base + DMA_CTR_REG_OFFSET);
	// claim channel
	writel(1, prtd->dma_base + DMA_CTR_REG_OFFSET);
	// set len
	writel(count, prtd->dma_base + DMA_LEN_REG_OFFSET);
	
	
	// set src
	//writeq(substream->runtime->dma_addr, prtd->dma_base + DMA_SRC_REG_OFFSET);
	writeq(prtd->pos, prtd->dma_base + DMA_SRC_REG_OFFSET);
	
	
	
	// set dest
	writel(prtd->fifo_base, prtd->dma_base + DMA_DEST_REG_OFFSET);
	// set dma burst to 16 bytes
	writel(0x44000008, prtd->dma_base + DMA_CFG_REG_OFFSET);
	// enable done irq and start
	writel(3 | (1 << 14), prtd->dma_base + DMA_CTR_REG_OFFSET);

	
}


static int idma_mmap(struct snd_soc_component *component,
		     struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	unsigned long size, offset;
	int ret;

	/* From snd_pcm_lib_mmap_iomem */
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	size = vma->vm_end - vma->vm_start;
	offset = vma->vm_pgoff << PAGE_SHIFT;
	ret = io_remap_pfn_range(vma, vma->vm_start,
			(runtime->dma_addr + offset) >> PAGE_SHIFT,
			size, vma->vm_page_prot);

	return ret;
}


static int idma_hw_params(struct snd_soc_component *component,struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd = substream->runtime->private_data;

	/* copy the dma info into runtime management */
	snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	/* get the max dma buffer size */
	runtime->dma_bytes = params_buffer_bytes(params);

	prtd->start = prtd->pos = runtime->dma_addr;
	prtd->period = params_period_bytes(params);
	prtd->end = runtime->dma_addr + runtime->dma_bytes;

	debug_log("idma_hw_params: runtime period %ld, dma addr 0x%llx, dma byte 0x%lx\n",prtd->period, \
			runtime->dma_addr,runtime->dma_bytes);
	
	return 0;
}
static int idma_hw_free(struct snd_soc_component *component,struct snd_pcm_substream *substream)
{
	snd_pcm_set_runtime_buffer(substream, NULL);

	return 0;
}
static int idma_prepare(struct snd_soc_component *component,struct snd_pcm_substream *substream)
{

	struct idma_ctrl *prtd = substream->runtime->private_data;
	debug_log("%s enter\n",__FUNCTION__);

	prtd->pos = prtd->start;

	return 0;
}



static snd_pcm_uframes_t
	idma_pointer(struct snd_soc_component *component,struct snd_pcm_substream *substream)
{
	struct idma_ctrl *prtd = substream->runtime->private_data;
	unsigned int byte_offset,count;
	snd_pcm_sframes_t frames;
	unsigned long flags;

	
	spin_lock_irqsave(&prtd->lock, flags);
	count = readl(prtd->dma_base + DMA_EXE_BYTES_OFFSET) ;

	byte_offset = prtd->pos - prtd->start;

	byte_offset = byte_offset +  ( prtd->sz - count );
		
	frames = bytes_to_frames(substream->runtime, byte_offset);
	
	/* must set frame to 0 if reach to top, otherwise alsa will report invalid position error */
	if(frames >= substream->runtime->buffer_size)
		frames = 0;
	
	spin_unlock_irqrestore(&prtd->lock, flags);
	
	debug_log("dma counter %ld, byte_offset %ld, pos 0x%llx, start 0x%llx\n",count,byte_offset,prtd->pos, prtd->start);
	return frames;
}



static int idma_trigger(struct snd_soc_component *component,struct snd_pcm_substream *substream, int cmd)
{
	struct idma_ctrl *prtd = substream->runtime->private_data;

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
		
		// clear all configuration
		writel(0, prtd->dma_base + DMA_CFG_REG_OFFSET);
		// clear  channel
		writel(0, prtd->dma_base + DMA_CTR_REG_OFFSET);

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
		
		snd_pcm_period_elapsed(substream);
		
		
		// must add lock on smp system, otherwise idma_pointer will return wrong point
		spin_lock(&prtd->lock);
		prtd->pos += prtd->sz;
		spdif_start_transfer(prtd, substream);
		spin_unlock(&prtd->lock);

	}
}

static int idma_open(struct snd_soc_component *component,struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct idma_ctrl *prtd;

	struct snd_card *card = substream->pcm->card;
	//struct snd_pcm *pcm = runtime->pcm;
	int ret = 0;

	debug_log("%s enter\n",__FUNCTION__);

	snd_soc_set_runtime_hwparams(substream, &idma_hardware);

	prtd = kzalloc(sizeof(struct idma_ctrl), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;


	prtd->dma_irq = dma_irq;
	prtd->fifo_base = fifo_base;
	prtd->dma_base = dma_base;
	
	ret = request_irq(prtd->dma_irq, idma_irq_hanlder, 0, "idma-i2s", prtd);
	if (ret < 0) {
		pr_err("fail to claim dam irq , ret = %d\n", ret);
		kfree(prtd);
		return ret;
	}

	spin_lock_init(&prtd->lock);
	runtime->private_data = prtd;
	prtd->token = (void *) substream;




	if (dma_set_mask_and_coherent(card->dev,
				      DMA_BIT_MASK(64))) {
		if (dma_set_mask_and_coherent(card->dev,
					      DMA_BIT_MASK(32))) {
			dev_warn(card->dev,
				 "No suitable DMA available\n");
			return -ENOMEM;
		}
	}
	
	
	if (substream) {
		ret = preallocate_idma_buffer(substream->pcm,
				SNDRV_PCM_STREAM_PLAYBACK);
	}
	
	
	
	return ret;
}

static int idma_close(struct snd_soc_component *component,struct snd_pcm_substream *substream)
{
	struct idma_ctrl *prtd = substream->runtime->private_data;
	struct snd_dma_buffer *buf;
	struct snd_card *card = substream->pcm->card;

	debug_log("%s enter\n",__FUNCTION__);
	// clear all configuration
	writel(0, prtd->dma_base + DMA_CFG_REG_OFFSET);
	// clear  channel
	writel(0, prtd->dma_base + DMA_CTR_REG_OFFSET);
	
	
	if (!substream)
		return -ENOMEM;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return -ENOMEM;

	
	dma_free_coherent(card->dev, buf->bytes,
				      buf->area, buf->addr);

	buf->area = NULL;
	buf->addr = 0;





	free_irq(prtd->dma_irq, prtd);


	if (!prtd)
		pr_err("idma_close called with prtd == NULL\n");

	kfree(prtd);

	return 0;
}





static void axi_dma_free(struct snd_soc_component *component,struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;

	substream = pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream;
	if (!substream)
		return;

	buf = &substream->dma_buffer;
	if (!buf->area)
		return;

	
	dma_free_coherent(pcm->card->dev, buf->bytes,
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
	buf->area = dma_alloc_coherent(pcm->card->dev, size, &buf->addr, GFP_KERNEL);
	if(!buf->area){
		printk("failed to alloc dma buff\n");
		return -ENOMEM;
	}
	
	printk("dma memory PHY addr 0x%llx,size of dma size %d\n",buf->addr,sizeof(buf->addr));
	
	buf->bytes = size;
	
	return 0;
}

static u64 idma_mask = DMA_BIT_MASK(32);

static int axi_dma_new(struct snd_soc_component *component,struct snd_soc_pcm_runtime *rtd)
{
	struct snd_card *card = rtd->card->snd_card;
	struct snd_pcm *pcm = rtd->pcm;
	int ret = 0;



	if (dma_set_mask_and_coherent(card->dev,
				      DMA_BIT_MASK(64))) {
		if (dma_set_mask_and_coherent(card->dev,
					      DMA_BIT_MASK(32))) {
			dev_warn(card->dev,
				 "No suitable DMA available\n");
			return -ENOMEM;
		}
	}
	
	
	/*
	if (!card->dev->dma_mask)
		card->dev->dma_mask = &idma_mask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = DMA_BIT_MASK(32);
	*/

	if (pcm->streams[SNDRV_PCM_STREAM_PLAYBACK].substream) {
		ret = preallocate_idma_buffer(pcm,
				SNDRV_PCM_STREAM_PLAYBACK);
	}

	return ret;
}

static irqreturn_t idma_irq_hanlder(int irqno, void *dev_id)
{
	struct idma_ctrl *prtd = dev_id;
	u32 reg;

	// clear irq status
	//reg = readl(prtd->dma_base + DMA_CTR_REG_OFFSET);
	//writel(reg| (1<< 30),prtd->dma_base + DMA_CTR_REG_OFFSET);

	idma_done(prtd->token,0);
	
	return IRQ_HANDLED;
}



/*


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


static struct snd_soc_platform_driver mx_axi_dma_platform = {
	.ops = &axi_dma_ops,
	.pcm_new = axi_dma_new,
	.pcm_free = axi_dma_free,
};
*/




static const struct snd_soc_component_driver mx_idma_platform = {
	.open		= idma_open,
	.close		= idma_close,
	.trigger	= idma_trigger,
	.pointer	= idma_pointer,
	/* .mmap		= idma_mmap, */
	.hw_params	= idma_hw_params,
	.hw_free	= idma_hw_free,
	.prepare	= idma_prepare,
	/* .pcm_construct	= axi_dma_new, */
	/* .pcm_destruct	= axi_dma_free, */
};

void mx_idma_init(struct platform_device *pdev, int irq,dma_addr_t fifo_addr,void __iomem *dma_reg)
{
	dma_irq = irq;
	fifo_base = fifo_addr;
	dma_base = dma_reg;
	if( devm_snd_soc_register_component(&pdev->dev, &mx_idma_platform,
					       NULL, 0)){
		dev_err(&pdev->dev, "failed to register dma driver\n");
	}
}

EXPORT_SYMBOL_GPL(mx_idma_init);

static int mx_idma_release(struct platform_device *pdev)
{
	return 0;
}



// this driver is only support new kernel
MODULE_AUTHOR("Murphy Xu");
MODULE_DESCRIPTION("I2S dma driver");
MODULE_LICENSE("GPL");


