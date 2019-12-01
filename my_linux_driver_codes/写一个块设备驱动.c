#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/version.h>
/*
* A simple block device driver based on memory
*
* Copyright 2008 –
* Zhaolei <zhaolei@cn.fujitsu.com>
*
* Sample for using:
* Create device file (first time only):
*     Note: If your system have udev, it can create device file for you in time
*           of lsmod and fdisk automatically.
*           Otherwise you need to create them yourself by following steps.
*     mknod /dev/simp_blkdev   b 72 0
*     mknod /dev/simp_blkdev1 b 72 1
*     mknod /dev/simp_blkdev2 b 72 2
*
* Create dirs for test (first time only):
*     mkdir /mnt/temp1/ # first time only
*     mkdir /mnt/temp2/ # first time only
*
* Run it:
*     make
*     insmod simp_blkdev.ko
*     # or insmod simp_blkdev.ko size=numK/M/G/T
*     fdisk /dev/simp_blkdev # create 2 patitions
*     mkfs.ext3 /dev/simp_blkdev1
*     mkfs.ext3 /dev/simp_blkdev2
*     mount /dev/simp_blkdev1 /mnt/temp1/
*     mount /dev/simp_blkdev2 /mnt/temp2/
*     # play in /mnt/temp1/ and /mnt/temp2/
*     umount /mnt/temp1/
*     umount /mnt/temp2/
*     rmmod simp_blkdev.ko
*
*/
#define SIMP_BLKDEV_DEVICEMAJOR COMPAQ_SMART2_MAJOR
#define SIMP_BLKDEV_DISKNAME "simp_blkdev"
#define SIMP_BLKDEV_SECTORSHIFT (9)
#define SIMP_BLKDEV_SECTORSIZE (1ULL<<SIMP_BLKDEV_SECTORSHIFT)
#define SIMP_BLKDEV_SECTORMASK (~(SIMP_BLKDEV_SECTORSIZE-1))
/* usable partitions is SIMP_BLKDEV_MAXPARTITIONS – 1 */
#define SIMP_BLKDEV_MAXPARTITIONS (64)
#define SIMP_BLKDEV_DATASEGORDER (2)
#define SIMP_BLKDEV_DATASEGSHIFT (PAGE_SHIFT + SIMP_BLKDEV_DATASEGORDER)
#define SIMP_BLKDEV_DATASEGSIZE (PAGE_SIZE << SIMP_BLKDEV_DATASEGORDER)
#define SIMP_BLKDEV_DATASEGMASK (~(SIMP_BLKDEV_DATASEGSIZE-1))
static struct request_queue *simp_blkdev_queue;
static struct gendisk *simp_blkdev_disk;
static struct radix_tree_root simp_blkdev_data;
DEFINE_MUTEX(simp_blkdev_datalock); /* protects the disk data op */
static char *simp_blkdev_param_size = "16M";
module_param_named(size, simp_blkdev_param_size, charp, S_IRUGO);
static unsigned long long simp_blkdev_bytes;
static int simp_blkdev_trans_oneseg(struct page *start_page,
                                    unsigned long offset, void *buf, unsigned int len, int dir)
{
	unsigned int done_cnt;
	struct page *this_page;
	unsigned int this_off;
	unsigned int this_cnt;
	void *dsk_mem;
	done_cnt = 0;
	while (done_cnt < len) {
		/* iterate each page */
		this_page = start_page + ((offset + done_cnt) >> PAGE_SHIFT);
		this_off = (offset + done_cnt) & ~PAGE_MASK;
		this_cnt = min(len – done_cnt, (unsigned int)PAGE_SIZE
		               – this_off);
		dsk_mem = kmap(this_page);
		if (!dsk_mem) {
			printk(KERN_ERR SIMP_BLKDEV_DISKNAME
			       ": map device page failed: %pn", this_page);
			return -ENOMEM;
		}
		dsk_mem += this_off;
		if (!dir)
			memcpy(buf + done_cnt, dsk_mem, this_cnt);
		else
			memcpy(dsk_mem, buf + done_cnt, this_cnt);
		kunmap(this_page);
		done_cnt += this_cnt;
	}
	return 0;
}
static int simp_blkdev_trans(unsigned long long dsk_offset, void *buf,
                             unsigned int len, int dir)
{
	unsigned int done_cnt;
	struct page *this_first_page;
	unsigned int this_off;
	unsigned int this_cnt;
	done_cnt = 0;
	while (done_cnt < len) {
		/* iterate each data segment */
		this_off = (dsk_offset + done_cnt) & ~SIMP_BLKDEV_DATASEGMASK;
		this_cnt = min(len – done_cnt,
		               (unsigned int)SIMP_BLKDEV_DATASEGSIZE – this_off);
		mutex_lock(&simp_blkdev_datalock);
		this_first_page = radix_tree_lookup(&simp_blkdev_data,
		                                    (dsk_offset + done_cnt) >> SIMP_BLKDEV_DATASEGSHIFT);
		if (!this_first_page) {
			if (!dir) {
				memset(buf + done_cnt, 0, this_cnt);
				goto trans_done;
			}
			/* prepare new memory segment for write */
			this_first_page = alloc_pages(
			                      GFP_KERNEL | __GFP_ZERO | __GFP_HIGHMEM,
			                      SIMP_BLKDEV_DATASEGORDER);
			if (!this_first_page) {
				printk(KERN_ERR SIMP_BLKDEV_DISKNAME
				       ": allocate page failedn");
				mutex_unlock(&simp_blkdev_datalock);
				return -ENOMEM;
			}
			this_first_page->index = (dsk_offset + done_cnt)
			                         >> SIMP_BLKDEV_DATASEGSHIFT;
			if (IS_ERR_VALUE(radix_tree_insert(&simp_blkdev_data,
			                                   this_first_page->index, this_first_page))) {
				printk(KERN_ERR SIMP_BLKDEV_DISKNAME
				       ": insert page to radix_tree failed"
				       " seg=%lun", this_first_page->index);
				__free_pages(this_first_page,
				             SIMP_BLKDEV_DATASEGORDER);
				mutex_unlock(&simp_blkdev_datalock);
				return -EIO;
			}
		}
		if (IS_ERR_VALUE(simp_blkdev_trans_oneseg(this_first_page,
		                 this_off, buf + done_cnt, this_cnt, dir))) {
			mutex_unlock(&simp_blkdev_datalock);
			return -EIO;
		}
trans_done:
		mutex_unlock(&simp_blkdev_datalock);
		done_cnt += this_cnt;
	}
	return 0;
}
static int simp_blkdev_make_request(struct request_queue *q, struct bio *bio)
{
	int dir;
	unsigned long long dsk_offset;
	struct bio_vec *bvec;
	int i;
	void *iovec_mem;
	switch (bio_rw(bio)) {
	case READ:
	case READA:
		dir = 0;
		break;
	case WRITE:
		dir = 1;
		break;
	default:
		printk(KERN_ERR SIMP_BLKDEV_DISKNAME
		       ": unknown value of bio_rw: %lun", bio_rw(bio));
		goto bio_err;
	}
	if ((bio->bi_sector << SIMP_BLKDEV_SECTORSHIFT) + bio->bi_size
	        > simp_blkdev_bytes) {
		printk(KERN_ERR SIMP_BLKDEV_DISKNAME
		       ": bad request: block=%llu, count=%un",
		       (unsigned long long)bio->bi_sector, bio->bi_size);
		goto bio_err;
	}
	dsk_offset = bio->bi_sector << SIMP_BLKDEV_SECTORSHIFT;
	bio_for_each_segment(bvec, bio, i) {
		iovec_mem = kmap(bvec->bv_page) + bvec->bv_offset;
		if (!iovec_mem) {
			printk(KERN_ERR SIMP_BLKDEV_DISKNAME
			       ": map iovec page failed: %pn", bvec->bv_page);
			goto bio_err;
		}
		if (IS_ERR_VALUE(simp_blkdev_trans(dsk_offset, iovec_mem,
		                                   bvec->bv_len, dir)))
			goto bio_err;
		kunmap(bvec->bv_page);
		dsk_offset += bvec->bv_len;
	}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	bio_endio(bio, bio->bi_size, 0);
#else
	bio_endio(bio, 0);
#endif
	return 0;
bio_err:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
	bio_endio(bio, 0, -EIO);
#else
	bio_endio(bio, -EIO);
#endif
	return 0;
}
static int simp_blkdev_getgeo(struct block_device *bdev,
                              struct hd_geometry *geo)
{
	/*
	    * capacity heads sectors cylinders
	    * 0~16M 1 1 0~32768
	    * 16M~512M 1 32 1024~32768
	    * 512M~16G 32 32 1024~32768
	    * 16G~… 255 63 2088~…
	    */
	if (simp_blkdev_bytes < 16 * 1024 * 1024) {
		geo->heads = 1;
		geo->sectors = 1;
	} else if (simp_blkdev_bytes < 512 * 1024 * 1024) {
		geo->heads = 1;
		geo->sectors = 32;
	} else if (simp_blkdev_bytes < 16ULL * 1024 * 1024 * 1024) {
		geo->heads = 32;
		geo->sectors = 32;
	} else {
		geo->heads = 255;
		geo->sectors = 63;
	}
	geo->cylinders = simp_blkdev_bytes >> SIMP_BLKDEV_SECTORSHIFT
	                 / geo->heads / geo->sectors;
	return 0;
}
struct block_device_operations simp_blkdev_fops = {
	.owner = THIS_MODULE,
	.getgeo = simp_blkdev_getgeo,
};
void free_diskmem(void)
{
	unsigned long long next_seg;
	struct page *seglist[64];
	int listcnt;
	int i;
	next_seg = 0;
	do {
		listcnt = radix_tree_gang_lookup(&simp_blkdev_data,
		                                 (void **)seglist, next_seg, ARRAY_SIZE(seglist));
		for (i = 0; i < listcnt; i++) {
			next_seg = seglist[i]->index;
			radix_tree_delete(&simp_blkdev_data, next_seg);
			__free_pages(seglist[i], SIMP_BLKDEV_DATASEGORDER);
		}
		next_seg++;
	} while (listcnt == ARRAY_SIZE(seglist));
}
int getparam(void)
{
	char unit;
	char tailc;
	if (sscanf(simp_blkdev_param_size, "%llu%c%c", &simp_blkdev_bytes,
	           &unit, &tailc) != 2) {
		return -EINVAL;
	}
	if (!simp_blkdev_bytes)
		return -EINVAL;
	switch (unit) {
	case ‘g’:
	case ‘G’:
		simp_blkdev_bytes <<= 30;
		break;
	case ‘m’:
	case ‘M’:
		simp_blkdev_bytes <<= 20;
		break;
	case ‘k’:
	case ‘K’:
		simp_blkdev_bytes <<= 10;
		break;
	case ‘b’:
	case ‘B’:
		break;
	default:
		return -EINVAL;
	}
	/* make simp_blkdev_bytes fits sector’s size */
	simp_blkdev_bytes = (simp_blkdev_bytes + SIMP_BLKDEV_SECTORSIZE – 1)
	                    & SIMP_BLKDEV_SECTORMASK;
	return 0;
}
static int __init simp_blkdev_init(void)
{
	int ret;
	ret = getparam();
	if (IS_ERR_VALUE(ret))
		goto err_getparam;
	simp_blkdev_queue = blk_alloc_queue(GFP_KERNEL);
	if (!simp_blkdev_queue) {
		ret = -ENOMEM;
		goto err_alloc_queue;
	}
	blk_queue_make_request(simp_blkdev_queue, simp_blkdev_make_request);
	simp_blkdev_disk = alloc_disk(SIMP_BLKDEV_MAXPARTITIONS);
	if (!simp_blkdev_disk) {
		ret = -ENOMEM;
		goto err_alloc_disk;
	}
	INIT_RADIX_TREE(&simp_blkdev_data, GFP_KERNEL);
	strcpy(simp_blkdev_disk->disk_name, SIMP_BLKDEV_DISKNAME);
	simp_blkdev_disk->major = SIMP_BLKDEV_DEVICEMAJOR;
	simp_blkdev_disk->first_minor = 0;
	simp_blkdev_disk->fops = &simp_blkdev_fops;
	simp_blkdev_disk->queue = simp_blkdev_queue;
	set_capacity(simp_blkdev_disk,
	             simp_blkdev_bytes >> SIMP_BLKDEV_SECTORSHIFT);
	add_disk(simp_blkdev_disk);
	return 0;
err_alloc_disk:
	blk_cleanup_queue(simp_blkdev_queue);
err_alloc_queue:
err_getparam:
	return ret;
}
static void __exit simp_blkdev_exit(void)
{
	del_gendisk(simp_blkdev_disk);
	free_diskmem();
	put_disk(simp_blkdev_disk);
	blk_cleanup_queue(simp_blkdev_queue);
}
module_init(simp_blkdev_init);
module_exit(simp_blkdev_exit);
MODULE_LICENSE("GPL");
