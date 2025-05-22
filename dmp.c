// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */

#include <linux/device-mapper.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/types.h>

#define DM_MSG_PREFIX "dmp"

typedef struct {
	unsigned long long rrq_bsize_total;
	unsigned long long wrq_bsize_total;
	unsigned long long rq_bsize_total;
	unsigned int rrq_num;
	unsigned int wrq_num;
	unsigned int rq_num;
} dmp_stats_t;

typedef struct {
	dmp_stats_t* stats;
	struct dm_dev* dev;
} dmp_dev_handle_t;

static int dmp_map(struct dm_target* ti, struct bio* bio)
{
	unsigned long long bsize;
	unsigned long long avg_bsize;
	dmp_stats_t* stats;
	dmp_dev_handle_t* dh;

	dh = ti->private;
	stats = dh->stats;
	bsize = bio->bi_iter.bi_size;
	bio->bi_bdev = dh->dev->bdev;
	stats->rq_num++;
	stats->rq_bsize_total = stats->rq_bsize_total + bsize;

	pr_info("Requests num: %u\n", stats->rq_num);
	pr_info("Received bio with block of size: %llu\n", bsize);
	pr_info("Total block size: %llu\n", stats->rq_bsize_total);

	switch (bio_op(bio)) {
	case REQ_OP_READ:
		stats->rrq_num++;
		stats->rrq_bsize_total += bsize;
		avg_bsize = stats->rrq_bsize_total / stats->rrq_num;
		pr_info("Current read requests num: %u\n", stats->rrq_num);
		pr_info("Current average read block size: %llu\n", avg_bsize);
		break;
	case REQ_OP_WRITE:
		stats->wrq_num++;
		stats->wrq_bsize_total += bsize;
		avg_bsize = stats->wrq_bsize_total / stats->wrq_num;
		pr_info("Current write requests num: %u\n", stats->wrq_num);
		pr_info("Current average write block size: %llu\n", avg_bsize);
		break;
	default:
		break;
	}

	bio_set_dev(bio, dh->dev->bdev);

	return DM_MAPIO_SUBMITTED;
}

static int dmp_ctr(struct dm_target* ti, unsigned int argc, char** argv)
{
	dmp_dev_handle_t* dh;
	int error;

	pr_info("argc: %u, argv[0]: %s\n", argc, argv[0]);

	if (argc != 1) {
		ti->error = "Invalid number of arguments";
		error = -EINVAL;
		goto fail;
	}

	pr_info("allocating dh...\n");

	dh = kzalloc(sizeof(dmp_dev_handle_t), GFP_KERNEL);
	if (!dh) {
		ti->error = "Cannot allocate memory for dmp device handle";
		error = -ENOMEM;
		goto fail;
	}

	dh->stats = kzalloc(sizeof(dmp_stats_t), GFP_KERNEL);
	if (!dh->stats) {
		ti->error = "Cannot allocate memory for dh";
		error = -ENOMEM;
		goto fail_stats;
	}

	pr_info("dh allocated\n");
	pr_info("getting device...\n");

	error = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &(dh->dev));
	if (error) {
		ti->error = "Device lookup failed";
		goto fail;
	}

	ti->private = dh;
	pr_info("got device!\n");

	return 0;

fail_stats:
	kfree(dh->stats);
fail:
	kfree(dh);
	return error;
}

static void dmp_dtr(struct dm_target* ti)
{
	dmp_dev_handle_t* dh = ti->private;
	dm_put_device(ti, dh->dev);
	kfree(dh);
}

static struct target_type dmp_target = {
	.name = "dmp",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.map = dmp_map,
	.ctr = dmp_ctr,
	.dtr = dmp_dtr
};

static int __init dmp_init(void)
{
	int result = dm_register_target(&dmp_target);
	if (result) {
		pr_err("Failed to register target: error %d\n", result);
		dm_unregister_target(&dmp_target);
		return result;
	}

	pr_info("Initialized dmp module\n");
	return 0;
}

static void __exit dmp_exit(void)
{
	dm_unregister_target(&dmp_target);
	pr_info("Exit dmp module\n");
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
