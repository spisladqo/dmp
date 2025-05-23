// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */
#include "dmp.h"

static struct kobject* kobj;
static struct kobj_attribute kobj_attr = __ATTR(dmp, 0444, dmp_sysfs_show, NULL);

ssize_t dmp_sysfs_show(struct kobject *kobj,
                struct kobj_attribute *attr, char *buf)
{
    if (!dmp_dh_global) {
        pr_err("Failed to show stats: link device to an existing one\n");
        return -ENODEV;
    }

	dmp_stats_t s = *(dmp_dh_global->stats);

    return sprintf(buf, "read:\n reqs: %u\n avg size: %llu\n"
						"write:\n reqs: %u\n avg size: %llu\n"
						"total:\n reqs: %u\n avg size: %llu\n",
		s.rrq_num, s.rrq_bsize_total / s.rrq_num, s.wrq_num,
		s.wrq_bsize_total / s.wrq_num, s.rq_num, s.rq_bsize_total / s.rq_num);
}

int dmp_sysfs_init(void) {
    int error = 0;

    kobj = kobject_create_and_add("dmp",kernel_kobj);
    if (!kobj) {
        pr_err("Failed to create kobject\n");
    }

    error = sysfs_create_file(kobj, &kobj_attr.attr);
    if (error) {
        pr_err("Failed to create sysfs file\n");
        kobject_put(kobj);
        return error;
    }

    pr_info("Initialized sysfs\n");
    return 0;
}

void dmp_sysfs_exit(void) {
    sysfs_remove_file(kobj, &kobj_attr.attr);
    kobject_put(kobj);
}
