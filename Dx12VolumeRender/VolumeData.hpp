#pragma once
#include <pch.h>
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
    UltrasoundFrame(winrt::Windows::Storage::Streams::DataReader const& dataReader, const int& width, const int& height, const int& channels, double widthPerPixel, double heightPerPixel) : width(width), height(height), channels(channels), widthPerPixel(widthPerPixel), heightPerPixel(heightPerPixel) {
        byte* originalImageData = new byte[width * height * channels];
        dataReader.ReadBytes(originalImageData);
        delete[] originalImageData;
        new (&originalImage)Eigen::Map<Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::Dynamic>>(originalImageData);
        //Eigen::Map<Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic, Eigen::Dynamic>>originalImage(&originalImageData, width * height * channels);
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

    //template<typename T>
    //void readFromBytes(Windows::Storage::Streams::DataReader const& dataReader, T& dataItem) {
    //    if ()
    //    dataReader.ReadBytes();
    //    std::memcpy(&dataItem, *fileBytePointer, sizeof(T));
    //    *fileBytePointer += sizeof(T);
    //}

    void readMatrix4dFromBytes(winrt::Windows::Storage::Streams::DataReader const& dataReader, Eigen::Matrix4d& mat4d) {
        double matData[16];
        for (auto i = 0; i < 4; ++i) {
            for (auto j = 0; j < 4; ++j) {
                matData[i, j] = dataReader.ReadDouble();
            }
        }
        new (&mat4d)Eigen::Map<Eigen::Matrix4d>(matData);
    }

    static VolumeData loadFileFromReader(winrt::Windows::Storage::Streams::DataReader const& dataReader) {
        VolumeData volumeData;
        volumeData.frameCount = dataReader.ReadInt32();
        volumeData.channels = dataReader.ReadInt32();
        volumeData.frameWidth = dataReader.ReadInt32();
        volumeData.frameHeight = dataReader.ReadInt32();
        volumeData.widthPerPixel = dataReader.ReadInt32();
        volumeData.heightPerPixel = dataReader.ReadInt32();
        volumeData.readMatrix4dFromBytes(dataReader, volumeData.matItoF);
        auto frameBufferSize = volumeData.frameWidth * volumeData.frameHeight * volumeData.channels;
        for (auto i = 0; i < volumeData.frameCount; ++i) {
            volumeData.frames.push_back(UltrasoundFrame<double>(dataReader, volumeData.frameWidth, volumeData.frameHeight, volumeData.channels, volumeData.widthPerPixel, volumeData.heightPerPixel));
        }
        return volumeData;
    }
};