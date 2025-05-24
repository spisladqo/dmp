# dmp
A custom device mapper target for operations statistics collection.


## Preinstallation

To build this module, you need to have the source code of your kernel locally.

## Build and usage
Clone this repository, open it and run `make`.

To insert the module into running kernel, run `sudo insmod dmp.ko`.

To create virtual device with name `<name>` and size `<size>` over some other device, run `sudo dmsetup create <name> --table "0 <size> dmp /dev/path/to/dev"`

To get statistics, you can read any of the following sysfs entries:
- `/sys/module/stat/<dev>/stats_rd`
- `/sys/module/stat/<dev>/stats_wr`
- `/sys/module/stat/<dev>/stats_total`

### Example:
```
$ sudo dmsetup create zero1 --table "0 4096 zero"

$ sudo dmsetup create dmp1 --table "0 4096 dmp /dev/mapper/zero1"

$ sudo dd if=/dev/urandom of=/dev/mapper/dmp1 oflag=direct bs=4k count=500

$ cat /sys/module/dmp/stat/252\:1/stats_wr 
write:
 reqs: 500
 avg size: 4096
```

## Testing

Module was tested on Linux fedora 6.14.6 with `dd` and `fio`.