// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */
#include "dmp.h"

static struct kobject* kobj;
static struct kobj_attribute kobj_attr_rd = __ATTR(read_stat, 0444, dmp_sysfs_show_rd, NULL);
static struct kobj_attribute kobj_attr_wr = __ATTR(write_stat, 0444, dmp_sysfs_show_wr, NULL);
static struct kobj_attribute kobj_attr_all = __ATTR(total_stat, 0444, dmp_sysfs_show_all, NULL);

ssize_t dmp_sysfs_show_rd(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    unsigned long long bsize_avg;
    dmp_stats_t s;

    if (!dmp_dh_global) {
        pr_err("Failed to show stats: device is unlinked\n");
        return -ENODEV;
    }

	s = *(dmp_dh_global->stats);

    if (s.rrq_num != 0) {
        bsize_avg = s.rrq_bsize_total / s.rrq_num;
    } else {
        bsize_avg = 0;
    }

    return sprintf(buf, "read:\n reqs: %u\n avg size: %llu\n",
		            s.rrq_num, bsize_avg);
}

ssize_t dmp_sysfs_show_wr(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    unsigned long long bsize_avg;
    dmp_stats_t s;

    if (!dmp_dh_global) {
        pr_err("Failed to show stats: device is unlinked\n");
        return -ENODEV;
    }

	s = *(dmp_dh_global->stats);

    if (s.wrq_num != 0) {
        bsize_avg = s.wrq_bsize_total / s.wrq_num;
    } else {
        bsize_avg = 0;
    }

    return sprintf(buf, "write:\n reqs: %u\n avg size: %llu\n",
		            s.wrq_num, bsize_avg);
}

ssize_t dmp_sysfs_show_all(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    unsigned long long bsize_total;
    unsigned long long bsize_avg;
    unsigned rq_num;
    dmp_stats_t s;

    if (!dmp_dh_global) {
        pr_err("Failed to show stats: device is unlinked\n");
        return -ENODEV;
    }

	s = *(dmp_dh_global->stats);

    bsize_total = s.rrq_bsize_total + s.wrq_bsize_total;
    rq_num = s.rrq_num + s.wrq_num;
    if (rq_num != 0) {
        bsize_avg = bsize_total / rq_num;
    } else {
        bsize_avg = 0;
    }

    return sprintf(buf, "total:\n reqs: %u\n avg size: %llu\n",
		            rq_num, bsize_avg);
}

int dmp_sysfs_init(void) {
    int error = 0;

    kobj = kobject_create_and_add("dmp", kernel_kobj);
    if (!kobj) {
        pr_err("Failed to create kobject\n");
        return -ENOMEM;
    }
    error = sysfs_create_file(kobj, &kobj_attr_rd.attr);
    if (error) {
        pr_err("Failed to create sysfs read stats file\n");
        goto fail_rd;
    }
    error = sysfs_create_file(kobj, &kobj_attr_wr.attr);
    if (error) {
        pr_err("Failed to create sysfs write stats file\n");
        goto fail_wr;
    }
    error = sysfs_create_file(kobj, &kobj_attr_all.attr);
    if (error) {
        pr_err("Failed to create sysfs total stats file\n");
        goto fail_all;
    }

    return 0;

fail_all:
    sysfs_remove_file(kobj, &kobj_attr_wr.attr);
fail_wr:
    sysfs_remove_file(kobj, &kobj_attr_rd.attr);
fail_rd:
    kobject_put(kobj);
    return error;
}

void dmp_sysfs_exit(void) {
    sysfs_remove_file(kobj, &kobj_attr_rd.attr);
    sysfs_remove_file(kobj, &kobj_attr_wr.attr);
    sysfs_remove_file(kobj, &kobj_attr_all.attr);
    kobject_put(kobj);
}
