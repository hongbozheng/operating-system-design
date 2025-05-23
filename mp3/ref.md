#### MP3 - Virtual Memory Page Fault Profiler Reference

list_empty
1. https://man7.org/linux/man-pages/man3/list.3.html
2. https://www.kernel.org/doc/htmldocs/kernel-api/API-list-empty.html
---
list_add_tail
1. https://www.kernel.org/doc/htmldocs/kernel-api/API-list-add-tail.html
2. https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch10s05.html
---
dev_t dev
1. https://man7.org/linux/man-pages/man3/makedev.3.html
2. https://www.oreilly.com/library/view/linux-device-drivers/0596005903/ch03.html
---
struct cdev
1. https://elixir.bootlin.com/linux/v5.15.63/source/include/linux/cdev.h#L14
---
int cdev_mmap
1. https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch13s02.html
---
remap_pfn_range
1. Reference: https://elixir.bootlin.com/linux/v5.15.63/source/mm/memory.c#L2452
2. Reference: https://www.kernel.org/doc/htmldocs/kernel-api/API-remap-pfn-range.html
---
struct file_operations
1. https://elixir.bootlin.com/linux/latest/source/include/linux/fs.h#L2093
2. https://tldp.org/LDP/lkmpg/2.4/html/c577.htm
3. https://docs.huihoo.com/doxygen/linux/kernel/3.7/structfile__operations.html
4. https://www.oreilly.com/library/view/linux-device-drivers/0596000081/ch03s03.html
---
register_chrdev_region
1. https://elixir.bootlin.com/linux/v5.15.63/source/fs/char_dev.c#L236
---
cdev_init
1. https://elixir.bootlin.com/linux/v5.15.63/source/fs/char_dev.c#L651
---
cdev_add
1. https://elixir.bootlin.com/linux/v5.15.63/source/fs/char_dev.c#L479
---
msecs_to_jiffies
1. https://elixir.bootlin.com/linux/v5.15.63/source/include/linux/jiffies.h#L363
---
vmalloc
1. https://elixir.bootlin.com/linux/v5.15.63/source/mm/vmalloc.c#L3104
---
SetPageReserved
1. https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html
---
cdev_del
1. https://elixir.bootlin.com/linux/v5.15.63/source/fs/char_dev.c#L594
---
unregister_chrdev_region
1. https://elixir.bootlin.com/linux/v5.15.63/source/fs/char_dev.c#L311
---
cancel_delayed_work_sync
1. https://manpages.debian.org/testing/linux-manual-4.8/cancel_delayed_work.9
2. https://elixir.bootlin.com/linux/v5.15.63/source/kernel/workqueue.c#L3309
---
ClearPageReserved
1. https://linux-kernel-labs.github.io/refs/heads/master/labs/memory_mapping.html