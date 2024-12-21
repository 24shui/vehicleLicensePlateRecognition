extern "C"
{
#include <stdio.h>
#include <errno.h>
#include <linux/fb.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "DRMwrap.h"
#include <stdio.h>
#include <memory.h>
#include <sys/time.h>
#include <stdlib.h>
}

#include "rockx.h"

// DRM设备操作结构体
struct drmHandle drm;
// 初始化摄像头
int init_video();
// 获取bmp图片
int get_bmp(int fd);

int show_bmp(const char *pathname, struct drmHandle *drm, int lcd)
{

	// 指向缓存
	unsigned int *lcd_p = (unsigned int *)drm->vaddr;

	// 打开bmp图片
	int bmp_fd = open(pathname, O_RDWR);
	if (bmp_fd < 0)
	{
		printf("open fail %s\n", pathname);
		return 0;
	}

	// 读取 54个自己头数据
	char head[54];
	read(bmp_fd, head, 54);

	int width = *((int *)&head[18]);
	int height = *((int *)&head[22]);
	// printf("图片的 %d %d\n",width,height);

	unsigned int color[height][width];
	unsigned char buf[width * height * 3];
	read(bmp_fd, buf, sizeof(buf));

	unsigned char *p = buf;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			unsigned char b = *p++;
			unsigned char g = *p++;
			unsigned char r = *p++;
			unsigned char a = 0;

			color[y][x] = a << 24 | r << 16 | g << 8 | b;
		}
	}

	// 显示图像
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			 *(lcd_p + y * drm->width + x) = color[y][x];
		}
	}

	// 更新 DRM 设备画面
	DRMshowUp(lcd, drm);

	// 关闭bmp
	close(bmp_fd);
}

rockx_handle_t carplate_det_handle;
rockx_handle_t carplate_align_handle;
rockx_handle_t carplate_recog_handle;
int init_rock()
{
	// 开始识别 车牌
	rockx_ret_t ret;
	struct timeval tv;

	// 创建车牌检查模块
	ret = rockx_create(&carplate_det_handle, ROCKX_MODULE_CARPLATE_DETECTION, nullptr, 0);
	if (ret != ROCKX_RET_SUCCESS)
	{
		printf("init rockx module ROCKX_MODULE_CARPLATE_DETECTION error %d\n", ret);
		return -1;
	}
	// 创建车牌校准模块
	ret = rockx_create(&carplate_align_handle, ROCKX_MODULE_CARPLATE_ALIGN, nullptr, 0);
	if (ret != ROCKX_RET_SUCCESS)
	{
		printf("init rockx module ROCKX_MODULE_CARPLATE_ALIGN error %d\n", ret);
		return -1;
	}
	// 创建车牌识别模块
	ret = rockx_create(&carplate_recog_handle, ROCKX_MODULE_CARPLATE_RECOG, nullptr, 0);
	if (ret != ROCKX_RET_SUCCESS)
	{
		printf("init rockx module ROCKX_MODULE_OBJECT_DETECTION error %d\n", ret);
		return -1;
	}
}

char *get_car_id(const char *img_path)
{
	rockx_ret_t ret;

	// read image  读取图片
	rockx_image_t input_image;
	rockx_image_read(img_path, &input_image, 1);

	// 车牌对象数组
	rockx_object_array_t carplate_array;
	memset(&carplate_array, 0, sizeof(rockx_object_array_t));

	// detect carplate  检测车牌，并把检测到的车牌放入数据
	ret = rockx_carplate_detect(carplate_det_handle, &input_image, &carplate_array, nullptr);
	if (ret != ROCKX_RET_SUCCESS)
	{
		printf("rockx_carplate_detect error %d\n", ret);
		return NULL;
	}

	static char platename[32];

	// 遍历所有检测到的车牌
	for (int i = 0; i < carplate_array.count; i++)
	{
		// create rockx_carplate_align_result_t for store result
		rockx_carplate_align_result_t result; // 车牌截取对象
		memset(&result, 0, sizeof(rockx_carplate_align_result_t));

		// 车牌的所在位置  和  准确度
		printf("(%d %d %d %d) %f\n", carplate_array.object[i].box.left, carplate_array.object[i].box.top,
			   carplate_array.object[i].box.right, carplate_array.object[i].box.bottom, carplate_array.object[i].score);

		// carplate_fmapping_   //根据车牌的位置截取车牌
		ret = rockx_carplate_align(carplate_align_handle, &input_image, &carplate_array.object[i].box, &result);
		if (ret != ROCKX_RET_SUCCESS)
		{
			printf("rockx_carplate_align error %d\n", ret);
			return NULL;
		}

		// save image  保存截取的车牌图片
		rockx_image_write("./refined_img.jpg", &(result.aligned_image));

		// recognize carplate number
		rockx_carplate_recog_result_t recog_result; // 开始识别车牌，并获取识别结果
		ret = rockx_carplate_recognize(carplate_recog_handle, &(result.aligned_image), &recog_result);

		// remember release aligned image
		rockx_image_release(&(result.aligned_image)); // 释放图片

		if (ret != ROCKX_RET_SUCCESS)
		{
			printf("rockx_face_detect error %d\n", ret);
			return NULL;
		}

		// process result

		memset(platename, 0, 32); // 输出识别结果
		for (int n = 0; n < recog_result.length; n++)
		{
			strcat(platename, CARPLATE_RECOG_CODE[recog_result.namecode[n]]);
		}
		printf("carplate: %s\n", platename);
	}

	// release
	rockx_image_release(&input_image);

	if (carplate_array.count < 1)
	{
		// 清空车牌
		memset(platename, 0, 32);
	}

	return platename;
}

// 释放资源
void free_rock()
{

	rockx_destroy(carplate_det_handle);
	rockx_destroy(carplate_align_handle);
	rockx_destroy(carplate_recog_handle);
}
int video_fd; // 打开摄像头



int lcd;	  // 打开 DRM 设备

void license_plate(void);
void show_contrl(void);
int main()
{
	// 打开 DRM 设备
	lcd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	// 初始化 DRM 设备
	DRMinit(lcd);

	// 为显示屏添加一个FrameBuffer显存，并获取显示屏的分辨率、显存入口和色深等信息
	DRMcreateFB(lcd, &drm);
	int bpp = drm.pitch / drm.width * 8;
	printf("显示器尺寸: %d×%d\n", drm.width, drm.height);
	printf("色深: %u字节\n", bpp / 8);

	// 初始化摄像头
	video_fd = init_video();

	// 初始化 ROCK
	init_rock();

	// 1、触摸屏初始化



	// 2、开机动画

	int x = 513;
	while (1)
	{
		// 3、获取坐标

		// 4、判断
		if (x < 512)
		{
			// 电子相册
			show_contrl();
		}

		if (x > 512)
		{
			// 车牌识别
			license_plate();
		}
	}

	// 释放 DRM 设备相关资源
	DRMfreeResources(lcd, &drm);

	// 释放资源
	free_rock();

	return 0;
}

// 车牌识别
void license_plate(void)
{
	int i = 1;
	// printf("输入1开始识别车牌\n");

	// scanf("%d", &i);
	if (i == 1)
	{
		// 获取bmp图片
		get_bmp(video_fd);

		// 显示bmp 图片
		show_bmp("0.bmp", &drm, lcd);

		char tmp0[32] = {0};
		// 获取车牌
		strcpy(tmp0, get_car_id("0.bmp"));

		printf("tmp0:%s\n", tmp0); // 打印车牌
	}
}

void show_contrl(void)
{
	char *buf[10] = {"1.bmp", "2.bmp", "3.bmp"}; // 3张
	int buf_flag = 0;							 // 数组下标，控制图片 0~2
	int x = 0;
	while (1)
	{
		// 获得坐标
		// get_xy();

		if (x < 300) // 上一张
		{
			buf_flag++; //
			if (buf_flag > 2)
			{
				buf_flag = 0;
			}
			// 显示图片
			show_bmp(buf[buf_flag], &drm, lcd);
			// show(buf[buf_flag]);
		}

		// 下一张 buf_flag--; 最小值是0
		if (x > 300 && x < 600)
		{
		}

		// 退出
		if (x > 600)
		{
			break;
		}
	}
}