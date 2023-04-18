// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for mx I2S controller
 *
 * Copyright (C) 2015 mx Corporation
 *
 * Author: Cyrille Pitchen <cyrille.pitchen@mx.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/mfd/syscon.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/dmaengine_pcm.h>

/*


sound {
	compatible = "mikroe,mikroe-proto";
	model = "wm8731 @ PolarFire";
	i2s-controller = <&i2s0>;
	audio-codec = <&wm8731>;
	dai-format = "i2s";
	};

&i2c0 {
	#address-cells = <1>;
	#size-cells = <0>;
	wm8731: wm8731@1a {
		compatible = "wlf,wm8731";
		reg = <0x1a>;
	};
};

	

i2s0: i2s@f8050000 {
	compatible = "mx,mx-i2s";
	reg = <0x60030000 0x40000>,
		<0x03000000 0x1000>;
	reg-names = "fifo","dma";
	interrupt-parent = <&plic>;
	interrupts = <5>;

	status = "okay";
};



*/



struct mx_i2s_dev;

struct mx_i2s_dev {
	struct device				*dev;
	struct regmap				*regmap;
	struct clk				*pclk;
	struct clk				*gclk;
	unsigned int				fmt;
	int					clk_use_no;
	int dma_irq;

};



#define MX_I2S_RATES		SNDRV_PCM_RATE_48000

#define MX_I2S_FORMATS	SNDRV_PCM_FMTBIT_S16_LE


static int mx_i2s_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct mx_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	dev->fmt = fmt;
	return 0;
}

static int mx_i2s_prepare(struct snd_pcm_substream *substream,
			     struct snd_soc_dai *dai)
{

	return 0;
}


static int mx_i2s_hw_params(struct snd_pcm_substream *substream,
			       struct snd_pcm_hw_params *params,
			       struct snd_soc_dai *dai)
{
	struct mx_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);




	switch (dev->fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;

	default:
		dev_err(dev->dev, "unsupported bus format\n");
		return -EINVAL;
	}


	switch (dev->fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* codec is slave, so cpu is master */
		break;

	default:
		dev_err(dev->dev, "unsupported master/slave mode\n");
		return -EINVAL;
	}


	switch (params_channels(params)) {
	case 1:
		dev_err(dev->dev, "unsupported number of audio channels: 1\n");
		break;
	case 2:
		break;
	default:
		dev_err(dev->dev, "unsupported number of audio channels\n");
		return -EINVAL;
	}
	

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		break;

	default:
		dev_err(dev->dev, "unsupported PCM format\n");
		return -EINVAL;
	}
	return 0;
}

static int mx_i2s_trigger(struct snd_pcm_substream *substream, int cmd,
			     struct snd_soc_dai *dai)
{




	int err = 0;
	struct mx_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:

		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:

		break;
	default:
		dev_err(dev->dev, "unsupported operation\n");
		return -EINVAL;
	}

	return err;
}

static const struct snd_soc_dai_ops mx_i2s_dai_ops = {
	.prepare	= mx_i2s_prepare,
	.trigger	= mx_i2s_trigger,
	.hw_params	= mx_i2s_hw_params,
	.set_fmt	= mx_i2s_set_dai_fmt,
};

static int mx_i2s_dai_probe(struct snd_soc_dai *dai)
{
	struct mx_i2s_dev *dev = snd_soc_dai_get_drvdata(dai);

	// snd_soc_dai_init_dma_data(dai, &dev->playback);
	return 0;
}

static struct snd_soc_dai_driver mx_i2s_dai = {
	.probe	= mx_i2s_dai_probe,
	.playback = {
		.channels_min = 2,
		.channels_max = 2,
		.rates = MX_I2S_RATES,
		.formats = MX_I2S_FORMATS,
	},
	.ops = &mx_i2s_dai_ops,

		/*
	.symmetric_rate = 1,
	.symmetric_sample_bits = 1,
	*/
};

static const struct snd_soc_component_driver mx_i2s_component = {
	.name	= "mx-i2s",
};


static const struct of_device_id mx_i2s_dt_ids[] = {
	{
		.compatible = "mx,mx-i2s",
	},

	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, mx_i2s_dt_ids);


extern void mx_idma_init(struct platform_device *pdev, int irq,dma_addr_t fifo_addr,void __iomem *dma_reg);
static int mx_i2s_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *match;
	struct mx_i2s_dev *dev;
	struct resource *fifo,*dma;
	struct regmap *regmap;
	void __iomem *base;
	int irq;
	int err;



	/* Get memory for driver data. */
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	


	fifo = platform_get_resource_byname(pdev, IORESOURCE_MEM, "fifo");
	if(!fifo){
		dev_err(&pdev->dev, "no FIFO addr found\n");
		return -ENODEV;
	}

	dev_info(&pdev->dev, "get FIFO address 0x%x\n", fifo->start);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0){
		dev_err(&pdev->dev, "get irq faild\n");
		return irq;
	}
	dev->dma_irq = irq;
	dma = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dma");
	if(!dma)
		dev_err(&pdev->dev, "no SPDIF reg found\n");

	
	
	base = devm_ioremap_resource(&pdev->dev, dma);
	if (IS_ERR(base))
		return PTR_ERR(base);




	dev->dev = &pdev->dev;
	platform_set_drvdata(pdev, dev);

	err = devm_snd_soc_register_component(&pdev->dev,
					      &mx_i2s_component,
					      &mx_i2s_dai, 1);
	if (err) {
		dev_err(&pdev->dev, "failed to register DAI: %d\n", err);
		return err;
	}

	mx_idma_init(pdev,dev->dma_irq,fifo->start,base);
	
/*

	if (of_property_match_string(np, "dma-names", "rx-tx") == 0)
		pcm_flags |= SND_DMAENGINE_PCM_FLAG_HALF_DUPLEX;
	err = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, pcm_flags);
	if (err) {
		dev_err(&pdev->dev, "failed to register PCM: %d\n", err);
		return err;
	}
	
*/

	dev_info(&pdev->dev, "register I2S OK\n");
	return 0;
}

static int mx_i2s_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver mx_i2s_driver = {
	.driver		= {
		.name	= "mx_i2s",
		.of_match_table	= of_match_ptr(mx_i2s_dt_ids),
	},
	.probe		= mx_i2s_probe,
	.remove		= mx_i2s_remove,
};
module_platform_driver(mx_i2s_driver);

MODULE_DESCRIPTION("I2S Controller driver");
MODULE_AUTHOR("Murphy xu");
MODULE_LICENSE("GPL v2");
