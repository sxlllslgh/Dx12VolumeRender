#pragma once

#include "VolumeRenderPage.g.h"

#include "DeviceResources.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage> {
    private:
        std::shared_ptr<MyDirectX12::DeviceResources> deviceResources;

    public:
        VolumeRenderPage();
        void Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage, implementation::VolumeRenderPage> {
    };
}
