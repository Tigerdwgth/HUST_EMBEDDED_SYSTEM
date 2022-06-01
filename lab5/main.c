#include "../common/common.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include "audio_util.h"

/*PYNQ-Z1的pdm采样频率*/
#define PDM_SAMPLE_RATE 3072000 /* 3.072MHz */

/*语音识别要求的pcm采样频率*/
#define PCM_SAMPLE_RATE 16000 /* 16KHz */
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
int pdm_byte_n = 3*(PDM_SAMPLE_RATE >> 3);
uint8_t *pdm_buf;
static char * send_to_vosk_server(char *file);
static void touch_event_cb(int fd)
{

	int type,x,y,finger;

	fb_image *img;

	type = touch_read(fd, &x,&y,&finger);
	switch(type){
	case TOUCH_PRESS:
		printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		//press to start recording
		
		board_audio_record((uint16_t *)pdm_buf, pdm_byte_n/2);
		printf("record end\n");
		break;
	case TOUCH_MOVE:
		//do nothing
		break;
	case TOUCH_RELEASE:
		printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\t",x,y,finger);
		// process
		pcm_info_st pcm_info;
		uint8_t *pcm_buf = pdm_to_pcm_s16_mono(pdm_buf, pdm_byte_n, PDM_SAMPLE_RATE, 64, &pcm_info);
		/* 3072000/64 --> 48KHz 音频数据*/
		pcm_info_st pcm_info2;
		uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info2);
		/*48KHz --> 16KHz 音频数据*/
		pcm_write_wav_file(pcm_buf2, &pcm_info2, "/tmp/test.wav");
		printf("write wav end\n");
		pcm_free_buf(pdm_buf);
		pcm_free_buf(pcm_buf);
		pcm_free_buf(pcm_buf2);
		//do recognition
		char *rev = send_to_vosk_server("/tmp/test.wav");
		printf("recv from server: %s\n", rev);
		//draw the result
		if(strlen(rev)>0)
		{
			if(strcmp(rev,"开灯")==0)
			{
				img = fb_read_jpeg_image("./turnup.png");

			}
			else if(strcmp(rev,"关灯")==0)
			{
				img = fb_read_jpeg_image("./turndown.png");
			}
			fb_draw_image(0,0,img,0);
			fb_free_image(img);
			fb_draw_text(100,50,rev,50,BLACK);
		}
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

	pdm_buf = malloc(pdm_byte_n);
	fb_init("/dev/fb0");
	font_init("./font.ttc");
	fb_draw_rect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT,COLOR_BACKGROUND);

	fb_image *img;
	img = fb_read_jpeg_image("./turndown.png");
	fb_draw_image(0,0,img,0);
	fb_free_image(img);
	fb_draw_text(100,50,"press to start",50,BLACK);
	fb_update();
	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event0");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);
	task_loop(); //进入任务循环


	// int pdm_byte_n = 3*(PDM_SAMPLE_RATE >> 3);
	// uint8_t *pdm_buf = malloc(pdm_byte_n);
	// board_audio_record((uint16_t *)pdm_buf, pdm_byte_n/2);
	// printf("record end\n");

	// pcm_info_st pcm_info;
	// uint8_t *pcm_buf = pdm_to_pcm_s16_mono(pdm_buf, pdm_byte_n, PDM_SAMPLE_RATE, 64, &pcm_info);
	// /* 3072000/64 --> 48KHz 音频数据*/

	// pcm_info_st pcm_info2;
	// uint8_t *pcm_buf2 = pcm_s16_mono_resample(pcm_buf, &pcm_info, PCM_SAMPLE_RATE, &pcm_info2);
	// /*48KHz --> 16KHz 音频数据*/

	// pcm_write_wav_file(pcm_buf2, &pcm_info2, "/tmp/test.wav");
	// printf("write wav end\n");
	// pcm_free_buf(pdm_buf);
	// pcm_free_buf(pcm_buf);
	// pcm_free_buf(pcm_buf2);
	// printf("free end\n");
	// char *rev = send_to_vosk_server("/tmp/test.wav");
	// printf("recv from server: %s\n", rev);
	return 0;
}

/*===============================================================*/	

#define IP "127.0.0.1"
#define PORT 8011

#define print_err(fmt, ...) \
	printf("%d:%s " fmt, __LINE__, strerror(errno), ##__VA_ARGS__);

static char * send_to_vosk_server(char *file)
{
	static char ret_buf[128]; //识别结果

	if((file == NULL)||(file[0] != '/')) {
		print_err("file %s error\n", file);
		return NULL;
	}

	int skfd = -1, ret = -1;
	skfd = socket(AF_INET, SOCK_STREAM, 0);
	if(skfd < 0) {
		print_err("socket failed\n");
		return NULL;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(IP);
	ret = connect(skfd,(struct sockaddr*)&addr, sizeof(addr));
	if(ret < 0) {
		print_err("connect failed\n");
		close(skfd);
		return NULL;
	}

	printf("send wav file name: %s\n", file);
	ret = send(skfd, file, strlen(file)+1, 0);
	if(ret < 0) {
		print_err("send failed\n");
		close(skfd);
		return NULL;
	}

	ret = recv(skfd, ret_buf, sizeof(ret_buf)-1, 0);
	if(ret < 0) {
		print_err("recv failed\n");
		close(skfd);
		return NULL;
	}
	ret_buf[ret] = '\0';
	return ret_buf;
}
