#pragma once

#include "App.xaml.g.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct App : AppT<App> {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

        winrt::Microsoft::UI::Xaml::Window GetMainWindow() { return window; }

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
