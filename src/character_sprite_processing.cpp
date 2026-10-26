#include "character_creator.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

Image DecodeGeneratedCharacterImage(const std::string& bytes) {
    // Check dimensions before decoding to keep malformed/decompression-bomb responses bounded.
    if (bytes.size() < 24 || bytes.size() > 32 * 1024 * 1024 ||
        std::memcmp(bytes.data(), "\x89PNG\r\n\x1a\n", 8) != 0 || std::memcmp(bytes.data() + 12, "IHDR", 4) != 0)
        throw std::runtime_error("服务未返回有效的 PNG 图片");
    const auto dimension = [&](int offset) {
        unsigned n = 0; for (int i = 0; i < 4; ++i) n = (n << 8) | static_cast<unsigned char>(bytes[offset+i]); return n;
    };
    const auto w = dimension(16), h = dimension(20);
    if (w < 16 || h < 16 || w > 4096 || h > 4096) throw std::runtime_error("生成图片尺寸必须在 16 到 4096 之间");
    Image image = LoadImageFromMemory(".png", reinterpret_cast<const unsigned char*>(bytes.data()), static_cast<int>(bytes.size()));
    if (!image.data) throw std::runtime_error("生成图片解码失败");
    return image;
}

Image PrepareCharacterSprite(const Image& source, bool removeGreen) {
    if (!source.data || source.width < 1 || source.height < 1 || source.width > 4096 || source.height > 4096)
        throw std::runtime_error("无效的小人图片");
    Image out = ImageCopy(source); ImageFormat(&out, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if (!out.data) throw std::runtime_error("图片处理失败");
    auto* pixels = static_cast<Color*>(out.data);
    if (removeGreen) {
        const int w = out.width, h = out.height;
        std::vector<unsigned char> visited(static_cast<std::size_t>(w) * h);
        std::vector<int> queue; queue.reserve(static_cast<std::size_t>(w) * h);
        const auto add = [&](int i) {
            if (visited[i]) return;
            visited[i] = 1;
            const auto c = pixels[i];
            if (c.a == 0 || (c.g >= 120 && int(c.g) - std::max(c.r, c.b) >= 55)) queue.push_back(i);
        };
        for (int x = 0; x < w; ++x) { add(x); add((h-1)*w+x); }
        for (int y = 0; y < h; ++y) { add(y*w); add(y*w+w-1); }
        for (std::size_t n = 0; n < queue.size(); ++n) {
            const int i = queue[n], x = i % w, y = i / w;
            pixels[i].a = 0;
            if (x) add(i-1); if (x+1 < w) add(i+1); if (y) add(i-w); if (y+1 < h) add(i+w);
        }
    }
    bool visible = false;
    for (int i = 0; i < out.width*out.height; ++i) if (pixels[i].a > 16) { visible = true; break; }
    if (!visible) { UnloadImage(out); throw std::runtime_error("去背景后没有可见人物，请关闭去绿幕或重新生成"); }
    ImageAlphaCrop(&out, .05F);
    // Uniform transparent margins preserve feet alignment in the existing renderer.
    ImageResizeCanvas(&out, out.width + 12, out.height + 12, 6, 6, BLANK);
    return out;
}
