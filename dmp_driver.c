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
#include <linux/device-mapper.h>
#include "dmp.h"

dmp_dev_handle_t* dmp_dh_global;

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

	return DM_MAPIO_REMAPPED;
}

static int dmp_ctr(struct dm_target* ti, unsigned int argc, char** argv)
{
	dmp_dev_handle_t* dh;
	int error = -ENOMEM;

	if (argc != 1) {
		ti->error = "Invalid number of arguments";
		return -EINVAL;
	}

	dh = kzalloc(sizeof(dmp_dev_handle_t), GFP_KERNEL);
	if (!dh) {
		ti->error = "Cannot allocate memory for dmp device handle";
		goto fail_handle;
	}

	dh->stats = kzalloc(sizeof(dmp_stats_t), GFP_KERNEL);
	if (!dh->stats) {
		ti->error = "Cannot allocate memory for dh";
		goto fail;
	}

	error = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &(dh->dev));
	if (error) {
		ti->error = "Device lookup failed";
		goto fail;
	}

	dmp_dh_global = dh;
	ti->private = dh;

	return 0;

fail:
	kfree(dh->stats);
fail_handle:
	kfree(dh);
	return error;
}

static void dmp_dtr(struct dm_target* ti)
{
	dmp_dev_handle_t* dh = ti->private;
	dm_put_device(ti, dh->dev);
	kfree(dh);
	dmp_dh_global = NULL;
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
		return result;
	}

	result = dmp_sysfs_init();

	if (result) {
		dm_unregister_target(&dmp_target);
		return result;
	}

	pr_info("Initialized dmp module\n");
	return 0;
}

static void __exit dmp_exit(void)
{
	dmp_sysfs_exit();
	dm_unregister_target(&dmp_target);
	pr_info("Exit dmp module\n");
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
