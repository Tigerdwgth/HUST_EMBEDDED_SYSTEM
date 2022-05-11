#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fb.h>

#include "common.h"
#include <libxlnk_cma.h>

/*====================================================================*/
#define PAGE_SIZE 0x1000
typedef struct {
	uintptr_t phy_base;
	volatile int *vir_base;
	int len;
} mmio_st;

static void _mmio_init(mmio_st *pmio, uintptr_t phy_base, int len)
{
	static int mem_fd = -1;
	if(mem_fd == -1)
	{
		mem_fd = open("/dev/mem", O_RDWR|O_SYNC);
		if(mem_fd == -1) {
			printf("failed to open /dev/mem, you are not root\n");
			return;
		}
	}

	if(phy_base & (PAGE_SIZE-1)) {
		printf("mmap base addr 0x%x is not align page_size\n", phy_base);
		return;
	}

	if(len & (PAGE_SIZE-1)) {
		printf("mmap len 0x%x is not align page_size\n", len);
		return;
	}

	pmio->phy_base = phy_base;
	pmio->len = len;
	pmio->vir_base = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, phy_base);
	if(pmio->vir_base == (void *)-1) {
		printf("failed to mmap vdma_phy_mem\n");
		return;
	}
}

static int _mmio_read_int(mmio_st *pmio, int offset) {
	if(offset & 3) {
		printf("_mmio_read: offset %d is not multiple of 4\n", offset);
		return 0;
	}
	if((offset < 0)||(offset >= pmio->len)) {
		printf("_mmio_read: offset %d error\n", offset);
		return 0;
	}
	return pmio->vir_base[offset>>2];
}

static void _mmio_write_int(mmio_st *pmio, int offset, int value) {
	if(offset & 3) {
		printf("_mmio_write: offset %d is not multiple of 4\n", offset);
		return;
	}
	if((offset < 0)||(offset >= pmio->len)) {
		printf("_mmio_write: offset %d error\n", offset);
		return;
	}
	pmio->vir_base[offset>>2] = value;
}

/*====================================================================*/
#define VDMA_PHY_MEM_ADDR	0x43000000
#define VDMA_PHY_MEM_LEN	0x10000
#define VIDEO_BUF_NUM	4
static mmio_st video_io;
static void _video_io_init() {
	if(video_io.phy_base == 0) {
		_mmio_init(&video_io, VDMA_PHY_MEM_ADDR, VDMA_PHY_MEM_LEN);
	}
}

static int _video_is_running(void) {
	return (_mmio_read_int(&video_io, 0x04) & 0x1) == 0;
}

static int _video_get_active_buf_index(void) {
	return (_mmio_read_int(&video_io, 0x28) >> 16) & 0x1F;
}

static int _video_get_buf_addr(int index) {
	return _mmio_read_int(&video_io, 0x5C + 4 * index);
}

#if 0
static void _video_set_buf_addr(int index, int phy_addr) {
	_mmio_write_int(&video_io, 0x5C + 4 * index, phy_addr);
}

static int _video_get_desired_buf_index(void) {
	return _mmio_read_int(&video_io, 0x28) & 0x1F;
}

static void _video_set_desired_buf_index(int index) {
	int old = _mmio_read_int(&video_io, 0x28);
    old = (old & ~0x1F)|index;
	_mmio_write_int(&video_io, 0x28, old);
}
#endif

void * board_video_get_addr(void)
{
	_video_io_init();

	if(!_video_is_running()) {
		printf("video dma is not run\n");
		exit(0);
	}

	int index = _video_get_active_buf_index();
	uintptr_t phy_addr = _video_get_buf_addr(index);
	//printf("cur index %d, phy_addr 0x%x\n", index, phy_addr);

	int len = SCREEN_WIDTH*SCREEN_HEIGHT*3;
	void *addr = (void *)cma_mmap(phy_addr, len);
	if(addr == (void *)-1){
		printf("failed to mmap memory for video\n");
		return NULL;
	}
	return addr;
}

/*====================================================================*/
#define AUDIO_PHY_MEM_ADDR	0x43c00000
#define AUDIO_PHY_MEM_LEN	0x10000
static mmio_st audio_io;
static void _audio_io_init() {
	if(audio_io.phy_base == 0) {
		_mmio_init(&audio_io, AUDIO_PHY_MEM_ADDR, AUDIO_PHY_MEM_LEN);
	}
}

enum audio_direct_regs {
    //Audio controller registers
    PDM_RESET_REG               = 0x00,
    PDM_TRANSFER_CONTROL_REG    = 0x04,
    PDM_FIFO_CONTROL_REG        = 0x08,
    PDM_DATA_IN_REG             = 0x0c,
    PDM_DATA_OUT_REG            = 0x10,
    PDM_STATUS_REG              = 0x14,
    //Audio controller Status Register Flags
    TX_FIFO_EMPTY               = 0,
    TX_FIFO_FULL                = 1,
    RX_FIFO_EMPTY               = 16,
    RX_FIFO_FULL                = 17
};

void board_audio_record(uint16_t *BufAddr, int nsamples)
{
	_audio_io_init();

	//Auxiliary
	uint32_t u32Temp, i=0;

	//Reset pdm
	_mmio_write_int(&audio_io, PDM_RESET_REG, 0x01);
	_mmio_write_int(&audio_io, PDM_RESET_REG, 0x00);
	//Reset fifos
	_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0xC0000000);
	_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000000);
	//Receive
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x00);
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x05);

	//Sample
	while(i < nsamples){
		u32Temp = ((_mmio_read_int(&audio_io, PDM_STATUS_REG)) 
					>> RX_FIFO_EMPTY) & 0x01;
		if(u32Temp == 0){
			_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000002);
			_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000000);
			u32Temp = _mmio_read_int(&audio_io, PDM_DATA_OUT_REG);
			BufAddr[i] = u32Temp; //取低16bit
			i++;
		}
	}

	//Stop
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x02);
}

void board_audio_play(uint16_t *BufAddr, int nsamples)
{
	_audio_io_init();

	//Auxiliary
	unsigned long u32Temp, i=0;

	//Reset i2s
	_mmio_write_int(&audio_io, PDM_RESET_REG, 0x01);
	_mmio_write_int(&audio_io, PDM_RESET_REG, 0x00);
	//Reset fifos
	_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0xC0000000);
	_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000000);
	//Transmit
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x00);
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x09);

	//Play
	while(i < nsamples) {
		u32Temp = ((_mmio_read_int(&audio_io, PDM_STATUS_REG)) 
					>> TX_FIFO_FULL) & 0x01;
		if(u32Temp == 0) {
			u32Temp = BufAddr[i];
			_mmio_write_int(&audio_io, PDM_DATA_IN_REG, u32Temp);
			_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000001);
			_mmio_write_int(&audio_io, PDM_FIFO_CONTROL_REG, 0x00000000);
			i++;
		}
	}

	//Stop/Reset
	_mmio_write_int(&audio_io, PDM_TRANSFER_CONTROL_REG, 0x00);
}
