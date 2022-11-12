#pragma once

#include "VolumeRenderPage.g.h"

#include "DeviceResources.h"
#include "VolumeRenderPipline.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage> {
    private:
        winrt::Microsoft::UI::Xaml::Window hostWindow{ nullptr };
        HWND hostWindowHandle{ nullptr };

        Windows::Storage::StorageFile selectedFile{ nullptr };

        std::shared_ptr<MyDirectX12::DeviceResources> deviceResources;
        std::unique_ptr<MyDirectX12::VolumeRenderPipline> renderPipeline;
        Windows::Foundation::IAsyncAction renderLoopWorker;
        MyDirectX12::StepTimer timer;
        Concurrency::critical_section criticalSection;

        void Update();
        void Render();
        winrt::Windows::Foundation::IAsyncAction StartRenderLoop();

        void OnDisplayContentsInvalidated(winrt::Windows::Graphics::Display::DisplayInformation const&, winrt::Windows::Foundation::IInspectable const&);

    public:
        VolumeRenderPage();
        ~VolumeRenderPage() { renderLoopWorker.Cancel(); }
        void OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        Windows::Foundation::IAsyncAction OnReadFileClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage, implementation::VolumeRenderPage> {
    };
}
