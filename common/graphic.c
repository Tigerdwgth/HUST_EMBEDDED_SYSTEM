#include "common.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

//static int LCD_FB_FD=-1;
static int *LCD_FB_BUF = NULL;
static int DRAW_BUF[SCREEN_WIDTH*SCREEN_HEIGHT];

static struct area {
	int x1, x2, y1, y2;
} update_area = {0,0,0,0};

#define AREA_SET_EMPTY(pa) do {\
	(pa)->x1 = SCREEN_WIDTH;\
	(pa)->x2 = 0;\
	(pa)->y1 = SCREEN_HEIGHT;\
	(pa)->y2 = 0;\
} while(0)

void fb_init(char *dev)
{
#if 0
	int fd;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	if(LCD_FB_BUF != NULL) return; /*already done*/

	//First: Open the device
	if((fd = open(dev, O_RDWR)) < 0){
		printf("Unable to open framebuffer %s, errno = %d\n", dev, errno);
		return;
	}
	if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) < 0){
		printf("Unable to FBIOGET_FSCREENINFO %s\n", dev);
		return;
	}
	if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0){
		printf("Unable to FBIOGET_VSCREENINFO %s\n", dev);
		return;
	}

	printf("framebuffer info: bits_per_pixel=%u,size=(%d,%d),virtual_pos_size=(%d,%d)(%d,%d),line_length=%u,smem_len=%u\n",
		fb_var.bits_per_pixel, fb_var.xres, fb_var.yres, fb_var.xoffset, fb_var.yoffset,
		fb_var.xres_virtual, fb_var.yres_virtual, fb_fix.line_length, fb_fix.smem_len);

	//Second: mmap
	int *addr;
	addr = mmap(NULL, fb_fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if((int)addr == -1){
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	if((fb_var.xoffset != 0) ||(fb_var.yoffset != 0))
	{
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if(ioctl(fd, FBIOPAN_DISPLAY, &fb_var) < 0) {
			printf("FBIOPAN_DISPLAY framebuffer failed\n");
		}
	}

	LCD_FB_FD = fd;
	LCD_FB_BUF = addr;
#endif

	LCD_FB_BUF = (int *)board_video_get_addr();
	if(LCD_FB_BUF == NULL){
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	//set empty
	AREA_SET_EMPTY(&update_area);
	return;
}

static void _copy_area(int *dst, int *src, struct area *pa)
{
	int x, y, w, h;
	x = pa->x1; w = pa->x2-x;
	y = pa->y1; h = pa->y2-y;
	char *src1 = (char *)src + y*SCREEN_WIDTH*4 + x*4;
	char *dst1 = (char *)dst + y*SCREEN_WIDTH*3 + x*3;
	while(h-- > 0)
	{
		int ww = w;
		char *src2 = src1;
		char *dst2 = dst1;
		while(ww-- > 0) {
			*dst2 = *src2;
			dst2[1] = src2[1];
			dst2[2] = src2[2];
			src2 += 4;
			dst2 += 3;
		}
		src1 += SCREEN_WIDTH*4;
		dst1 += SCREEN_WIDTH*3;
	}
}

static int _check_area(struct area *pa)
{
	if(pa->x2 == 0) return 0; //is empty

	if(pa->x1 < 0) pa->x1 = 0;
	if(pa->x2 > SCREEN_WIDTH) pa->x2 = SCREEN_WIDTH;
	if(pa->y1 < 0) pa->y1 = 0;
	if(pa->y2 > SCREEN_HEIGHT) pa->y2 = SCREEN_HEIGHT;

	if((pa->x2 > pa->x1) && (pa->y2 > pa->y1))
		return 1; //no empty

	//set empty
	AREA_SET_EMPTY(pa);
	return 0;
}

void fb_update(void)
{
	if(_check_area(&update_area) == 0) return; //is empty
	_copy_area(LCD_FB_BUF, DRAW_BUF, &update_area);
	AREA_SET_EMPTY(&update_area); //set empty
	return;
}

/*======================================================================*/

static void * _begin_draw(int x, int y, int w, int h)
{
	int x2 = x+w;
	int y2 = y+h;
	if(update_area.x1 > x) update_area.x1 = x;
	if(update_area.y1 > y) update_area.y1 = y;
	if(update_area.x2 < x2) update_area.x2 = x2;
	if(update_area.y2 < y2) update_area.y2 = y2;
	return DRAW_BUF;
}

void fb_draw_pixel(int x, int y, int color)
{
	if(x<0 || y<0 || x>=SCREEN_WIDTH || y>=SCREEN_HEIGHT) return;
	int *buf = _begin_draw(x,y,1,1);
/*---------------------------------------------------*/
	*(buf + y*SCREEN_WIDTH + x) = color;
/*---------------------------------------------------*/
	return;
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{
	if(x < 0) { w += x; x = 0;}
	if(x+w > SCREEN_WIDTH) { w = SCREEN_WIDTH-x;}
	if(y < 0) { h += y; y = 0;}
	if(y+h >SCREEN_HEIGHT) { h = SCREEN_HEIGHT-y;}
	if(w<=0 || h<=0) return;
/*---------------------------------------------------*/
	int i,j;
	for(i = 0; i < w; ++i)
		for( j = 0; j < h ; ++j)
			fb_draw_pixel(x+i, y + j, color);

/*---------------------------------------------------*/
	return;
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
/*---------------------------------------------------*/
	int l,r,u,d;
	if(x1>x2)
	{
		l=x1;
		x1=x2;
		x2=l;
		l=y1;
		y1=y2;
		y2=l;
	}

	if(x1>x2){l=x2;r=x1;}else {l=x1;r=x2;}
	if(y1>y2){u=y1;d=y2;}else{u=y2;d=y1;}
	if(x1==x2)
	{
		for(;d<u;++d)fb_draw_pixel(x1,d,color);
	}
	else if(y1==y2)
	{
		//h
		for(;l<r;++l)fb_draw_pixel(l,y1,color);
	}
	else
	{

		double step=((double)abs(y1-y2)/abs(x1-x2));
		// double yy1=y1;
		// if (!(x1<x2&&y1<y2))step=-step; 
		// for(;l<r;++l)
		// {
		// 	fb_draw_pixel(l,(int)yy1,color);
		// 	yy1+=step;
		// }		
		if(step>4)
		{
			if (!(x1<x2&&y1<y2))step=-step; 
			for(;l<r;++l)
			{	
				int tmp=step;
				if(step<0)
				{
				while(tmp++<0)fb_draw_pixel(l,y1--,color);
				}
				else
				{
					while(tmp-->0)fb_draw_pixel(l,y1++,color);
				}
			}
		// 	printf("step: %.6f \n",step);
		// 	sleep(1);
		}
		else
		{
			double yy1=y1;
			if (!(x1<x2&&y1<y2))step=-step; 
			for(;l<r;++l)
			{
				fb_draw_pixel(l,(int)yy1,color);
				yy1+=step;
			}
		}
	}
/*---------------------------------------------------*/
	return;
}

void fb_draw_image(int x, int y, fb_image *image, int color)
{
	if(image == NULL) return;

	int ix = 0; //image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height

	if(x<0) {w+=x; ix-=x; x=0;}
	if(y<0) {h+=y; iy-=y; y=0;}
	
	if(x+w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if(y+h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if((w <= 0)||(h <= 0)) return;

	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------------------*/
	char *dst = (char *)(buf + y*SCREEN_WIDTH + x);
	char *src; //不同的图像颜色格式定位不同
/*---------------------------------------------------------------*/

	int alpha;
	int ww;

	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);

		return;
	}
	else if(image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);

		return;
	}
	else if(image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);

		return;
	}
/*---------------------------------------------------------------*/
	return;
}

void fb_draw_border(int x, int y, int w, int h, int color)
{
	if(w<=0 || h<=0) return;
	fb_draw_rect(x, y, w, 1, color);
	if(h > 1) {
		fb_draw_rect(x, y+h-1, w, 1, color);
		fb_draw_rect(x, y+1, 1, h-2, color);
		if(w > 1) fb_draw_rect(x+w-1, y+1, 1, h-2, color);
	}
}

/** draw a text string **/
void fb_draw_text(int x, int y, char *text, int font_size, int color)
{
	fb_image *img;
	fb_font_info info;
	int i=0;
	int len = strlen(text);
	while(i < len)
	{
		img = fb_read_font_image(text+i, font_size, &info);
		if(img == NULL) break;
		fb_draw_image(x+info.left, y-info.top, img, color);
		fb_free_image(img);

		x += info.advance_x;
		i += info.bytes;
	}
	return;
}

