#pragma once
#include <pch.h>

#include "cuda_runtime.h"
#include "cublas_v2.h"

template<typename imageDataType, typename coordinateDataType>
class UltrasoundFrame {
private:
    int width;
    int height;
    int channels;
    double widthPerPixel;
    double heightPerPixel;
    imageDataType* deviceImageArray;
    coordinateDataType* devicePixelCoordinates;

public:
    double* matFtoB;

    UltrasoundFrame(winrt::Windows::Storage::Streams::DataReader const& dataReader, const int& width, const int& height, const int& channels, double widthPerPixel, double heightPerPixel) : width(width), height(height), channels(channels), widthPerPixel(widthPerPixel), heightPerPixel(heightPerPixel) {
        winrt::array_view<byte> originalImageData;
        dataReader.ReadBytes(originalImageData);
        auto hostImageArray = new imageDataType[width * height * channels];
        for (auto i = 0; i < originalImageData.size(); ++i) {
            hostImageArray[i] = static_cast<imageDataType>(originalImageData[i] * 255);
        }

        cudaMalloc((void**)&deviceImageArray, width * height * channels * sizeof(imageDataType));
        cublasSetVector(width * height * channels, sizeof(imageDataType), hostImageArray, 1, deviceImageArray, 1);
        cudaDeviceSynchronize();
        delete[] hostImageArray;
    }

    ~UltrasoundFrame() {
        if (deviceImageArray != nullptr) {
            cudaFree(deviceImageArray);
        }
        if (devicePixelCoordinates != nullptr) {
            cudaFree(devicePixelCoordinates);
        }
        if (matFtoB != nullptr) {
            cudaFree(matFtoB);
        }
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
    double* matItoF;
    std::vector<UltrasoundFrame<double, double>> frames;
    cublasHandle_t handle;

    VolumeData() {
        auto status = cublasCreate(&handle);
        if (status != CUBLAS_STATUS_SUCCESS) {
            if (status == CUBLAS_STATUS_NOT_INITIALIZED) {
                OutputDebugStringW(L"Initial cuBLAS object failed.\n");
            }
        }
    }

    ~VolumeData() {
        cublasDestroy(handle);
    }

    void readMatrix4dFromBytes(winrt::Windows::Storage::Streams::DataReader const& dataReader, double* mat4x4) {
        double hostMatrix[16];
        for (auto i = 0; i < 16; ++i) {
            hostMatrix[i] = dataReader.ReadDouble();
        }
        cudaMalloc((void**)&mat4x4, 16 * sizeof(double));
        cublasSetVector(16, sizeof(double), hostMatrix, 1, mat4x4, 1);
        cudaDeviceSynchronize();
    }

    static concurrency::task<VolumeData> loadFileFromReader(winrt::Windows::Storage::Streams::DataReader const& dataReader) {
        return concurrency::create_task([&] {
            VolumeData volumeData;
            co_await dataReader.LoadAsync(32);
            volumeData.frameCount = dataReader.ReadInt32();
            volumeData.channels = dataReader.ReadInt32();
            volumeData.frameWidth = dataReader.ReadInt32();
            volumeData.frameHeight = dataReader.ReadInt32();
            volumeData.widthPerPixel = dataReader.ReadDouble();
            volumeData.heightPerPixel = dataReader.ReadDouble();
            co_await dataReader.LoadAsync(volumeData.frameCount * volumeData.frameWidth * volumeData.frameHeight * sizeof(double) * (volumeData.channels + 3));
            volumeData.readMatrix4dFromBytes(dataReader, volumeData.matItoF);
            auto frameBufferSize = volumeData.frameWidth * volumeData.frameHeight * volumeData.channels;
            for (auto i = 0; i < volumeData.frameCount; ++i) {
                volumeData.frames.push_back(UltrasoundFrame<double, double>(dataReader, volumeData.frameWidth, volumeData.frameHeight, volumeData.channels, volumeData.widthPerPixel, volumeData.heightPerPixel));
            }
            for (auto frame : volumeData.frames) {
                volumeData.readMatrix4dFromBytes(dataReader, frame.matFtoB);
            }
            return volumeData;
        });
    }
};