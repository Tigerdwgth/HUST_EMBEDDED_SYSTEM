#include <stdio.h>
#include <time.h>

#include "../common/common.h"

#define COLOR_BACKGROUND	FB_COLOR(0xff,0xff,0xff)
#define RED		FB_COLOR(255,0,0)
#define ORANGE	FB_COLOR(255,165,0)
#define YELLOW	FB_COLOR(255,255,0)
#define GREEN	FB_COLOR(0,255,0)
#define CYAN	FB_COLOR(0,127,255)
#define BLUE	FB_COLOR(0,0,255)
#define PURPLE	FB_COLOR(139,0,255)
#define WHITE   FB_COLOR(255,255,255)
#define BLACK   FB_COLOR(0,0,0)
static int touch_fd;

//record last pos of finger
char buf[100]={0};
int fingerx[5];
int fingery[5];

static void touch_event_cb(int fd)
{

	int type,x,y,finger;
	type = touch_read(fd, &x,&y,&finger);

	int i=fingerx[finger],k;
	double j=fingery[finger];
	//vertical line
	int intj=fingery[finger];
	int intstepy=j>y?-1:1;	
	double stepy=((double)abs(fingery[finger]-y)/abs(fingerx[finger]-x));
	stepy=j>y?-stepy:stepy;
	int stepx=i>x?-1:1;


	int size=40;
	int color=RED;//different color fo different finger
	switch (finger)
	{
		case 0 :
			color=RED;
			break;
		case 1 :
			color=ORANGE;
			break;	
		case 2:
			color=YELLOW;
			break;
		case 3 :
			color=GREEN;
			break;						
		case 4 :
			color=BLUE;
			break;		
		default:
			break;
	}

	switch(type){
	case TOUCH_PRESS:

		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		fb_draw_text(x,y,"●",size,color);
		if(x<100&&y<100)exit(0);
		if(100<x&&x<200&&y<100)printf("--------------------------------start---------------------------------\n");
		if(200<x&&x<300&&y<100)
		{
			fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
			fb_draw_rect(0,0,100,100,ORANGE);//button
			fb_draw_rect(100,0,100,100,PURPLE);//button
			fb_draw_rect(200,0,100,100,CYAN);//button
			fb_draw_text(0,0,"EXIT",18,BLACK);
			fb_draw_text(0,0,"CLEAR",18,BLACK);
			fb_draw_text(300,40,"多点触控",32,PURPLE);
		}
		fingerx[finger]=x;i=x;fingery[finger]=y;j=y;
		break;
	case TOUCH_MOVE:
		if(i>800)i=800;
		if(j>600)j=600;
		printf("TOUCH_MOVE：x=%d-->%d,y=%d-->%d,finger=%d\n",i,x,(int)j,y,finger);
		if(i==x)
		{
			for(;intj!=y;intj+=intstepy)
			{
				// fb_draw_rect(i,intj,size,size,color);
				fb_draw_text(i,intj,"●",size,color);
			}	
		}
		else
		{
			for(;i!=x;i+=stepx)
			{
				for(k=0;k!=(int)(stepy+intstepy);k+=intstepy)
					fb_draw_text(i,(int)j+k,"●",size,color);
				j+=stepy;
			}

		}
		fingerx[finger]=x;
		fingery[finger]=y;
		break;
	case TOUCH_RELEASE:
		printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\t",x,y,finger);
		break;
	case TOUCH_ERROR:
		printf("close touch fd\n");
		close(fd);
		task_delete_file(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
}

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	font_init("./font.ttc");
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);
	fb_draw_text(300,40,"多点触控",32,PURPLE);
	fb_draw_rect(0,0,100,100,ORANGE);
	fb_draw_rect(200,0,100,100,CYAN);
	fb_draw_text(0,0,"EXIT",18,BLACK);
	fb_draw_text(0,0,"CLEAR",18,BLACK);
	fb_draw_rect(100,0,100,100,PURPLE);//button
	fb_update();
	int _;
	printf("initizing!!!!\n");
	for(_=0;_<5;_++)
	{
			fingerx[_]=-1;
			fingery[_]=-1;
	}

		//init static arr
	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event0");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	task_loop(); //进入任务循环
	return 0;
}
