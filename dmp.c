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
} dmp_stat_t;

static dmp_stat_t stats;

typedef struct {
	struct dm_dev* dev;
	sector_t start;
} dmp_target_t;

static int dmp_map(struct dm_target* ti, struct bio* bio)
{
	unsigned long long bsize;
	unsigned long long avg_bsize;
	dmp_target_t* dt;

	dt = ti->private;
	bsize = bio->bi_iter.bi_size;
	bio->bi_bdev = dt->dev->bdev;
	stats.rq_num++;
	stats.rq_bsize_total = stats.rq_bsize_total + bsize;

	pr_info("Requests num: %u\n", stats.rq_num);
	pr_info("Received bio with block of size: %llu\n", bsize);
	pr_info("Total block size: %llu\n", stats.rq_bsize_total);

	switch (bio_op(bio)) {
	case REQ_OP_READ:
		stats.rrq_num++;
		stats.rrq_bsize_total += bsize;
		avg_bsize = stats.rrq_bsize_total / stats.rrq_num;
		pr_info("Current read requests num: %u\n", stats.rrq_num);
		pr_info("Current average read block size: %llu\n", avg_bsize);
		break;
	case REQ_OP_WRITE:
		stats.wrq_num++;
		stats.wrq_bsize_total += bsize;
		avg_bsize = stats.wrq_bsize_total / stats.wrq_num;
		pr_info("Current write requests num: %u\n", stats.wrq_num);
		pr_info("Current average write block size: %llu\n", avg_bsize);
		break;
	default:
		break;
	}

	submit_bio(bio);

	pr_info("Bio submitted successfully\n");
	return DM_MAPIO_SUBMITTED;
}

static int dmp_ctr(struct dm_target* ti, unsigned int argc, char** argv)
{
	dmp_target_t* dt;
	sector_t start;
	int error;

	if (argc != 2) {
		pr_err("Invalid number of arguments\n");
		ti->error = "Invalid number of arguments";
		return -EINVAL;
	}

	dt = kzalloc(sizeof(dmp_target_t), GFP_KERNEL);
	if (!dt) {
		pr_err("Cannot allocate memory for dt\n");
		ti->error = "Cannot allocate memory for dt";
		return -ENOMEM;
	}

	if (sscanf(argv[1], "%llu", &start) != 1)
	{
		pr_err("Invalid device sector\n");
		ti->error = "Invalid device sector";
		kfree(dt);
		return -EINVAL;
	}

	error = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &(dt->dev));
	if (error) {
		pr_err("Device lookup failed\n");
		ti->error = "Device lookup failed";
		kfree(dt);
		return error;
	}

	dt->start = start;
	ti->private = dt;

	return 0;
}

static void dmp_dtr(struct dm_target* ti)
{
	dmp_target_t* dt = ti->private;
	dm_put_device(ti, dt->dev);
	kfree(dt);
}

static struct target_type dmp_target = {
	.name = "dmp_target",
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
	}
	pr_info("Initialized dmp module\n");
	return result;
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
