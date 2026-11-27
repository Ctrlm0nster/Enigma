/**
 * @file ImageDecoder.hpp
 * @brief libjpeg-turbo image decoding routines.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace workbench {

struct DecodedImage {
  int width{0};
  int height{0};
  std::vector<uint8_t> rgba;
  bool valid{false};
};

DecodedImage decodeJpeg(const std::string &path);

} // namespace workbench
