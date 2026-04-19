/*
   <student1> <student_number>, <student2> <student_number>

   Estimated hours used per student: <hours used for this ex>

  [ ] AI Used in solving the exercise
      - Explain how AI was used in solving the exercise
*/

#include <cstdint>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstring>
#include <cmath>

using std::cout;


/*
 * Helper functions, do not modify these 
 */

constexpr int WIDTH = 352;
constexpr int HEIGHT = 288;
constexpr int BUFFER_SIZE = WIDTH * HEIGHT;

struct image_rgb {
  std::vector<uint8_t> r;
  std::vector<uint8_t> g;
  std::vector<uint8_t> b;
};

image_rgb load_bitmap_rgb(const char* filename)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to open file");

  // BMP header is typically 54 bytes
  uint8_t header[54];
  file.read(reinterpret_cast<char*>(header), 54);

  if (header[0] != 'B' || header[1] != 'M')
    throw std::runtime_error("Not a BMP file");

  int dataOffset = *reinterpret_cast<int*>(&header[10]);
  int width = *reinterpret_cast<int*>(&header[18]);
  int height = *reinterpret_cast<int*>(&header[22]);
  int bpp = *reinterpret_cast<uint16_t*>(&header[28]);

  if (width != WIDTH || height != HEIGHT)
    throw std::runtime_error("Unexpected image size");

  if (bpp != 24)
    throw std::runtime_error("Only 24-bit BMP supported");

  // Move to pixel data
  file.seekg(dataOffset, std::ios::beg);

  // Each row is padded to 4-byte boundary
  int rowPadded = (width * 3 + 3) & (~3);

  std::vector<uint8_t> row(rowPadded);

  image_rgb img;
  img.r.resize(width * height);
  img.g.resize(width * height);
  img.b.resize(width * height);

  // BMP stores pixels bottom-up
  for (int y = height - 1; y >= 0; --y)
  {
    file.read(reinterpret_cast<char*>(row.data()), rowPadded);

    for (int x = 0; x < width; ++x)
    {
      int i = y * width + x;
      int j = x * 3;

      img.b[i] = row[j + 0];
      img.g[i] = row[j + 1];
      img.r[i] = row[j + 2];
    }
  }

  return img;
}

void save_grayscale_bmp(const char* filename,
  const uint8_t *gray)
{
  std::ofstream file(filename, std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to open file for writing");

  const int rowPadded = (WIDTH * 3 + 3) & (~3);
  const int dataSize = rowPadded * HEIGHT;
  const int fileSize = 54 + dataSize;

  uint8_t header[54] = { 0 };

  // BMP Header
  header[0] = 'B';
  header[1] = 'M';

  *reinterpret_cast<uint32_t*>(&header[2]) = fileSize;
  *reinterpret_cast<uint32_t*>(&header[10]) = 54; // pixel data offset

  // DIB Header (BITMAPINFOHEADER)
  *reinterpret_cast<uint32_t*>(&header[14]) = 40;       // header size
  *reinterpret_cast<int32_t*>(&header[18]) = WIDTH;
  *reinterpret_cast<int32_t*>(&header[22]) = HEIGHT;
  *reinterpret_cast<uint16_t*>(&header[26]) = 1;        // planes
  *reinterpret_cast<uint16_t*>(&header[28]) = 24;       // bpp
  *reinterpret_cast<uint32_t*>(&header[34]) = dataSize; // image size

  file.write(reinterpret_cast<char*>(header), 54);

  std::vector<uint8_t> row(rowPadded);

  // Write bottom-up (BMP format)
  for (int y = HEIGHT - 1; y >= 0; --y)
  {
    for (int x = 0; x < WIDTH; ++x)
    {
      uint8_t g = gray[y * WIDTH + x];

      int j = x * 3;
      row[j + 0] = g; // B
      row[j + 1] = g; // G
      row[j + 2] = g; // R
    }

    // zero padding if needed
    for (int i = WIDTH * 3; i < rowPadded; ++i)
      row[i] = 0;

    file.write(reinterpret_cast<char*>(row.data()), rowPadded);
  }
}

void rgb2greyscale(const image_rgb &img, uint8_t* grey)
{
  size_t size = img.r.size();

  for (size_t i = 0; i < size; ++i)
  {
    grey[i] = static_cast<uint8_t>(
      0.299f * img.r[i] +
      0.587f * img.g[i] +
      0.114f * img.b[i]
      );
  }
}

double compute_psnr(const uint8_t *img1,
  const uint8_t *img2)
{
  double mse = 0.0;

  for (size_t i = 0; i < BUFFER_SIZE; ++i)
  {
    int diff = int(img1[i]) - int(img2[i]);
    mse += diff * diff;
  }

  mse /= static_cast<double>(BUFFER_SIZE);

  if (mse == 0.0)
    return INFINITY; // identical images

  const double maxPixel = 255.0;
  double psnr = 10.0 * std::log10((maxPixel * maxPixel) / mse);

  return psnr;
}


/*
 * MAIN TASK STARTS HERE. Modify the ex9_deblock() stub with your algorithm.
 */

void ex9_deblock(uint8_t* img) {
  constexpr int block_width = 16;
  constexpr int block_height = 16;

  // loop vertically
  // 6 blocks around the border are checked: i, i+1, i+2, i+3, i+4, i + 5
  for (size_t i = 0; i < HEIGHT; ++i) {
    for (size_t j = block_width - 2; j < WIDTH - 3; j += block_width) {
      // img[i * WIDTH + j] = 0;
      size_t border = i * WIDTH + j;
      uint8_t p3 = img[border - 1];
      uint8_t p2 = img[border + 0];
      uint8_t p1 = img[border + 1];
      uint8_t p0 = img[border + 2];
      uint8_t q0 = img[border + 3];
      uint8_t q1 = img[border + 4];
      uint8_t q2 = img[border + 5];
      uint8_t q3 = img[border + 6];

      img[border] = (2 * p3 +  3 * p2 + p1 + p0 + q0 + 4) >> 3; // p2
      img[border + 1] = (p2 + p1 + p0 + q0 + 2) >> 2; // p1
      img[border + 2] = (p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3; // p0

      img[border + 3] = (q2 + 2 * q1 + 2 * q0 + 2 * p0 + p1 + 4) >> 3; // q0
      img[border + 4] = (q2 + q1 + q0 + p0 + 2) >> 2; // q1
      img[border + 5] = (2 * q3 + 3 * q2 + q1 + q0 + p0 + 4) >> 3; // q2
    }
  }

  // loop horizontally
  for (size_t i = block_width - 2; i < HEIGHT - 3; i += block_width) {
    for (size_t j = 0; j < WIDTH; ++j) {
      // img[i * HEIGHT + j] = 0;
      uint8_t p3 = img[(i - 1) * WIDTH + j];
      uint8_t p2 = img[(i - 0) * WIDTH + j];
      uint8_t p1 = img[(i + 1) * WIDTH + j];
      uint8_t p0 = img[(i + 2) * WIDTH + j];
      uint8_t q0 = img[(i + 3) * WIDTH + j];
      uint8_t q1 = img[(i + 4) * WIDTH + j];
      uint8_t q2 = img[(i + 5) * WIDTH + j];
      uint8_t q3 = img[(i + 6) * WIDTH + j];

      img[(i - 0) * WIDTH + j] = (2 * p3 +  3 * p2 + p1 + p0 + q0 + 4) >> 3; // p2
      img[(i + 1) * WIDTH + j] = (p2 + p1 + p0 + q0 + 2) >> 2; // p1
      img[(i + 2) * WIDTH + j] = (p2 + 2 * p1 + 2 * p0 + 2 * q0 + q1 + 4) >> 3; // p0

      img[(i + 3) * WIDTH + j] = (q2 + 2 * q1 + 2 * q0 + 2 * p0 + p1 + 4) >> 3; // q0
      img[(i + 4) * WIDTH + j] = (q2 + q1 + q0 + p0 + 2) >> 2; // q1
      img[(i + 5) * WIDTH + j] = (2 * q3 + 3 * q2 + q1 + q0 + p0 + 4) >> 3; // q2
    }
  }
}

int main() {
  // Change paths if needed
  constexpr char* path_orig = "original.bmp";
  constexpr char* path_deblock = "encoded_with_deblock.bmp";
  constexpr char* path_nodeblock = "encoded_no_deblock.bmp";
  constexpr char* path_exercise_output = "exercise_output.bmp";
  
  const image_rgb pic_orig = load_bitmap_rgb(path_orig);
  const image_rgb pic_deblock = load_bitmap_rgb(path_deblock);
  const image_rgb pic_nodeblock = load_bitmap_rgb(path_nodeblock);

  uint8_t greyscale_orig[BUFFER_SIZE];
  rgb2greyscale(pic_orig, greyscale_orig);

  uint8_t greyscale_deblock[BUFFER_SIZE];
  rgb2greyscale(pic_deblock, greyscale_deblock);

  uint8_t greyscale_nodeblock[BUFFER_SIZE];
  rgb2greyscale(pic_nodeblock, greyscale_nodeblock);

  uint8_t greyscale_exercise[BUFFER_SIZE];
  memcpy(greyscale_exercise, greyscale_nodeblock, sizeof(uint8_t) * BUFFER_SIZE);
  ex9_deblock(greyscale_exercise);

  save_grayscale_bmp(path_exercise_output, greyscale_exercise);

  const double psnr_deblock = compute_psnr(greyscale_orig, greyscale_deblock);
  const double psnr_nodeblock = compute_psnr(greyscale_orig, greyscale_nodeblock);
  const double psnr_exercise = compute_psnr(greyscale_orig, greyscale_exercise);

  cout << "PSNR deblocked  " << psnr_deblock << "\n";
  cout << "PSNR no deblock " << psnr_nodeblock << "\n";
  cout << "PSNR exercise " << psnr_exercise << "\n\n";

  cout << "Check the result image " << path_exercise_output << "\n";

  return 0;
}