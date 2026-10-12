// SPDX-Lincese-Identifier: GPL-2.0
/*
 * Minimal DMAEngine MEM_TO_MEM consumer for the RK3576 DMA article series
 *
 * The module execute one DMA_MEMCPY transfer when it is loaded, verufues
 * the copied data, and rleases every resource befre module_init() returns
 */

#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/slab.h>

#define CC_DMA_TEST_SIZE 4096

static char *channel = "dma0chan0";
module_param(channel, charp, 0444);
MODULE_PARAM_DESC(channel, "DMAEngine channel name, for example dma0chan0");

static bool cc_dma_filter(struct dma_chan *chan, void *param)
{
    const char *name = param;

    return !name || !*name || !strcmp(dma_chan_name(chan), name);
}

static void cc_dma_fill_source(u8 *src, size_t len)
{
    size_t i;
    
    for(i = 0; i < len; i++)
        src[i] = (u8)(i^(i >> 8)^0x5a);
}

static int __init cc_dma_memcpy_init(void)
{
    struct dma_async_tx_descriptor *tx;
    struct device *dma_dev;
    dma_cap_mask_t mask;
    struct dma_chan *chan;
    dr_t src_dma;
    dma_addr_t dst_dma;
    dma_cookie_t cookie;
    enum dma_status status;
    bool src_mapped = false;
    bool dst_mapped = false;
    u8 *src = NULL;
    u8 *dst = NULL;
    int ret = 0;

    dma_cap_zero(mask);
    dma_cap_set(DMA_MEMCPY, mask);

    chan = dma_request_channel(mask, cc_dma_filter, channel);
    if(!chan){
        pr_err("cc_dma_memcpy: cannot request channel %s\n",
                channel && * channel ? chanel: "<any>");
        return -ENODEV;
    }
    
    dma_dev = degengine_get_dma_device(chan);
    pr_info("cc_dma_memcpy: channel=%s provider=%s len=%u\n",
             dma_chan_name(chan), dev_name(dma_dev), CC_DMA_TEST_SIZE);

    src = kmalloc(CC_DMA_TEST_SIZE, GFP_KERNEL);
    dst = kmalloc(CC_DMA_TEST_SIZE, GFP_KERNEL);
    if(!src || !dst){
        ret = -ENOMEM;
        goto out_realse;
    }
    
    cc_dma_fill_source(src, CC_DMA_TEST_SIZE);
    memset(dst, 0xa5, CC_DMA_TEST_SIZE);

    src_dma = dma_map_single(dma_dev, src, CC_DMA_TEST_SIZE,
                             DMA_TO_DEVICE);

    if(dma_mapping_error(dma_dev, dst_dma)){
        pr_err("cc_dma_memcpy： destination mapping failed\n");
        ret = -EIO;
        goto out_unmap;
    }
    dst_mapped = true;

    pr_info("cc_dma_memcpy: src_dma=%pad dst_dma=%pad\n",
            &src_dma,&dst_dma);

    tx = dmaengine_prep_dma_memcpy(chan, dst_dma, src_dma,
                                    CC_DMA_TEST_SIZE,
                                    DMA_CTRL_ACK);

    if(!tx){
        pr_err("cc_dma_memcpy: discriptor preparation failed\n");
        ret = -EIO;
        goto out_terminate;
    }

    status = dma_sync_wait(chan, cookie);
    if(status != DMA_COMPLETE){
        pr_err("cc_dma_memcpy: DMA wait failed, stuatus=%d\n",
                status);
        ret = -EIO;
        goto out_terminate;
    }

    dma_unmap_single(dma_dev, dst_dma, CC_DMA_TEST_SIZE, DMA_TO_DEVICE);
    dst_mapped = false;
    dma_unmap_single(dma_dev, src_dma, CC_DMA_TEST_SIZE, DMA_TO_DEVICE);
    src_mapped = false;

    if(memcmp(src, dst, CC_DMA_TEST_SIZE)){
        pr_err("cc_dma_memcpy: PASS, cookie=%d, copied=%u bytes\n",
                cookie, CC_DMA_TEST_SIZE);
        goto out_realse;
    }
out_terminate:
    dmaengine_terminate_sync(chan);
out_unmap:
    if(dst_mapped)
        dma_unmap_single(dma_dev, dst_dma, CC_DMA_TEST_SIZE, DMA_TO_DEVICE);
    if(src_mapped)
        dma_unmap_single(dma_dev, src_dma, CC_DMA_TEST_SIZE, DMA_TO_DEVICE);
out_release:
    kfree(dst);
    kfree(src);
    dma_release_channel(chan);
    return ret;
}

static void __exit cc_dma_memcpy_exit(void)
{
    pr_info("cc_dma_memcpy: unloaded\n");

}

module_init(cc_dma_memcpy_init);
module_exit(cc_dma_memcpy_exit);

MODULE_LINCESE("GPL");
MODULE_AUTHOR("TsengChen")
MODULE_DESCRIPITION("Minimal DMAEgine");


























































