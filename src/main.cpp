#include <iostream>
#include <bmp.hxx>
#include <format>

using bmp::rgb;
namespace draw = bmp::draw;

auto process(bmp::image image) {
  const auto [width, height] = image.dimension();

  std::cout << std::endl;
  bmp::display(std::cout, image);
  std::cout << std::endl;

  draw::line(image, {0, 0}, {width - 1, height - 1}, rgb::splat(255));
  draw::line(image, {width - 1, 0}, {0, height - 1}, rgb::splat(255));

  bmp::display(std::cout, image);
  std::cout << std::endl;

  std::string file;

  std::cout << "out file path: ";
  std::cin >> file;

  image.save(file);
}

auto main() -> int {
  std::string file;

  std::cout << "bmp file path: ";
  std::cin >> file;

  try {
    if (auto bmp = bmp::image::load(file); bmp.has_value()) {
      process(std::move(*bmp));
    } else if (bmp.error() == bmp::error::invalid_format)
      std::cout << "error: invalid bmp file";
    else if (bmp.error() == bmp::error::invalid_depth)
      std::cout << "error: unsupported depth\n";
    else
      std::cout << "unexpected!\n";
  } catch (std::ios_base::failure& exception) {
    std::cout << std::format("file `{}` error: ", file) << exception.what();
  }
}
