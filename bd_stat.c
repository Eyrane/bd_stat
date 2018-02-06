#include <linux/init.h>
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/jiffies.h>
#include <asm/io.h>
#include <linux/clk.h>
#include <asm/div64.h>

#define RK322X_DDR_MSCH_PHYS     0xff720000  //msch的物理地址。msch仲裁vop、vpu、cpu哪个占有ddr。vop、vpu、cpu的地址是在msch的基础上偏移的。
#define RK322X_DDR_MSCH_SIZE     SZ_64K

#define INIT_REG0_OFFSET	0x8 
#define INIT_REG1_OFFSET	0xc 
#define INIT_REG2_OFFSET	0x138 
#define INIT_REG3_OFFSET	0x14c 
#define INIT_REG4_OFFSET	0x160
#define INIT_REG5_OFFSET	0x174 
#define TRIGGER_OFFSET		0x28		
#define COUNTER_REG0_OFFSET	0x13c
#define COUNTER_REG1_OFFSET	0x150
#define COUNTER_REG2_OFFSET	0x164
#define COUNTER_REG3_OFFSET	0x178
#define CPU_MSCH_OFFSET		0x1800
#define GPU_MSCH_OFFSET		0x400
#define PERI_MSCH_OFFSET	0x800
#define VOP0_MSCH_OFFSET	0xc00
#define VOP1_MSCH_OFFSET	0x1000
#define VIDEO_MSCH_OFFSET	0x1400

static void __iomem *io_base;
static struct hrtimer timer;
static ktime_t kt;
static unsigned long long cpu_count;
static unsigned long long gpu_count;
static unsigned long long peri_count;
static unsigned long long vop0_count;
static unsigned long long vop1_count;
static unsigned long long video_count;
static unsigned long long ddrfreq;

MODULE_LICENSE("GPL");

static void relative_init(int offset)
{
	writel(0x8, io_base + INIT_REG0_OFFSET + offset);
	writel(0x1, io_base + INIT_REG1_OFFSET + offset);
	writel(0x6, io_base + INIT_REG2_OFFSET + offset);
	writel(0x10, io_base + INIT_REG3_OFFSET + offset);
	writel(0x8, io_base + INIT_REG4_OFFSET + offset);
	writel(0x10, io_base + INIT_REG5_OFFSET + offset);
	printk("iobase: %x, INIT_REG0_OFFSET: %x, offeset: %x,reg=%x \n", (int)io_base, (int)INIT_REG0_OFFSET, (int)offset, (int)(io_base + INIT_REG0_OFFSET + offset));
}

static void all_trigger(void)
{
	writel(0x1, io_base + TRIGGER_OFFSET + CPU_MSCH_OFFSET);
	writel(0x1, io_base + TRIGGER_OFFSET + GPU_MSCH_OFFSET);
	writel(0x1, io_base + TRIGGER_OFFSET + PERI_MSCH_OFFSET);
	writel(0x1, io_base + TRIGGER_OFFSET + VOP0_MSCH_OFFSET);
	writel(0x1, io_base + TRIGGER_OFFSET + VOP1_MSCH_OFFSET);
	writel(0x1, io_base + TRIGGER_OFFSET + VIDEO_MSCH_OFFSET);
}

static void read_count(void)
{
	cpu_count = readl(io_base + COUNTER_REG3_OFFSET + CPU_MSCH_OFFSET);
	cpu_count = (cpu_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + CPU_MSCH_OFFSET);
	gpu_count = readl(io_base + COUNTER_REG3_OFFSET + GPU_MSCH_OFFSET);
	gpu_count = (gpu_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + GPU_MSCH_OFFSET);
	peri_count = readl(io_base + COUNTER_REG3_OFFSET + PERI_MSCH_OFFSET);
	peri_count = (peri_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + PERI_MSCH_OFFSET);
	vop0_count = readl(io_base + COUNTER_REG3_OFFSET + VOP0_MSCH_OFFSET);
	vop0_count = (vop0_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + VOP0_MSCH_OFFSET);
	vop1_count = readl(io_base + COUNTER_REG3_OFFSET + VOP1_MSCH_OFFSET);
	vop1_count = (vop1_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + VOP1_MSCH_OFFSET);
	video_count = readl(io_base + COUNTER_REG3_OFFSET + VIDEO_MSCH_OFFSET);
	video_count = (video_count << 16) | readl(io_base + COUNTER_REG2_OFFSET + VIDEO_MSCH_OFFSET);
}

static enum hrtimer_restart hrtimer_handler(struct hrtimer *timer)
{
	unsigned long long totalcount = 0;


	read_count();//读取虚拟地址中的值
	printk("cpu: %10lld, gpu: %10lld, peri: %10lld, vop0: %10lld, vop1: %10lld, video: %10lld\n", cpu_count, gpu_count, peri_count, vop0_count, vop1_count, video_count);
	
	totalcount = cpu_count + gpu_count+ peri_count + vop0_count + vop1_count + video_count;
	printk("totalcount = %lld/%lld \n", totalcount,ddrfreq*8);
	
	x = totalcount;
	y = ddrfreq*8;
    printk("%lld/%lld\n", totalcount,y);//打印出总带宽
	
	
	all_trigger();
	hrtimer_forward(timer, timer->base->get_time(), kt);
	return HRTIMER_RESTART;
}

static int bd_stat_init(void)
{
	struct clk *clk;
	
	clk = clk_get(NULL,"clk_ddr");//得到clk_ddr对应的时钟
	ddrfreq = clk_get_rate(clk);//得到时钟频率
	
	io_base = ioremap(RK322X_DDR_MSCH_PHYS, RK322X_DDR_MSCH_SIZE);//物理地址映射到虚拟地址
        printk("cpu\n");
	relative_init(CPU_MSCH_OFFSET);//虚拟地址进行清0
        printk("gpu\n");
	relative_init(GPU_MSCH_OFFSET);
        printk("peri\n");
	relative_init(PERI_MSCH_OFFSET);
        printk("vop0\n");
	relative_init(VOP0_MSCH_OFFSET);
        printk("vop1\n");
	relative_init(VOP1_MSCH_OFFSET);
        printk("vpu\n");
	relative_init(VIDEO_MSCH_OFFSET);
	
	
	kt = ktime_set(1, 0); /* 1 sec */ //定时器，1s
	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);//初始化定时器工作模式
	timer.function = hrtimer_handler;//主要是读取地址中的值
	hrtimer_start(&timer, kt, HRTIMER_MODE_REL);//第二个参数用于设置超时参数
	return 0;
}

static void bd_stat_exit(void)
{
	iounmap(io_base);
	hrtimer_cancel(&timer);
}

module_init(bd_stat_init);
module_exit(bd_stat_exit);
