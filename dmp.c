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

typedef struct {
        struct dm_dev *dev;
        sector_t start;
} dmp_target_t;

static dmp_stat_t stats;

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	unsigned int bsize;
	dmp_target_t *dt;

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

	submit_bio(bio->bi_rw,bio);

	return DM_MAPIO_SUBMITTED;
}

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
