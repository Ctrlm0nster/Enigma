#include "ImageDecoder.hpp"

#include <csetjmp>
#include <cstdio>
#include <jpeglib.h>

namespace workbench {
namespace {

struct JpegErrorManager {
  struct jpeg_error_mgr pub;
  jmp_buf setjmpBuffer;
};

void jpegErrorExit(j_common_ptr cinfo) {
  auto *err = reinterpret_cast<JpegErrorManager *>(cinfo->err);
  longjmp(err->setjmpBuffer, 1);
}

} // namespace

DecodedImage decodeJpeg(const std::string &path) {
  FILE *file = std::fopen(path.c_str(), "rb");
  if (!file) {
    return {0, 0, {}, false};
  }

  struct jpeg_decompress_struct cinfo {};
  JpegErrorManager jerr {};

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpegErrorExit;

  if (setjmp(jerr.setjmpBuffer)) {
    jpeg_destroy_decompress(&cinfo);
    std::fclose(file);
    return {0, 0, {}, false};
  }

  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, file);
  jpeg_read_header(&cinfo, TRUE);

#if defined(JCS_EXT_RGBA)
  cinfo.out_color_space = JCS_EXT_RGBA;
  jpeg_start_decompress(&cinfo);

  const int width = static_cast<int>(cinfo.output_width);
  const int height = static_cast<int>(cinfo.output_height);
  const int rowStride = width * 4;

  std::vector<uint8_t> rgba(static_cast<std::size_t>(width * height * 4));
  while (cinfo.output_scanline < cinfo.output_height) {
    auto *rowPointer = &rgba[cinfo.output_scanline * rowStride];
    jpeg_read_scanlines(&cinfo, &rowPointer, 1);
  }
#else
  cinfo.out_color_space = JCS_RGB;
  jpeg_start_decompress(&cinfo);

  const int width = static_cast<int>(cinfo.output_width);
  const int height = static_cast<int>(cinfo.output_height);
  const int rowStride = width * 3;

  std::vector<uint8_t> rgbRow(static_cast<std::size_t>(rowStride));
  std::vector<uint8_t> rgba(static_cast<std::size_t>(width * height * 4));

  while (cinfo.output_scanline < cinfo.output_height) {
    const auto line = cinfo.output_scanline;
    auto *rowPointer = rgbRow.data();
    jpeg_read_scanlines(&cinfo, &rowPointer, 1);
    for (int col = 0; col < width; ++col) {
      const auto dst = (line * width + col) * 4;
      const auto src = col * 3;
      rgba[dst + 0] = rgbRow[src + 0];
      rgba[dst + 1] = rgbRow[src + 1];
      rgba[dst + 2] = rgbRow[src + 2];
      rgba[dst + 3] = 255;
    }
  }
#endif

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  std::fclose(file);

  return {width, height, std::move(rgba), true};
}

} // namespace workbench
