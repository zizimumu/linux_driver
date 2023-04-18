#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>


static int xilinx_dai_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{


	return 0;
}

static struct snd_soc_ops xilinx_dai_ops = {
	.hw_params = xilinx_dai_hw_params,
};

static int xilinx_dai_init(struct snd_soc_pcm_runtime *rtd)
{
	return 0;
}



static int xilinx_card_remove(struct snd_soc_card *card)
{
	return 0;
}



static struct snd_soc_dai_link xilinx_dai = {
	.name = "adv7611_dai",
	.stream_name = "adv7611 hdmi PCM",
	
	.codec_dai_name = "adv7611-hdmi-hifi",
	

	.init = xilinx_dai_init,
	.ops = &xilinx_dai_ops,
	.dai_fmt = SND_SOC_DAIFMT_SPDIF,
};

static struct snd_soc_card snd_soc_xilinx_card = {
	.name = "adv7611-HDMI",
	.owner = THIS_MODULE,
	.dai_link = &xilinx_dai,
	.num_links = 1,

	.remove = xilinx_card_remove,

	.fully_routed = true,
};



static int xilinx_snd_driver_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct device_node *np = pdev->dev.of_node;

	/* it will create sound_card */
    snd_soc_xilinx_card.dev = &pdev->dev;


    	xilinx_dai.codec_of_node = of_parse_phandle(np,
						"audio-codec", 0);
	if (!xilinx_dai.codec_of_node) {
		dev_err(&pdev->dev,
			"Property 'audio-codec' missing or invalid\n");
		ret = -EINVAL;
		return ret;
	}

	xilinx_dai.cpu_of_node = of_parse_phandle(np,
			"spdif-controller", 0);
	if (!xilinx_dai.cpu_of_node) {
		dev_err(&pdev->dev,
			"Property 'spdif-controller' missing or invalid\n");
		ret = -EINVAL;
		return ret;
	}

	xilinx_dai.platform_of_node = xilinx_dai.cpu_of_node;


    
	ret = snd_soc_register_card(&snd_soc_xilinx_card);
	if (ret) {
		dev_err(&pdev->dev, "snd_soc_register_card failed (%d)\n",
			ret);
	}

	printk("xilinx_snd_driver_probe done......\n");
	
	return 0;

}


static int xilinx_snd_driver_remove(struct platform_device *pdev)
{


	return 0;
}



static const struct of_device_id xilinx_snd_of_match[] = {
	{ .compatible = "adv7611-audio-card", },
	{},
};
MODULE_DEVICE_TABLE(of, xilinx_spdif_of_match);

static struct platform_driver xilinx_snd_driver = {
	.driver = {
		.name = "xilinx-snd",
		.of_match_table = xilinx_snd_of_match,
	},
	.probe = xilinx_snd_driver_probe,
	.remove = xilinx_snd_driver_remove,
};
module_platform_driver(xilinx_snd_driver);

MODULE_AUTHOR("MurphyXu");
MODULE_DESCRIPTION("xilinx sound card driver");
MODULE_LICENSE("GPL");

