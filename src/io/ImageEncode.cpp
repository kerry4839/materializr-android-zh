// The ONE translation unit that owns the stb_image_write implementation.
#include "ImageEncode.h"

#include <cstdlib>
#include <zlib.h>

// Route stb's PNG deflate through zlib (already linked for project-file
// compression): noticeably smaller thumbnails than stb's own compressor.
// Contract: return a malloc'd buffer (stb frees it with STBIW_FREE).
static unsigned char* mzrZlibCompress(unsigned char* data, int dataLen,
                                      int* outLen, int quality) {
    uLongf bound = compressBound(static_cast<uLong>(dataLen));
    unsigned char* buf = static_cast<unsigned char*>(std::malloc(bound));
    if (!buf) return nullptr;
    if (compress2(buf, &bound, data, static_cast<uLong>(dataLen),
                  quality > 0 && quality <= 9 ? quality : 8) != Z_OK) {
        std::free(buf);
        return nullptr;
    }
    *outLen = static_cast<int>(bound);
    return buf;
}

#define STBIW_ZLIB_COMPRESS mzrZlibCompress
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO      // we only ever encode to memory
#include "../third_party/stb_image_write.h"

namespace materializr {

bool encodePng(const uint8_t* rgba, int width, int height,
               std::vector<uint8_t>& pngOut) {
    if (!rgba || width <= 0 || height <= 0) return false;
    int len = 0;
    unsigned char* png = stbi_write_png_to_mem(
        rgba, width * 4, width, height, 4, &len);
    if (!png || len <= 0) {
        if (png) STBIW_FREE(png);
        return false;
    }
    pngOut.assign(png, png + len);
    STBIW_FREE(png);
    return true;
}

} // namespace materializr
