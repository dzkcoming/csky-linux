#include <linux/console.h>
#include <linux/memblock.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <asm/sections.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>

/* fixme: reserve mem and mem not work together. */
static void __init zone_sizes_init(void)
{
	unsigned long zone_size[MAX_NR_ZONES];
	unsigned long min, max;

	min = PFN_UP(memblock_start_of_DRAM());
	max = PFN_DOWN(memblock_end_of_DRAM());

	memset(zone_size, 0, sizeof(zone_size));

	zone_size[ZONE_NORMAL] = max - min;

	free_area_init_node(0, zone_size, min, NULL);
}

static void __init csky_memblock_init(void)
{
	min_low_pfn = PFN_UP(memblock_start_of_DRAM());
	max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());

	memblock_reserve(__pa(_stext), _end - _stext);
#ifdef CONFIG_BLK_DEV_INITRD
	memblock_reserve(__pa(initrd_start), initrd_end - initrd_start);
#endif

	early_init_fdt_reserve_self();
	early_init_fdt_scan_reserved_mem();

	memblock_dump_all();

	zone_sizes_init();
}

void __init setup_arch(char **cmdline_p)
{
	*cmdline_p = boot_command_line;

	printk("www.c-sky.com\n");

	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code = (unsigned long) _etext;
	init_mm.end_data = (unsigned long) _edata;
	init_mm.brk = (unsigned long) _end;

	parse_early_param();

	csky_memblock_init();

	unflatten_and_copy_device_tree();

	sparse_init();

	pgd_init((unsigned long)swapper_pg_dir);

#if defined(CONFIG_VT) && defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
}

extern unsigned int _sbss, _ebss, vec_base;
asmlinkage __visible void __init
pre_start(
	unsigned int	magic,
	void		*param
	)
{
	int vbr = (int) &vec_base;

	/* Setup vbr reg */
	__asm__ __volatile__(
			"mtcr %0, vbr\n"
			::"b"(vbr));

	/* Setup mmu as coprocessor */
	select_mmu_cp();

	/* Setup page mask to 4k */
	write_mmu_pagemask(0);

	/* Clean up bss section */
	memset((void *)&__bss_start, 0,
		(unsigned int)&_end - (unsigned int)&__bss_start);

	if (magic == 0x20150401) {
		early_init_dt_scan(param);
	}

	return;
}

