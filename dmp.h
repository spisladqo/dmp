// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */

#ifndef DMP_H
#define DMP_H

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/spinlock.h>

#define DM_MSG_PREFIX "dmp"

typedef struct {
    struct kobject kobj;
	spinlock_t lock_rd_stats;
	spinlock_t lock_wr_stats;
	unsigned long long rrq_bsize_total;
	unsigned long long wrq_bsize_total;
	unsigned int rrq_num;
	unsigned int wrq_num;
} dmp_stats_t;

typedef struct {
	dmp_stats_t* stats;
	struct dm_dev* dev;
} dmp_dev_handle_t;

dmp_stats_t *dmp_init_stats(const char *name, struct kset *kset);
void dmp_free_stats(dmp_stats_t *stats);

#endif // DMP_H
