#pragma once

#include "VolumeRenderPage.g.h"

#include "DeviceResources.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage> {
    private:
        std::shared_ptr<MyDirectX12::DeviceResources> deviceResources;
        winrt::Microsoft::UI::Xaml::Window hostWindow{ nullptr };
        HWND hostWindowHandle{ nullptr };

        Windows::Storage::StorageFile selectedFile{ nullptr };

    public:
        VolumeRenderPage();
        void OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        Windows::Foundation::IAsyncAction OnReadFileClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage, implementation::VolumeRenderPage> {
    };
}
