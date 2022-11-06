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
        auto hostWindow = Application::Current().try_as<App>()->GetMainWindow();

        deviceResources = std::make_shared<MyDirectX12::DeviceResources>(hostWindow);
        deviceResources->CreateDeviceResources();
    }
}

void winrt::Dx12VolumeRender::implementation::VolumeRenderPage::Page_Loaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    deviceResources->SetSwapChainPanel(displayArea(), XamlRoot().RasterizationScale());
}