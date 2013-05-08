#ifndef GONKDISPLAYJB_H
#define GONKDISPLAYJB_H

#include "libdisplay/GonkDisplay.h"
#include "FramebufferSurface.h"
#include "hardware/hwcomposer.h"
#include "utils/RefBase.h"

namespace mozilla {

class GonkDisplayJB : public GonkDisplay {
public:
    GonkDisplayJB();
    ~GonkDisplayJB();

    virtual ANativeWindow* GetNativeWindow();

    virtual void SetEnabled(bool enabled);

    virtual void* GetHWCDevice();

    virtual bool SwapBuffers(EGLDisplay dpy, EGLSurface sur);

private:
    hw_module_t const*        mModule;
    hwc_composer_device_1_t*  mHwc;
    android::sp<android::FramebufferSurface> mFBSurface;
    android::sp<ANativeWindow> mSTClient;
    hwc_display_contents_1_t* mList;
    uint32_t mWidth;
    uint32_t mHeight;
};

}

#endif /* GONKDISPLAYJB_H */
