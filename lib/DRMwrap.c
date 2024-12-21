#include "DRMwrap.h"
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

drmModeConnector *conn;
drmModeRes *res;
drmModePlaneRes *plane_res;
uint32_t conn_id;
uint32_t crtc_id;
uint32_t plane_id;

void DRMinit(int fd)
{
    // 获取crtc、encoder、connector属性信息
    res = drmModeGetResources(fd);
    crtc_id = res->crtcs[0];
    conn_id = res->connectors[0];

    drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    plane_res = drmModeGetPlaneResources(fd);
    if(plane_res == NULL)
    {
        perror("drmModeGetPlaneResources() failed");
        exit(0);
    }

    plane_id = plane_res->planes[0];
}

int DRMcreateFB(int fd, struct drmHandle *drm)
{
    // 获取连接器信息，包含显示器分辨率信息
    conn = drmModeGetConnector(fd, conn_id);
    if(conn == NULL)
    {
        perror("drmModeGetConnector()失败");
        return -1;
    }
    drm->width  = conn->modes[0].hdisplay;
    drm->height = conn->modes[0].vdisplay;

    struct drm_mode_create_dumb create = {};
    struct drm_mode_map_dumb map = {};
    int retcode;

    // 创建一块连续物理内存，大小等于显示屏的大小
    create.width = drm->width;
    create.height = drm->height;
    create.bpp = 32;
    retcode = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
    if(retcode != 0)
    {
        perror("drmIoctl(DRM_IOCTL_MODE_CREATE_DUMB)失败");
        return retcode;
    }

    /* bind the dumb-buffer to an FB object */
    drm->pitch = create.pitch;
    drm->size = create.size;
    drm->handle = create.handle;
    retcode = drmModeAddFB(fd, drm->width, drm->height, 24/*depth*/, 32/*bpp*/, drm->pitch,
                    drm->handle, &drm->fb_id);
    if(retcode != 0)
    {
        perror("drmModeAddFB()失败");
        return retcode;
    }

    // 将显存映射到用户空间
    map.handle = create.handle;
    retcode = drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    if(retcode != 0)
    {
        perror("drmIoctl(DRM_IOCTL_MODE_MAP_DUMB)失败");
        return retcode;
    }

    drm->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, map.offset);
    if(drm->vaddr == MAP_FAILED)
    {
        perror("mmap()失败");
        return -1;
    }
    return 0;
}

int DRMshowUp(int fd, struct drmHandle *drm)
{
    int retcode;
    retcode = drmModeSetCrtc(fd, crtc_id, drm->fb_id, 0, 0, &conn_id, 1, &conn->modes[0]);

    if(retcode != 0)
    {
        perror("drmModeSetCrtc()失败");
        return retcode;
    }
    return 0;
}

void DRMfreeResources(int fd, struct drmHandle *drm)
{
    struct drm_mode_destroy_dumb destroy = {};
    drmModeRmFB(fd, drm->fb_id);
    munmap(drm->vaddr, drm->size);

    destroy.handle = drm->handle;
    drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);

    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
}
