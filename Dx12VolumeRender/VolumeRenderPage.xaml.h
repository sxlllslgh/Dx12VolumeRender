#pragma once

#include "VolumeRenderPage.g.h"

#include "DeviceResources.h"
#include "VolumeRenderPipline.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage>, MyDirectX12::IDeviceNotify {
    private:
        winrt::Microsoft::UI::Xaml::Window hostWindow{ nullptr };
        HWND hostWindowHandle{ nullptr };

        Windows::Storage::StorageFile selectedFile{ nullptr };

        std::shared_ptr<MyDirectX12::DeviceResources> deviceResources;
        std::unique_ptr<MyDirectX12::VolumeRenderPipline> renderPipeline;
        Windows::Foundation::IAsyncAction renderLoopWorker;
        MyDirectX12::StepTimer timer;
        Concurrency::critical_section criticalSection;
        
        void StartRenderLoop();

        void OnDisplayContentsInvalidated(winrt::Windows::Graphics::Display::DisplayInformation const&, winrt::Windows::Foundation::IInspectable const&);

        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

    public:
        VolumeRenderPage();
        ~VolumeRenderPage();
        void OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        Windows::Foundation::IAsyncAction OnReadFileClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnRenderAreaSizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e);
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage, implementation::VolumeRenderPage> {};
}