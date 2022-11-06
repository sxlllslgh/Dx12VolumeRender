#include "pch.h"
#include "VolumeRenderPage.xaml.h"
#if __has_include("VolumeRenderPage.g.cpp")
#include "VolumeRenderPage.g.cpp"
#endif

#include "App.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Dx12VolumeRender::implementation {
    VolumeRenderPage::VolumeRenderPage() {
        InitializeComponent();

        hostWindow = Application::Current().try_as<App>()->GetMainWindow();
        hostWindow.try_as<IWindowNative>()->get_WindowHandle(&hostWindowHandle);

        deviceResources = std::make_shared<MyDirectX12::DeviceResources>(hostWindow);
        deviceResources->CreateDeviceResources();
    }
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnPageLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    deviceResources->SetSwapChainPanel(displayArea(), XamlRoot().RasterizationScale());
}

Windows::Foundation::IAsyncAction winrt::Dx12VolumeRender::implementation::VolumeRenderPage::OnReadFileClick(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    Windows::Storage::Pickers::FileOpenPicker picker;
    auto initializeWithWindow{ picker.as<IInitializeWithWindow>() };
    initializeWithWindow->Initialize(hostWindowHandle);

    picker.FileTypeFilter().Append(L".uvf");
    selectedFile = co_await picker.PickSingleFileAsync();

    if (selectedFile != nullptr) {
        auto fileBuffer = co_await Windows::Storage::FileIO::ReadBufferAsync(selectedFile);
        auto fileBytes = fileBuffer.data();
        int frameCounts;
        std::memcpy(&frameCounts, fileBytes, sizeof(int));
    }
}
