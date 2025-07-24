#pragma once

#include <fstream>
#include <iostream>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>
#include <expected>
#include <optional>

namespace bmp {

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;

struct rgb {
  u8 r;
  u8 g;
  u8 b;

  static constexpr auto splat(u8 x) {
    return rgb{x, x, x};
  }
};

#pragma pack(push, 1)

struct file_header {
  u16 type = 0x4D42;
  u32 size;
  u16 _reserved1;
  u16 _reserved2;
  u32 offset;
};

struct info_header {
  u32 size;
  i32 width;
  i32 height;
  u16 planes;
  u16 bit_depth;
  u32 compress;
  u32 size_image;
  i32 xpels_per_meter;
  i32 ypels_per_meter;
  u32 clr_used;
  u32 clr_important;
};

#pragma pack(pop)

enum class error {
  /// not a bmp stream
  invalid_format,
  invalid_depth,
  todo,
};

struct header_t {
  file_header file_header;
  info_header info_header;

  void save(std::ofstream &file, const std::vector<u8> &pixels) const {
    file.write(reinterpret_cast<const char *>(&file_header), sizeof(file_header));
    file.write(reinterpret_cast<const char *>(&info_header), sizeof(info_header));

    file.seekp(file_header.offset, std::ios::beg);
    file.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
    file.close();
  }
};

class image {
  u32 _width;
  u32 _height;
  u16 _bit_depth;
  u32 _row_pad;
  std::vector<u8> _pixels;

  header_t header;

  explicit image(u32 width, u32 height, u16 bit_depth, u32 row_pad, std::vector<u8> pixels,
                 header_t header) noexcept
      : _width(width),
        _height(height),
        _bit_depth(bit_depth),
        _row_pad(row_pad),
        _pixels(std::move(pixels)),
        header(header) {}

 public:
  image(image &&other) noexcept
      : _width(other._width),
        _height(other._height),
        _bit_depth(other._bit_depth),
        _row_pad(other._row_pad),
        _pixels(std::move(other._pixels)),
        header(other.header) {}

  [[nodiscard]] static auto load(const std::string &filename) -> std::expected<image, error> {
    file_header file_header;
    info_header info_header;

    auto file = std::ifstream(filename, std::ios::binary);
    file.exceptions(std::ifstream::badbit | std::ifstream::failbit);

    file.read(reinterpret_cast<char *>(&file_header), sizeof(file_header));
    if (file_header.type != 0x4D42) {
      return std::unexpected(error::invalid_format);
    }

    file.read(reinterpret_cast<char *>(&info_header), sizeof(info_header));
    if (info_header.bit_depth != 24 && info_header.bit_depth != 32) {
      return std::unexpected(error::invalid_depth);
    }

    if (info_header.width < 0 || info_header.height < 0) {
      return std::unexpected(error::todo);
    }

    auto width = static_cast<u32>(info_header.width);
    auto height = static_cast<u32>(info_header.height);
    auto bit_depth = info_header.bit_depth;

    auto bytes = bit_depth / 8;
    auto row_pad = (4 - (width * bytes) % 4) % 4;

    auto row = width * bytes + row_pad;
    auto pixels = std::vector<u8>(row * height);

    file.seekg(file_header.offset, std::ios::beg);
    file.read(reinterpret_cast<char *>(pixels.data()), pixels.size());

    return image(width, height, bit_depth, row_pad, std::move(pixels),
                 header_t(file_header, info_header));
  }

  [[nodiscard]] auto in_bounds(u32 x, u32 y) const -> bool {
    return x < _width && y < _height;
  }

  [[nodiscard]] auto pixel_index(u32 x, u32 y) const -> u32 {
    return (y * (_width * (_bit_depth / 8) + _row_pad)) + (x * (_bit_depth / 8));
  }

  [[nodiscard]] auto pixel(u32 x, u32 y) const {
    return pixel_checked(x, y).value();
  }

  [[nodiscard]] auto pixel_checked(u32 x, u32 y) const -> std::optional<rgb> {
    if (not in_bounds(x, y)) {
      return std::nullopt;
    } else {
      auto index = pixel_index(x, y);
      return rgb(_pixels[index], _pixels[index + 1], _pixels[index + 2]);
    }
  }

  void set_pixel(u32 x, u32 y, rgb color) {
    if (not in_bounds(x, y)) {
      throw std::out_of_range("pixel out of image");
    }
    auto index = (y * (_width * (_bit_depth / 8) + _row_pad)) + (x * (_bit_depth / 8));

    auto [r, g, b] = color;
    _pixels[index] = b;
    _pixels[index + 1] = g;
    _pixels[index + 2] = r;
    if (_bit_depth == 32) {
      _pixels[index + 3] = 0;
    }
  }

  [[nodiscard]] auto &&pixels() const {
    return _pixels;
  }

  [[nodiscard]] auto &&pixels() {
    return _pixels;
  }

  [[nodiscard]] auto depth() const {
    return _bit_depth;
  }

  [[nodiscard]] auto width() const {
    return _width;
  }

  [[nodiscard]] auto height() const {
    return _height;
  }

  [[nodiscard]] auto dimension() const -> std::pair<u32, u32> {
    return {width(), height()};
  }

  void save(const std::string &filename) const {
    std::ofstream file(filename, std::ios::binary);
    file.exceptions(std::ifstream::badbit | std::ifstream::failbit);

    header.save(file, pixels());
  }
};

void display(std::ostream &out, const image &image) {
  const auto [width, height] = image.dimension();

  for (auto y = 0; y < height; y++) {
    for (auto x = 0; x < width; x++) {
      auto [r, g, b] = image.pixel(x, y);
      if (r == 0 && g == 0 && b == 0) {
        out << '*';
      } else if (r == 255 && g == 255 && b == 255) {
        out << ' ';
      } else {
        out << '?';
      }
      out << ' ';
    }
    out << std::endl;
  }
}

namespace draw {

void line(image &image, std::pair<i32, i32> from, std::pair<i32, i32> to, rgb color) {
  auto [x1, y1] = from;
  auto [x2, y2] = to;

  auto dx = std::abs(x2 - x1);
  auto dy = std::abs(y2 - y1);
  auto err = dx - dy;

  const auto sx = x1 < x2 ? 1 : -1;
  const auto sy = y1 < y2 ? 1 : -1;

  while (true) {
    image.set_pixel(x1, y1, color);
    if (x1 == x2 && y1 == y2)
      break;

    if (err * 2 > -dy) {
      err -= dy;
      x1 += sx;
    }
    if (err * 2 < dx) {
      err += dx;
      y1 += sy;
    }
  }
}

}  // namespace draw

}  // namespace  bmp
