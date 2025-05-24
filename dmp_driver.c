// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */

#include <linux/device-mapper.h>
#include <linux/init.h>
#include <linux/bio.h>
#include <linux/types.h>
#include <linux/device-mapper.h>
#include "dmp.h"

static struct kset *stats_kset;

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
		spin_lock(&stats->lock_rd_stats);
		stats->rrq_num++;
		stats->rrq_bsize_total += bsize;
		avg_bsize = stats->rrq_bsize_total / stats->rrq_num;
		spin_unlock(&stats->lock_rd_stats);
		break;
	case REQ_OP_WRITE:
		spin_lock(&stats->lock_wr_stats);
		stats->wrq_num++;
		stats->wrq_bsize_total += bsize;
		avg_bsize = stats->wrq_bsize_total / stats->wrq_num;
		spin_unlock(&stats->lock_wr_stats);
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
		goto fail;
	}

	error = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &dh->dev);
	if (error) {
		ti->error = "Device lookup failed";
		goto fail;
	}

	dh->stats = dmp_init_stats(dm_table_device_name(ti->table), stats_kset);
	if (!dh->stats) {
		ti->error = "Failed to initialize stats";
		goto fail_stats;
	}

	ti->private = dh;

	return 0;

fail_stats:
	dm_put_device(ti, dh->dev);
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
	int error = dm_register_target(&dmp_target);
	if (error) {
		return error;
	}

	stats_kset = kset_create_and_add("stat", NULL, &THIS_MODULE->mkobj.kobj);
	if (!stats_kset) {
		dm_unregister_target(&dmp_target);
		return -ENOMEM;
	}

	return 0;
}

static void __exit dmp_exit(void)
{
	dm_unregister_target(&dmp_target);
	kset_unregister(stats_kset);
}

module_init(dmp_init);
module_exit(dmp_exit);

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
