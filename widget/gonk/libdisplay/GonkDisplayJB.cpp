#include "GonkDisplayJB.h"
#include <gui/SurfaceTextureClient.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>

using namespace android;

namespace mozilla {

static GonkDisplayJB* sGonkDisplay = nullptr;

GonkDisplayJB::GonkDisplayJB()
    : mList(nullptr)
    , mModule(nullptr)
    , mHwc(nullptr)
{
    int err = hw_get_module(HWC_HARDWARE_MODULE_ID, &mModule);
    ALOGW_IF(err, "%s module not found", HWC_HARDWARE_MODULE_ID);
    if (err)
        return;

    err = hwc_open_1(mModule, &mHwc);
    ALOGE_IF(err, "%s device failed to initialize (%s)",
             HWC_HARDWARE_COMPOSER, strerror(-err));

    int32_t values[3];
    const uint32_t attrs[] = {
        HWC_DISPLAY_WIDTH,
        HWC_DISPLAY_HEIGHT,
        HWC_DISPLAY_DPI_X,
        HWC_DISPLAY_NO_ATTRIBUTE
    };
    mHwc->getDisplayAttributes(mHwc, 0, 0, attrs, values);

    mWidth = values[0];
    mHeight = values[1];
    xdpi = values[2];
    surfaceformat = HAL_PIXEL_FORMAT_RGBA_8888;

    mFBSurface = new FramebufferSurface(0, mWidth, mHeight, surfaceformat);

    sp<SurfaceTextureClient> stc = new SurfaceTextureClient(static_cast<sp<ISurfaceTexture> >(mFBSurface->getBufferQueue()));
    mSTClient = stc;

    mList = (hwc_display_contents_1_t *)malloc(sizeof(*mList) + (sizeof(hwc_layer_1_t)*2));
    mHwc->blank(mHwc, HWC_DISPLAY_PRIMARY, 0);
}

GonkDisplayJB::~GonkDisplayJB()
{
    if (mHwc)
        hwc_close_1(mHwc);
    free(mList);
}

ANativeWindow*
GonkDisplayJB::GetNativeWindow()
{
    return mSTClient.get();
}

void
GonkDisplayJB::SetEnabled(bool enabled)
{
    mHwc->blank(mHwc, HWC_DISPLAY_PRIMARY, !enabled);
}

void*
GonkDisplayJB::GetHWCDevice()
{
    return mHwc;
}

bool
GonkDisplayJB::SwapBuffers(EGLDisplay dpy, EGLSurface sur)
{
    eglSwapBuffers(dpy, sur);
    hwc_display_contents_1_t *displays[HWC_NUM_DISPLAY_TYPES] = {NULL};
    const hwc_rect_t r = { 0, 0, mWidth, mHeight };
    displays[HWC_DISPLAY_PRIMARY] = mList;
    mList->retireFenceFd = -1;
    mList->numHwLayers = 2;
    mList->dpy = dpy;
    mList->sur = sur;
    mList->flags = HWC_GEOMETRY_CHANGED;
    mList->hwLayers[0].compositionType = HWC_BACKGROUND;
    mList->hwLayers[0].hints = 0;
    mList->hwLayers[0].flags = 0;
    mList->hwLayers[0].backgroundColor = {0};
    mList->hwLayers[1].compositionType = HWC_FRAMEBUFFER_TARGET;
    mList->hwLayers[1].hints = 0;
    mList->hwLayers[1].flags = 0;
    mList->hwLayers[1].handle = mFBSurface->lastHandle;
    mList->hwLayers[1].transform = 0;
    mList->hwLayers[1].blending = HWC_BLENDING_PREMULT;
    mList->hwLayers[1].sourceCrop = r;
    mList->hwLayers[1].displayFrame = r;
    mList->hwLayers[1].visibleRegionScreen.numRects = 1;
    mList->hwLayers[1].visibleRegionScreen.rects = &mList->hwLayers[0].sourceCrop;
    mList->hwLayers[1].acquireFenceFd = mFBSurface->lastFenceFD;
    mList->hwLayers[1].releaseFenceFd = -1;
    mHwc->prepare(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
    int err = mHwc->set(mHwc, HWC_NUM_DISPLAY_TYPES, displays);
//ALOGE("Render attempt: err %d, handle %d, fenceFD %d, %d x %d", err, sFBSurface->lastHandle, sFBSurface->lastFenceFD, gScreenBounds.width, gScreenBounds.height);
    mFBSurface->setReleaseFenceFd(mList->hwLayers[1].releaseFenceFd);
    if (mList->retireFenceFd >= 0)
        close(mList->retireFenceFd);
    return !err;
}

GonkDisplay*
GetGonkDisplay()
{
    if (!sGonkDisplay)
        sGonkDisplay = new GonkDisplayJB();
    return sGonkDisplay;
}

} // namespace mozilla
