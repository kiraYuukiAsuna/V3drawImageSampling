#include <array>
#include <iostream>
#include <filesystem>

#include "FL_downSample3D.h"
#include "FL_upSample3D.h"
#include "stackutil.h"
#include "cxxopts.hpp"

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

    enum class SampleMode {
        Unknown,
        UpSample,
        DownSample
    };

    SampleMode mode;
};

class ImageSampler {
public:
    ImageSampler() = default;

    void setSamplingCreateInfo(const ImageSamplingCreateInfo&info) {
        m_Info = info;
    }

    bool resampleImage() {
        if (!std::filesystem::exists(m_Info.inputImagePath)) {
            std::cerr << "Input image does not exist" << std::endl;
            return false;
        }

        std::cout << "x_factor = " << m_Info.xResolutionFactor << std::endl;
        std::cout << "y_factor = " << m_Info.yResolutionFactor << std::endl;
        std::cout << "z_factor = " << m_Info.zResolutionFactor << std::endl;
        std::cout << "ch = " << m_Info.channel << std::endl;
        std::cout << "inimg_file = " << m_Info.inputImagePath << std::endl;
        std::cout << "outimg_file = " << m_Info.outputImagePath << std::endl;

        unsigned char* imgData = nullptr;
        int64_t* imgSizePtr = nullptr;
        int dataType;
        if (!loadImage((char *)m_Info.inputImagePath.c_str(), imgData, imgSizePtr, dataType)) {
            std::cerr << "Load image " << m_Info.inputImagePath << " error!" << std::endl;
            return false;
        }

        if (m_Info.size[0] != 0 && m_Info.size[1] != 0 && m_Info.size[2] != 0) {
            if (imgSizePtr[0] >= m_Info.size[0] && imgSizePtr[1] >= m_Info.size[1] && imgSizePtr[2] >= m_Info.size[2]) {
                m_Info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;
                m_Info.xResolutionFactor = (double)imgSizePtr[0] / m_Info.size[0];
                m_Info.yResolutionFactor = (double)imgSizePtr[1] / m_Info.size[1];
                m_Info.zResolutionFactor = (double)imgSizePtr[2] / m_Info.size[2];
            }
            else if (imgSizePtr[0] <= m_Info.size[0] && imgSizePtr[1] <= m_Info.size[1] && imgSizePtr[2] <= m_Info.size[
                         2]) {
                m_Info.mode = ImageSamplingCreateInfo::SampleMode::UpSample;
                m_Info.xResolutionFactor = (double)m_Info.size[0] / imgSizePtr[0];
                m_Info.yResolutionFactor = (double)m_Info.size[1] / imgSizePtr[1];
                m_Info.zResolutionFactor = (double)m_Info.size[2] / imgSizePtr[2];
            }
            else {
                std::cerr << "Invalid size! Cannot determine downsampling or upsampling mode!" << std::endl;
                return false;
            }
        }

        if (m_Info.xResolutionFactor < 1 || m_Info.yResolutionFactor < 1 || m_Info.zResolutionFactor < 1) {
            std::cerr << "Sample factor cannot be smaller than 1" << std::endl;
            return false;
        }

        int64_t channelSize = imgSizePtr[0] * imgSizePtr[1] * imgSizePtr[2];
        auto* imgDataChX = new unsigned char [channelSize];
        for (int64_t i = 0; i < channelSize; i++) {
            imgDataChX[i] = imgData[i + (m_Info.channel - 1) * channelSize];
        }
        imgSizePtr[3] = 1;

        auto channelSizeResample = static_cast<int64_t>(ceil(
            m_Info.xResolutionFactor * m_Info.yResolutionFactor * m_Info.zResolutionFactor * channelSize));
        auto* imageDataResampled = new unsigned char [channelSizeResample];

        int64_t sampleOutSize[4];
        double dfactor[3];
        dfactor[0] = m_Info.xResolutionFactor;
        dfactor[1] = m_Info.yResolutionFactor;
        dfactor[2] = m_Info.zResolutionFactor;
        if (m_Info.mode == ImageSamplingCreateInfo::SampleMode::UpSample) {
            sampleOutSize[0] = (int64_t)ceil(imgSizePtr[0] * m_Info.xResolutionFactor);
            sampleOutSize[1] = (int64_t)ceil(imgSizePtr[1] * m_Info.yResolutionFactor);
            sampleOutSize[2] = (int64_t)ceil(imgSizePtr[2] * m_Info.zResolutionFactor);
            sampleOutSize[3] = 1;
            upsample3dvol(imageDataResampled, imgData, sampleOutSize, imgSizePtr, dfactor);
        }
        else if (m_Info.mode == ImageSamplingCreateInfo::SampleMode::DownSample) {
            sampleOutSize[0] = (int64_t)floor(imgSizePtr[0] / m_Info.xResolutionFactor);
            sampleOutSize[1] = (int64_t)floor(imgSizePtr[1] / m_Info.yResolutionFactor);
            sampleOutSize[2] = (int64_t)floor(imgSizePtr[2] / m_Info.zResolutionFactor);
            sampleOutSize[3] = 1;
            downsample3dvol(imageDataResampled, imgData, sampleOutSize, imgSizePtr, dfactor, m_Info.tag);
        }
        saveImage(m_Info.outputImagePath.c_str(), imageDataResampled, sampleOutSize, 1);
        if (imgData) {
            delete []imgData;
            imgData = nullptr;
        }
        if (imgDataChX) {
            delete []imgDataChX;
            imgDataChX = nullptr;
        }
        if (imageDataResampled) {
            delete []imageDataResampled;
            imageDataResampled = nullptr;
        }

        return true;
    }

private:
    ImageSamplingCreateInfo m_Info;
};


int smain(int argc, char** argv) {
    cxxopts::ParseResult result;
    try {
        cxxopts::Options options("v3draw image resampler", "Upsample or Downsample v3draw image");
        options.add_options()
                ("i,inputImagePath", "inputImagePath", cxxopts::value<std::string>()) // a bool parameter
                ("o,outputImagePath", "outputImagePath", cxxopts::value<std::string>())
                ("s,size", "size x y z", cxxopts::value<int>())
                ("x,xResolutionFactor", "xResolutionFactor", cxxopts::value<double>())
                ("y,yResolutionFactor", "yResolutionFactor", cxxopts::value<double>())
                ("z,zResolutionFactor", "zResolutionFactor", cxxopts::value<double>())
                ("c,channel", "channel", cxxopts::value<int>()->default_value("1"))
                ("m,mode", "mode", cxxopts::value<std::string>())
                ("t,tag", "tag", cxxopts::value<std::string>()->default_value("1"));
        result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            exit(0);
        }
    }
    catch (std::exception&e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    ImageSamplingCreateInfo info;

    if (result.count("inputImagePath")) {
        info.inputImagePath = result["inputImagePath"].as<std::string>();
    }
    if (result.count("outputImagePath")) {
        info.outputImagePath = result["outputImagePath"].as<std::string>();
    }

    if (result.count("size")) {
        info.size[0] = result["size"].as<int>();
        info.size[1] = result["size"].as<int>();
        info.size[2] = result["size"].as<int>();
    }
    else {
        if (result.count("xResolutionFactor")) {
            info.xResolutionFactor = result["xResolutionFactor"].as<double>();
        }
        if (result.count("yResolutionFactor")) {
            info.yResolutionFactor = result["yResolutionFactor"].as<double>();
        }
        if (result.count("zResolutionFactor")) {
            info.zResolutionFactor = result["zResolutionFactor"].as<double>();
        }
        if (result.count("mode")) {
            if (result["mode"].as<std::string>() == "up") {
                info.mode = ImageSamplingCreateInfo::SampleMode::UpSample;
            }
            else if (result["mode"].as<std::string>() == "down") {
                info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;
            }
            else {
                std::cerr << "Unknown sample mode!" << std::endl;
                return -1;
            }
        }
    }

    if (result.count("channel")) {
        info.channel = result["channel"].as<int>();
    }
    if (result.count("tag")) {
        info.tag = result["tag"].as<std::string>().at(0);
    }

    std::filesystem::path rootPath("/media/seele/bb91b197-9dda-4678-959b-b4f8e0dcd527/RatingImage");

    int successCount = 0, failedCount = 0;
    for (auto&dirEntry: std::filesystem::directory_iterator(rootPath)) {
        std::cout << dirEntry.path() << std::endl;

        info.inputImagePath = dirEntry.path().string();
        info.outputImagePath =
                dirEntry.path().parent_path().string() + "/" + dirEntry.path().stem().string() + "_resampled.v3draw";
        info.size[0] = 256;
        info.size[1] = 256;
        info.size[2] = 32;
        info.xResolutionFactor = 2.0;
        info.yResolutionFactor = 2.0;
        info.zResolutionFactor = 2.0;
        info.channel = 1;
        info.tag = 1;
        info.mode = ImageSamplingCreateInfo::SampleMode::DownSample;

        ImageSampler sampler;
        sampler.setSamplingCreateInfo(info);
        if (sampler.resampleImage()) {
            std::cout << "Resample image success!" << std::endl;
            successCount++;
        }
        else {
            std::cerr << "Resample image failed!" << std::endl;
            failedCount++;
        }
    }

    std::cout << "Success count: " << successCount << std::endl;
    std::cout << "Failed count: " << failedCount << std::endl;
}
