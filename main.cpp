#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>

#include "FL_downSample3D.h"
#include "FL_upSample3D.h"
#include "cxxopts.hpp"
#include "stackutil.h"

struct ImageSamplingCreateInfo {
  std::string inputImagePath;
  std::string outputImagePath;
  std::array<int, 3> size = {0, 0, 0};
  double xResolutionFactor;
  double yResolutionFactor;
  double zResolutionFactor;
  int channel = 1;

  // tag == 0: using averaging to generate new pixel value
  // tag == 1: using subsampling to generate new pixel value
  unsigned char tag = 1;

  enum class SampleMode { Unknown, UpSample, DownSample };

  SampleMode mode;
};

class ImageSampler {
public:
  ImageSampler() = default;

  void setSamplingCreateInfo(const ImageSamplingCreateInfo &info) {
    m_Info = info;
  }

  bool resampleImage() {
    if (!std::filesystem::exists(m_Info.inputImagePath)) {
      std::cerr << "Input image does not exist: " << m_Info.inputImagePath
                << std::endl;
      return false;
    }

    std::cout << "Processing: " << m_Info.inputImagePath << std::endl;

    unsigned char *imgData = nullptr;
    int64_t *imgSizePtr = nullptr;
    int dataType;
    if (!loadImage((char *)m_Info.inputImagePath.c_str(), imgData, imgSizePtr,
                   dataType)) {
      std::cerr << "Load image " << m_Info.inputImagePath << " error!"
                << std::endl;
      return false;
    }

    // 计算当前体积大小
    int64_t currentVolume = imgSizePtr[0] * imgSizePtr[1] * imgSizePtr[2];
    int64_t targetVolume = 128 * 128 * 64; // 目标体积

    std::cout << "Original size: " << imgSizePtr[0] << "x" << imgSizePtr[1]
              << "x" << imgSizePtr[2] << std::endl;
    std::cout << "Original volume: " << currentVolume
              << ", Target volume: " << targetVolume << std::endl;

    // 如果当前体积超过目标体积，计算合适的缩放因子
    if (currentVolume > targetVolume) {
      m_Info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;

      // 计算需要的缩放因子，使缩放后的体积不超过目标体积
      double volumeRatio = (double)currentVolume / targetVolume;
      double scaleFactor =
          std::cbrt(volumeRatio); // 三维等比例缩放，每个维度缩放相同因子

      // 确保缩放因子不小于1
      scaleFactor = std::max(1.0, scaleFactor);

      m_Info.xResolutionFactor = scaleFactor;
      m_Info.yResolutionFactor = scaleFactor;
      m_Info.zResolutionFactor = scaleFactor;

      std::cout << "Need downsampling with scale factor: " << scaleFactor
                << std::endl;
    } else {
      // 如果当前体积小于等于目标体积，不需要缩放
      m_Info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;
      m_Info.xResolutionFactor = 1.0;
      m_Info.yResolutionFactor = 1.0;
      m_Info.zResolutionFactor = 1.0;

      std::cout
          << "No downsampling needed, volume is already within target limits"
          << std::endl;
    }

    std::cout << "x_factor = " << m_Info.xResolutionFactor << std::endl;
    std::cout << "y_factor = " << m_Info.yResolutionFactor << std::endl;
    std::cout << "z_factor = " << m_Info.zResolutionFactor << std::endl;
    std::cout << "ch = " << m_Info.channel << std::endl;
    std::cout << "outimg_file = " << m_Info.outputImagePath << std::endl;

    // 如果不需要缩放，直接复制原文件
    if (m_Info.xResolutionFactor <= 1.01 && m_Info.yResolutionFactor <= 1.01 &&
        m_Info.zResolutionFactor <= 1.01) {
      std::cout << "Using original image (no downsampling needed)" << std::endl;
      saveImage(m_Info.outputImagePath.c_str(), imgData, imgSizePtr, dataType);
      delete[] imgData;
      return true;
    }

    int64_t channelSize = imgSizePtr[0] * imgSizePtr[1] * imgSizePtr[2];
    auto *imgDataChX = new unsigned char[channelSize];
    // 只处理指定的通道
    for (int64_t i = 0; i < channelSize; i++) {
      imgDataChX[i] = imgData[i + (m_Info.channel - 1) * channelSize];
    }

    // 计算缩放后的大小
    int64_t sampleOutSize[4];
    sampleOutSize[0] = (int64_t)floor(imgSizePtr[0] / m_Info.xResolutionFactor);
    sampleOutSize[1] = (int64_t)floor(imgSizePtr[1] / m_Info.yResolutionFactor);
    sampleOutSize[2] = (int64_t)floor(imgSizePtr[2] / m_Info.zResolutionFactor);
    sampleOutSize[3] = 1;

    std::cout << "New size: " << sampleOutSize[0] << "x" << sampleOutSize[1]
              << "x" << sampleOutSize[2] << std::endl;
    std::cout << "New volume: "
              << (sampleOutSize[0] * sampleOutSize[1] * sampleOutSize[2])
              << std::endl;

    // 准备缩放后的图像缓冲区
    auto channelSizeResample =
        sampleOutSize[0] * sampleOutSize[1] * sampleOutSize[2];
    auto *imageDataResampled = new unsigned char[channelSizeResample];

    // 设置缩放因子
    double dfactor[3];
    dfactor[0] = m_Info.xResolutionFactor;
    dfactor[1] = m_Info.yResolutionFactor;
    dfactor[2] = m_Info.zResolutionFactor;

    // 执行下采样
    downsample3dvol(imageDataResampled, imgDataChX, sampleOutSize, imgSizePtr,
                    dfactor, m_Info.tag);

    // 保存结果
    saveImage(m_Info.outputImagePath.c_str(), imageDataResampled, sampleOutSize,
              1);

    // 清理内存
    delete[] imgData;
    delete[] imgDataChX;
    delete[] imageDataResampled;

    return true;
  }

private:
  ImageSamplingCreateInfo m_Info;
};

int main(int argc, char **argv) {
  cxxopts::ParseResult result;
  try {
    cxxopts::Options options(
        "v3draw image resampler",
        "Downsample v3draw/v3dpbd images to fit target volume");
    options.add_options()("p,path", "Directory path containing images",
                          cxxopts::value<std::string>())(
        "o,output", "Output directory (optional)",
        cxxopts::value<std::string>())(
        "c,channel", "Channel to process",
        cxxopts::value<int>()->default_value("1"))(
        "t,tag", "Downsampling method (0:averaging, 1:subsampling)",
        cxxopts::value<int>()->default_value("1"))("h,help", "Print help");
    result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      exit(0);
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  std::filesystem::path rootPath;
  std::filesystem::path outputPath;

  if (result.count("path")) {
    rootPath = result["path"].as<std::string>();
  } else {
    rootPath = R"(C:\Users\KiraY\Desktop\TestImage)";
  }

  if (result.count("output")) {
    outputPath = result["output"].as<std::string>();
  } else {
    outputPath = rootPath;
  }

  // 创建输出目录（如果不存在）
  if (!std::filesystem::exists(outputPath)) {
    std::filesystem::create_directories(outputPath);
  }

  int channel = result["channel"].as<int>();
  int tag = result["tag"].as<int>();

  int successCount = 0, failedCount = 0, skippedCount = 0;

  // 处理所有v3draw和v3dpbd文件
  for (auto &dirEntry : std::filesystem::directory_iterator(rootPath)) {
    std::string extension = dirEntry.path().extension().string();
    std::string out_extension = ".v3dpbd";

    // 检查文件扩展名是否为v3draw或v3dpbd
    if (extension != ".v3draw" && extension != ".v3dpbd") {
      std::cout << "Skipping non-image file: " << dirEntry.path() << std::endl;
      skippedCount++;
      continue;
    }

    ImageSamplingCreateInfo info;
    info.inputImagePath = dirEntry.path().string();
    info.outputImagePath = (outputPath / (dirEntry.path().stem().string() +
                                          "_downsampled" + out_extension))
                               .string();
    info.channel = channel;
    info.tag = tag;
    info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;

    ImageSampler sampler;
    sampler.setSamplingCreateInfo(info);

    std::cout << "\n--------------------------------" << std::endl;
    if (sampler.resampleImage()) {
      std::cout << "Resample image success: " << dirEntry.path().filename()
                << std::endl;
      successCount++;
    } else {
      std::cerr << "Resample image failed: " << dirEntry.path().filename()
                << std::endl;
      failedCount++;
    }
    std::cout << "--------------------------------\n" << std::endl;
  }

  std::cout << "\nSummary:" << std::endl;
  std::cout << "Processed files: " << (successCount + failedCount) << std::endl;
  std::cout << "Success count: " << successCount << std::endl;
  std::cout << "Failed count: " << failedCount << std::endl;
  std::cout << "Skipped count: " << skippedCount << std::endl;

  return (failedCount == 0) ? 0 : -1;
}