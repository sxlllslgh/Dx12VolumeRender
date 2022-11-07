#pragma once

#include <Eigen/Core>

template<typename dataType>
class UltrasoundFrame {
private:
    int width;
    int height;
    int channels;
    double widthPerPixel;
    double heightPerPixel;
    Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::Dynamic> originalImage;
    Eigen::Matrix<dataType, Eigen::Dynamic, Eigen::Dynamic, Eigen::Dynamic> image;
    Eigen::Matrix<dataType, Eigen::Dynamic, Eigen::Dynamic, 3> pixelCoordinates;
public:
    UltrasoundFrame(unsigned char const& originalImageData, int width, int height, int channels, double widthPerPixel, double heightPerPixel) : width(width), height(height), channels(channels), widthPerPixel(widthPerPixel), heightPerPixel(heightPerPixel) {
        Eigen::Map<Eigen::Matrix<double, width, height, channels>>originalImage(originalImageData);
        image = originalImage.cast<dataType>();
    }

    void loadCoordinates(dataType const& coordinatesData) {
        Eigen::Map<Eigen::Matrix<double, width, height, 3>>pixelCoordinates(coordinatesData);
    }
};

class VolumeData {
public:
    int frameCount;
    int channels;
    int frameWidth;
    int frameHeight;
    double widthPerPixel;
    double heightPerPixel;
    Eigen::Matrix4d matItoF;
    std::vector<UltrasoundFrame<double>> frames;

    template<typename T>
    void readFromBytes(uint8_t** fileBytePointer, T& dataItem) {
        std::memcpy(&dataItem, *fileBytePointer, sizeof(T));
        *fileBytePointer += sizeof(int);
    }

    void readMatrix4dFromBytes(uint8_t** fileBytePointer, Eigen::Matrix4d& mat4d) {
        double matData[16];
        std::memcpy(matData, *fileBytePointer, sizeof(double) * 16);
        new (&mat4d)Eigen::Map<Eigen::Matrix4d>(matData);
        *fileBytePointer += sizeof(double) * 16;
    }
};