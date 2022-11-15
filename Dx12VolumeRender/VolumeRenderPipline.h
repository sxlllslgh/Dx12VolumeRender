#pragma once

#include <memory>

#include "DeviceResources.h"
#include "StepTimer.hpp"
#include "VolumeData.hpp"

namespace MyDirectX12 {
    class VolumeRenderPipline {
    public:
        VolumeRenderPipline(const std::shared_ptr<DeviceResources>& deviceResources) : deviceResources(deviceResources) {}
        VolumeRenderPipline() noexcept(false) {}
        ~VolumeRenderPipline() = default;

        VolumeRenderPipline(VolumeRenderPipline&&) = default;
        VolumeRenderPipline& operator= (VolumeRenderPipline&&) = default;

        VolumeRenderPipline(VolumeRenderPipline const&) = delete;
        VolumeRenderPipline& operator= (VolumeRenderPipline const&) = delete;
        void Update(StepTimer const& timer);
        void Render(StepTimer const& timer);
        void Clear();

        void LoadVolumeData(VolumeData const& givenData) {}

    private:
        inline const FLOAT* ConvertColorToArray(winrt::Windows::UI::Color const& color);

        std::shared_ptr<DeviceResources> deviceResources;

        StepTimer timer;

        VolumeData volumeData;
    };
}