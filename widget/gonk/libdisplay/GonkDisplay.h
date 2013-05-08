#ifndef GONKDISPLAY_H
#define GONKDISPLAY_H

#include <system/window.h>

namespace mozilla {

class GonkDisplay {
public:
    virtual ANativeWindow* GetNativeWindow() = 0;

    virtual void SetEnabled(bool enabled) = 0;

    virtual void* GetHWCDevice() = 0;

    virtual bool SwapBuffers(void* dpy, void* sur) = 0;

    uint32_t xdpi;
    uint32_t surfaceformat;
};

GonkDisplay* GetGonkDisplay();

}
#endif /* GONKDISPLAY_H */
