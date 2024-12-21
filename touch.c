// input event buf
// time 时间触发时间
// type 记录事件类型
// code 记录事件代码(事件具体类型)
// value 记录事件值
#include <stdio.h>
#include <stdlib.h>
// open 函数头文件
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
// read 函数头文件
#include <unistd.h>
// input event头文件
#include <linux/input.h>
#include <math.h>
#include <xf86drm.h>
#include "DRMwrap.h"

int flag;
int start_x, start_y = 0;
int end_x = 0;
int end_y = 0;
int touch_active = 0;

int touch()
{
    // 打开触摸屏设备文件
    int touch_fd = open("/dev/input/event0", O_RDONLY);
    if (touch_fd == -1)
    {
        perror("open touch device error");
        return -1;
    }

    struct input_event buf;

    while (1)
    {
        if (read(touch_fd, &buf, sizeof(buf)) > 0)
        {
            if (buf.type == EV_ABS)
            {
                if (buf.code == ABS_X)
                {
                    printf("x= %d\n", buf.value);
                    if (touch_active)
                    {
                        end_x = buf.value; // 更新当前X坐标
                    }
                }
                if (buf.code == ABS_Y)
                {
                    printf("y= %d\n", buf.value);
                    if (touch_active)
                    {
                        end_y = buf.value; // 更新当前Y坐标
                    }
                }
            }

            if (buf.type == EV_KEY && buf.code == BTN_TOUCH)
            {
                if (buf.value == 0)
                { // 抬起
                    printf("Touch up\n");
                    touch_active = 0; // 重置触摸状态
                    printf("End: (%d, %d)\n", end_x, end_y);
                    printf("Delta x: %d, Delta y: %d\n", end_x - start_x, end_y - start_y);
                    break;
                }
                else if (buf.value == 1)
                { // 按下
                    printf("Touch down\n");
                    touch_active = 1; // 设置触摸状态
                    start_x = end_x;  // 记录起点坐标
                    start_y = end_y;  // 记录起点坐标
                    printf("Start: (%d, %d)\n", start_x, start_y);
                }
            }
        }
    }

    // 关闭触摸屏设备
    close(touch_fd);
}

int slide()
{
    flag = 0;

    if (abs(end_x - start_x) > abs(end_y - start_y))
    {
        if (end_x - start_x > 0)
        {
            flag = 2;
        }
        else if (end_x - start_x < 0)
        {
            flag = 1;
        }
        else
        {
            printf("没有移动\n");
        }
    }

    if (abs(end_x - start_x) < abs(end_y - start_y))
    {
        if (end_y - start_y > 0)
        {
            flag = 4;
        }
        else if (end_y - start_y < 0)
        {
            flag = 3;
        }
        else
        {
            printf("没有移动\n");
        }
    }

    return flag;
}

int lcd_fd;
int *lcd_ptr;
struct drmHandle drm;

// drm 设备显示 bmp 图片

int lcd_draw_bmp(int x, int y, int w, int h, const char *pathname)
{
    // 更新画面
    DRMshowUp(lcd_fd, &drm);

    lcd_ptr = (int *)drm.vaddr;

    int bmp_fd = open(pathname, O_RDWR);

    char header[54];
    char rgb_buf[w * h * 3];

    read(bmp_fd, header, 54);         // 读取文件头
    read(bmp_fd, rgb_buf, w * h * 3); // 读取颜色数据

    int i, j;
    int r, g, b, color;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            b = rgb_buf[(w * j + i) * 3 + 0];
            g = rgb_buf[(w * j + i) * 3 + 1];
            r = rgb_buf[(w * j + i) * 3 + 2];
            color = b | g << 8 | r << 16;
            lcd_ptr[1024 * (h - 1 - j + y) + i + x] = color;
        }
    }

    close(bmp_fd);
}

// 设备初始化

int dev_init()
{
    // 打开 drm 设备

    lcd_fd = open("/dev/dri/card0", O_RDWR);

    if (-1 == lcd_fd)
    {
        printf("open lcd device failed!\n");
        return -1;
    }

    // 初始化 drm 设备
    DRMinit(lcd_fd);

    // 申请内存

    DRMcreateFB(lcd_fd, &drm);
}

int dev_exit()
{
    DRMfreeResources(lcd_fd, &drm);
    close(lcd_fd);
}

int main()
{
    dev_init();

    while (1)
    {
        touch();
        slide();

        if (flag == 1)
        {
            printf("左滑\n");
            lcd_draw_bmp(256, 150, 512, 300, "1.bmp");
        }
        else if (flag == 2)
        {
            printf("右滑\n");
            lcd_draw_bmp(256, 150, 512, 300, "2.bmp");
        }
        else if (flag == 3)
        {
            printf("上滑\n");
        }
        else if (flag == 4)
        {
            printf("下滑\n");
        }
    }

    getchar();

    dev_exit();

    return 0;
}