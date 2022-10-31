#include "pch.h"
#include "VolumeRenderPage.xaml.h"
#if __has_include("VolumeRenderPage.g.cpp")
#include "VolumeRenderPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Dx12VolumeRender::implementation {
    VolumeRenderPage::VolumeRenderPage() {
        InitializeComponent();
    }
}
