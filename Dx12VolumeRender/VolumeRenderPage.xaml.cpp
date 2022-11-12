#include "pch.h"
#include "VolumeRenderPage.xaml.h"
#if __has_include("VolumeRenderPage.g.cpp")
#include "VolumeRenderPage.g.cpp"
#endif

#include "App.xaml.h"

#include "VolumeData.hpp"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Dx12VolumeRender::implementation {
    VolumeRenderPage::VolumeRenderPage() {
        InitializeComponent();

        hostWindow = Application::Current().try_as<App>()->GetMainWindow();
        hostWindow.try_as<IWindowNative>()->get_WindowHandle(&hostWindowHandle);

        //auto factory{ winrt::get_activation_factory<winrt::Windows::Graphics::Display::DisplayInformation, IDisplayInformationStaticsInterop>() };
        //winrt::Windows::Graphics::Display::DisplayInformation displayInfo{ nullptr };

        //winrt::check_hresult(factory->GetForWindow(hostWindowHandle, winrt::guid_of<winrt::Windows::Graphics::Display::DisplayInformation>(), winrt::put_abi(displayInfo)));
        
        /*
        DispatcherQueue().TryEnqueue([=]() {
            winrt::Windows::Graphics::Display::DisplayInformation::DisplayContentsInvalidated(winrt::Windows::Foundation::TypedEventHandler<winrt::Windows::Graphics::Display::DisplayInformation, winrt::Windows::Foundation::IInspectable>(this, &VolumeRenderPage::OnDisplayContentsInvalidated));
        });*/
        
        deviceResources = std::make_shared<MyDirectX12::DeviceResources>(hostWindow);
        deviceResources->CreateDeviceResources();
    }
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    deviceResources->SetSwapChainPanel(displayArea(), XamlRoot().RasterizationScale());
    deviceResources->ValidateDevice();
    renderPipeline = std::make_unique<MyDirectX12::VolumeRenderPipline>(deviceResources);
    StartRenderLoop();
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::Update() {
    timer.Tick([&]() {
        renderPipeline->Update(timer);
    });
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::Render() {
    renderPipeline->Render(timer);
}

winrt::Windows::Foundation::IAsyncAction winrt::Dx12VolumeRender::implementation::VolumeRenderPage::StartRenderLoop() {
    if (renderLoopWorker != nullptr && renderLoopWorker.Status() == winrt::Windows::Foundation::AsyncStatus::Started) {
        co_return;
    }

    winrt::Windows::System::Threading::WorkItemHandler workItemHandler = winrt::Windows::System::Threading::WorkItemHandler([this](winrt::Windows::Foundation::IAsyncAction action) {
        while (action.Status() == winrt::Windows::Foundation::AsyncStatus::Started) {
            Concurrency::critical_section::scoped_lock lock(criticalSection);
            Update();
            Render();
        }
    });
    renderLoopWorker = winrt::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, winrt::Windows::System::Threading::WorkItemPriority::High, winrt::Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnDisplayContentsInvalidated(winrt::Windows::Graphics::Display::DisplayInformation const&, winrt::Windows::Foundation::IInspectable const&) {
    Concurrency::critical_section::scoped_lock lock(criticalSection);
    deviceResources->ValidateDevice();
}

Windows::Foundation::IAsyncAction winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnReadFileClick(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&) {
    Windows::Storage::Pickers::FileOpenPicker picker;
    auto initializeWithWindow{ picker.as<IInitializeWithWindow>() };
    initializeWithWindow->Initialize(hostWindowHandle);

    picker.FileTypeFilter().Append(L".uvf");
    selectedFile = co_await picker.PickSingleFileAsync();

    if (selectedFile != nullptr) {
        auto fileBuffer = co_await Windows::Storage::FileIO::ReadBufferAsync(selectedFile);
        auto fileBytes = fileBuffer.data();
        VolumeData volumeData;
        volumeData.readFromBytes<int>(&fileBytes, volumeData.frameCount);
        volumeData.readFromBytes<int>(&fileBytes, volumeData.channels);
        volumeData.readFromBytes<int>(&fileBytes, volumeData.frameWidth);
        volumeData.readFromBytes<int>(&fileBytes, volumeData.frameHeight);
        volumeData.readFromBytes<double>(&fileBytes, volumeData.widthPerPixel);
        volumeData.readFromBytes<double>(&fileBytes, volumeData.heightPerPixel);
        volumeData.readMatrix4dFromBytes(&fileBytes, volumeData.matItoF);
        
    }
}
