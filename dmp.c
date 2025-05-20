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
	unsigned int read_num;
	unsigned int write_num;
	unsigned int op_num;
	unsigned int avg_bsize_read;
	unsigned int avg_bsize_write;
	unsigned int avg_bsize;
} dmp_stat_t;

static dmp_stat_t stats;

typedef struct {
	struct dm_dev* dev;
	sector_t start;
} dmp_target_t;

static int dmp_map(struct dm_target* ti, struct bio* bio)
{
	unsigned int bsize;
	dmp_target_t* dt;

	dt = ti->private;
	bsize = bio->bi_iter.bi_size;
	bio->bi_bdev = dt->dev->bdev;

	stats.op_num++;
	stats.avg_bsize = (stats.avg_bsize + bsize) / stats.op_num;

	switch (bio_op(bio)) {
	case REQ_OP_READ:
		stats.read_num++;
		stats.avg_bsize_read = (stats.avg_bsize_read + bsize) / stats.read_num;
		break;
	case REQ_OP_WRITE:
		stats.write_num++;
		stats.avg_bsize_write = (stats.avg_bsize_write + bsize) / stats.write_num;
		break;
	default:
		break;
	}

	submit_bio(bio->bi_rw, bio);

	return DM_MAPIO_SUBMITTED;
}

static int dmp_ctr(struct dm_target* ti, unsigned int argc, char** argv)
{
	dmp_target_t* dt;
	sector_t start;
	int err;

	if (argc != 2) {
		pr_err("Invalid number of arguments\n");
		ti->error = "Invalid number of arguments";
		return -EINVAL;
	}

	dt = kzalloc(sizeof(dmp_target_t), GFP_KERNEL);
	if (!dt) {
		pr_err("Cannot allocate memory for dt\n");
		ti->err = "Cannot allocate memory for dt";
		return -ENOMEM;
	}

	if (sscanf(argv[1], "%llu", &start) != 1)
	{
		pr_err("Invalid device sector\n");
		ti->error = "Invalid device sector";
		free(dt);
		return -EINVAL;
	}

	err = dm_get_device(ti, argv[0], dm_table_get_mode(ti->table), &(mdt->dev));
	if (err) {
		pr_err("Device lookup failed\n");
		ti->error = "Device lookup failed";
		free(dt);
		return err;
	}

	dt->start = start;
	ti->private = dt;

	return 0;
}

static struct target_type dmp_target = {
	.name = "dmp_target",
	.version = {1, 0, 0},
	.module = THIS_MODULE,
	.map = dmp_map,
	.ctr = dmp_ctr,
};

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
