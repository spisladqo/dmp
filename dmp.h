// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */

#ifndef DMP_H
#define DMP_H

#include <linux/kobject.h>
#include <linux/spinlock.h>

#define DM_MSG_PREFIX "dmp"

typedef struct {
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

extern dmp_dev_handle_t* dmp_dh_global;

int dmp_sysfs_init(void);
void dmp_sysfs_exit(void);
ssize_t dmp_sysfs_show_rd(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf);
ssize_t dmp_sysfs_show_wr(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf);
ssize_t dmp_sysfs_show_all(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf);
#endif // DMP_H
