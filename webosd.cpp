#include <chrono>
#include <vdr/device.h>
#include "fpng.h"
#include "webosd.h"
#include "server.h"
#include "log.h"

cOsd *cWebOsdProvider::CreateOsd(int Left, int Top, uint Level) {
    debug2("%s, %d, %d, %d", __PRETTY_FUNCTION__, Left, Top, Level);

    return new cWebOsd(*this, Left, Top, Level);
}

cWebOsdProvider::cWebOsdProvider() : cOsdProvider() {
    debug2("%s", __PRETTY_FUNCTION__);
}

cWebOsdProvider::~cWebOsdProvider() {
    debug2("%s", __PRETTY_FUNCTION__);
}

cWebOsd::cWebOsd(cWebOsdProvider &Provider, int Left, int Top, uint Level) : cOsd(Left, Top, Level) {
    debug2("%s, %d, %d, %d", __PRETTY_FUNCTION__, Left, Top, Level);

    WebBridgeServer->sendSize();
}

cWebOsd::~cWebOsd() {
    WebBridgeServer->sendClearOsd();
}

void cWebOsd::Flush() {
    if (!cOsd::Active())
        return;

    if (IsTrueColor()) {
        LOCK_PIXMAPS;
        int left = Left();
        int top = Top();
        const cRect *vp;
        int vx, vy, vw, vh;
        int x, y;
        const uint8_t *pixel;

        std::vector<uint8_t> outbuffer;
        while (cPixmapMemory *pm = dynamic_cast<cPixmapMemory *>(RenderPixmaps())) {
            vp = &pm->ViewPort();
            vx = vp->X();
            vy = vp->Y();
            vw = vp->Width();
            vh = vp->Height();
            pixel = pm->Data();

            // convert
            uint32_t *argb = (uint32_t *) pixel;
            for (int i = 0; i < vw*vh; ++i) {
                // argb[i] = (argb[i] >> 24) | (argb[i] << 8);
                argb[i] = ((argb[i] & 0x00FF0000) >> 16) | ((argb[i] & 0x0000FF00)) | ((argb[i] & 0x000000FF) << 16)
                    | ((argb[i] & 0xFF000000));
            }

            outbuffer.clear();
            bool pngResult = fpng::fpng_encode_image_to_memory(argb, vw, vh, 4, outbuffer, 0);
            WebBridgeServer->sendPngImage(left + vx, top + vy, vw, vh, outbuffer.size(), outbuffer.data());

            DestroyPixmap(pm);
        }
    }
}

