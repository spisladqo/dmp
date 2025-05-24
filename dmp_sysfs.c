// SPDX-License-Identifier: GPL-2.0-only
/*
 * Vlasenco Daniel <vlasenko.daniil26@gmail.com>
 *
 * This file is released under the GPL.
 */
#include "dmp.h"

typedef struct {
	struct attribute attr;
	ssize_t (*show) (dmp_stats_t *stats, char *buf);
} stats_attr_t;

static ssize_t stats_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	dmp_stats_t *stats = container_of(kobj, dmp_stats_t, kobj);
	stats_attr_t *attribute = container_of(attr, stats_attr_t, attr);

	if (!attribute->show) {
		return -EIO;
	}
	return attribute->show(stats, buf);
}

static ssize_t stats_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t len)
{
	return -EIO;
}

static const struct sysfs_ops stats_sysfs_ops = {
	.show = stats_show,
	.store = stats_store,
};

static ssize_t stats_rd_show(dmp_stats_t *stats, char *buf)
{
    unsigned long long bsize_avg;
    int ret;

    spin_lock(&stats->lock_rd_stats);
    if (stats->rrq_num != 0) {
        bsize_avg = stats->rrq_bsize_total / stats->rrq_num;
    } else {
        bsize_avg = 0;
    }
    ret = sprintf(buf, "read:\n reqs: %u\n avg size: %llu\n",
		stats->rrq_num, bsize_avg);
    spin_unlock(&stats->lock_rd_stats);

    return ret;
}

static ssize_t stats_wr_show(dmp_stats_t *stats, char *buf)
{
    unsigned long long bsize_avg;
    int ret;

    spin_lock(&stats->lock_wr_stats);
    if (stats->wrq_num != 0) {
        bsize_avg = stats->wrq_bsize_total / stats->wrq_num;
    } else {
        bsize_avg = 0;
    }
    ret = sprintf(buf, "write:\n reqs: %u\n avg size: %llu\n",
        stats->wrq_num, bsize_avg);
    spin_unlock(&stats->lock_wr_stats);

    return ret;
}

static ssize_t stats_total_show(dmp_stats_t *stats, char *buf)
{
    unsigned long long bsize_total;
    unsigned long long bsize_avg;
    unsigned rq_num;
    int ret;

    spin_lock(&stats->lock_rd_stats);
    spin_lock(&stats->lock_wr_stats);
    bsize_total = stats->rrq_bsize_total + stats->wrq_bsize_total;
    rq_num = stats->rrq_num + stats->wrq_num;
    if (rq_num != 0) {
        bsize_avg = bsize_total / rq_num;
    } else {
        bsize_avg = 0;
    }
    ret = sprintf(buf, "total:\n reqs: %u\n avg size: %llu\n",
        rq_num, bsize_avg);
    spin_unlock(&stats->lock_wr_stats);
    spin_unlock(&stats->lock_rd_stats);

    return ret;
}

static stats_attr_t kobj_attr_rd = __ATTR_RO(stats_rd);
static stats_attr_t kobj_attr_wr = __ATTR_RO(stats_wr);
static stats_attr_t kobj_attr_total = __ATTR_RO(stats_total);


static void stats_release(struct kobject *kobj)
{
	dmp_stats_t *stats = container_of(kobj, dmp_stats_t, kobj);
	kfree(stats);
}

static struct attribute *stats_attrs[] = {
	&kobj_attr_rd.attr,
	&kobj_attr_wr.attr,
	&kobj_attr_total.attr,
	NULL,
};
ATTRIBUTE_GROUPS(stats);

static const struct kobj_type stats_type = {
	.release = stats_release,
	.sysfs_ops = &stats_sysfs_ops,
	.default_groups = stats_groups,
};

dmp_stats_t *dmp_init_stats(const char *name, struct kset *kset) {
    dmp_stats_t *stats;
    int error;

    stats = kzalloc(sizeof(dmp_stats_t), GFP_KERNEL);
    if (!stats) {
        return NULL;
    }

    stats->kobj.kset = kset;

	spin_lock_init(&stats->lock_rd_stats);
	spin_lock_init(&stats->lock_wr_stats);

    error = kobject_init_and_add(&stats->kobj, &stats_type, NULL, "%s", name);
    if (error) {
        goto fail;
    }

    error = kobject_uevent(&stats->kobj, KOBJ_ADD);
    if (error) {
        goto fail;
    }

    return stats;

fail:
    kobject_put(&stats->kobj);
    kfree(stats);
    return NULL;
}

void dmp_free_stats(dmp_stats_t *stats) {
    if (!stats) {
        return;
    }

    kobject_put(&stats->kobj);
    kfree(stats);
}