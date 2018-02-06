#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/time.h>
#include <time.h>

#include <sys/mman.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>


#define RK322X_DDR_MSCH_PHYS     0xff720000
#define RK322X_DDR_MSCH_SIZE     1024*64

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

void *io_base;

void relative_init(int offset)
{
	*((unsigned long *)(io_base + INIT_REG0_OFFSET + offset)) = 0x8;
	*((unsigned long *)(io_base + INIT_REG1_OFFSET + offset)) = 0x1;
	*((unsigned long *)(io_base + INIT_REG2_OFFSET + offset)) = 0x6;
	*((unsigned long *)(io_base + INIT_REG3_OFFSET + offset)) = 0x10;
	*((unsigned long *)(io_base + INIT_REG4_OFFSET + offset)) = 0x8;
	*((unsigned long *)(io_base + INIT_REG5_OFFSET + offset)) = 0x10;
	//printf("iobase: %x, INIT_REG0_OFFSET: %x, offeset: %x,reg=%x \n", (int)io_base, (int)INIT_REG0_OFFSET, (int)offset, (int)(io_base + INIT_REG0_OFFSET + offset));
}

void all_trigger(void)
{
	*((unsigned long *)(io_base + TRIGGER_OFFSET + CPU_MSCH_OFFSET)) = 0x1;
	*((unsigned long *)(io_base + TRIGGER_OFFSET + GPU_MSCH_OFFSET)) = 0x1;
	*((unsigned long *)(io_base + TRIGGER_OFFSET + PERI_MSCH_OFFSET)) = 0x1;
	*((unsigned long *)(io_base + TRIGGER_OFFSET + VOP0_MSCH_OFFSET)) = 0x1;
	*((unsigned long *)(io_base + TRIGGER_OFFSET + VOP1_MSCH_OFFSET)) = 0x1;
	*((unsigned long *)(io_base + TRIGGER_OFFSET + VIDEO_MSCH_OFFSET)) = 0x1; 
}

int get_ddr_freq()
{
	char file_str[1024];
	char *p;
	int ddr,i=0;
	FILE *fp = fopen("/sys/kernel/debug/clk/clk_summary","r");
	if (fp == NULL)
	{
		printf("open file failed");
		exit(1);
	}
	while ( fgets(file_str,sizeof(file_str),fp) )
	{
		if( strstr(file_str,"clk_ddrmsch") )
		{
			//printf("\n%s\n",file_str);
			
			p = strtok(file_str," ");
			while(p!=NULL)
			{
				p = strtok(NULL," ");
				i = atoi(p);
				if(i>10000)
				{
					//printf("ddr_freq = %d\n",i);
					break;
				}
			}
		}
	}
	fclose(fp);
	return i;
}

void get_bd(void)
{
	unsigned long long ddrfreq,cpu_count,gpu_count,video_count,vop0_count,vop1_count,peri_count,totalcount = 0;
	cpu_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + CPU_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + CPU_MSCH_OFFSET));						
	vop0_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + VOP0_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + VOP0_MSCH_OFFSET));	       
	video_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + VIDEO_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + VIDEO_MSCH_OFFSET));
	gpu_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + GPU_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + GPU_MSCH_OFFSET));
	peri_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + PERI_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + PERI_MSCH_OFFSET));
	vop1_count = (*((unsigned long *)(io_base + COUNTER_REG3_OFFSET + VOP1_MSCH_OFFSET))<<16) | *((unsigned long *)(io_base + COUNTER_REG2_OFFSET + VOP1_MSCH_OFFSET));
	totalcount = cpu_count + gpu_count+ peri_count + vop0_count + vop1_count + video_count;
	ddrfreq = get_ddr_freq();
	printf("cpu_count = %-12lld  vop0_count = %-12lld  vop1_count = %-12lld  video_count = %-12lld  gpu_count = %-12lld  peri_count = %-12lld  totalcount = %-12lld  ddrcount = %-12lld  total/ddr = %f\n", 
			cpu_count,vop0_count,vop1_count,video_count,gpu_count,peri_count,totalcount,ddrfreq*8,(0.0+totalcount)/(0.0+ddrfreq*8));
}

int main()
{
	int mfd;
	unsigned long long cpu_count,gpu_count,video_count,vop0_count,vop1_count,peri_count,totalcount = 0;

	mfd = open("/dev/mem", O_RDWR);
	if (mfd == -1) {
		printf("open /dev/mem failed");
		exit(1);
	}
	io_base = mmap(NULL, RK322X_DDR_MSCH_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mfd, RK322X_DDR_MSCH_PHYS);
	if (io_base == (void *)(-1)) {
		fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
		exit(1);
	}

	relative_init(VIDEO_MSCH_OFFSET);
	relative_init(CPU_MSCH_OFFSET);
	relative_init(VOP0_MSCH_OFFSET);
	relative_init(GPU_MSCH_OFFSET);
	relative_init(PERI_MSCH_OFFSET);
	relative_init(VOP1_MSCH_OFFSET);
	
	//ddrfreq = get_ddr_freq();

	while(1)
	{
		sleep(1);
		{
			get_bd();
			all_trigger();
		}
	}
	return 0;	
}