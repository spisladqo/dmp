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

int32_t reads = 0;
int32_t writes = 0;

static int dmp_map(struct dm_target *ti, struct bio *bio)
{
	switch (bio_op(bio)) {
	case REQ_OP_READ:
		reads++;
		break;
	case REQ_OP_WRITE:
		writes++;
		break;
	default:
		return DM_MAPIO_KILL;
	}

	bio_endio(bio);

	return DM_MAPIO_SUBMITTED;
}

MODULE_AUTHOR("Vlasenco Daniel <vlasenko.daniil26@gmail.com>");
MODULE_DESCRIPTION("Device-mapper-based module for device statistics collection");
MODULE_LICENSE("GPL");
