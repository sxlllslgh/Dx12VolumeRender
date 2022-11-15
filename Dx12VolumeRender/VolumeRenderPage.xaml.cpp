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
        deviceResources->RegisterDeviceNotify(this);
        deviceResources->CreateDeviceResources();

        timer.SetFixedTimeStep(true);
        timer.SetTargetElapsedSeconds(1.0 / 60);
    }

    VolumeRenderPage::~VolumeRenderPage() {
        renderLoopWorker.Cancel();
    }
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    deviceResources->SetSwapChainPanel(renderArea(), XamlRoot().RasterizationScale());
    deviceResources->ValidateDevice();
    renderPipeline = std::make_unique<MyDirectX12::VolumeRenderPipline>(deviceResources);
    StartRenderLoop();
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::StartRenderLoop() {
    if (renderLoopWorker != nullptr && renderLoopWorker.Status() == winrt::Windows::Foundation::AsyncStatus::Started) {
        return;
    }

    winrt::Windows::System::Threading::WorkItemHandler workItemHandler = winrt::Windows::System::Threading::WorkItemHandler([this](winrt::Windows::Foundation::IAsyncAction action) {
        while (action.Status() == winrt::Windows::Foundation::AsyncStatus::Started) {
            Concurrency::critical_section::scoped_lock lock(criticalSection);
            timer.Tick([&]() {
                renderPipeline->Update(timer);
            });
            renderPipeline->Render(timer);
        }
    });
    renderLoopWorker = winrt::Windows::System::Threading::ThreadPool::RunAsync(workItemHandler, winrt::Windows::System::Threading::WorkItemPriority::High, winrt::Windows::System::Threading::WorkItemOptions::TimeSliced);
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnDeviceLost() {}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnDeviceRestored() {}

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
        auto fileStream = co_await selectedFile.OpenSequentialReadAsync();
        /*fileStream.
        auto fileBuffer = co_await Windows::Storage::FileIO::ReadBufferAsync(selectedFile);*/
        Windows::Storage::Streams::DataReader dataReader(fileStream);
        //dataReader.FromBuffer(fileBuffer);
        //HRESULT hr = unknown->QueryInterface(_uuidof(winrt::Windows::Storage::Streams::IBufferByteAccess), &bufferByteAccess);*/
        //byte* fileBytes = nullptr;
        //bufferByteAccess->Buffer(&fileBytes);
        auto volumeData = VolumeData::loadFileFromReader(dataReader);
        renderPipeline->LoadVolumeData(volumeData);
    }
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnRenderAreaSizeChanged(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::SizeChangedEventArgs const& e) {
    if (IsLoaded() && e.NewSize() != deviceResources->GetOutputSize()) {
        Concurrency::critical_section::scoped_lock lock(criticalSection);
        //deviceResources->SetSwapChainPanel(renderArea(), XamlRoot().RasterizationScale());
        //renderLoopWorker.Cancel();
        //renderLoopWorker.Completed([=, strongThis = get_strong()](Windows::Foundation::IAsyncAction const& asyncInfo, Windows::Foundation::AsyncStatus const& asyncStatus) {
        //    if (asyncStatus == Windows::Foundation::AsyncStatus::Canceled) {
        //        return;
        //    }
        //    renderPipeline = std::make_unique<MyDirectX12::VolumeRenderPipline>(deviceResources);
        //    StartRenderLoop();
        //});
        //StartRenderLoop();
        deviceResources->ChangeSwapChainSize(e.NewSize());
        timer.ResetElapsedTime();
        //deviceResources->CreateWindowSizeDependentResources();
    }
}