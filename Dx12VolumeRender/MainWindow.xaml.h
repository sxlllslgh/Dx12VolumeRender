#pragma once

#include "MainWindow.g.h"

#include "NavigationCategoryViewModel.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct MainWindow : MainWindowT<MainWindow> {
    private:
        HWND nativeWindow{ nullptr };

        // System backdrop related vairables to apply Mica or Acrylic effects.
        Windows::System::DispatcherQueueController dispatcherQueueController{ nullptr };
        Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration systemBackdropConfiguration{ nullptr };
        Microsoft::UI::Xaml::Window::Activated_revoker activatedRevoker;
        Microsoft::UI::Xaml::FrameworkElement rootElement{ nullptr };
        Microsoft::UI::Xaml::FrameworkElement::ActualThemeChanged_revoker themeChangedRevoker;
        Microsoft::UI::Composition::SystemBackdrops::MicaController micaController{ nullptr };

        // ViewModel for the Navigation view
        Dx12VolumeRender::NavigationCategoryViewModel mainWindowNavigationCategoryViewModel;

        winrt::hstring activeItem;

        // System backdrop related functions to apply Mica or Acrylic effects.
        void EnsureDispatcherQueue();
        void SetupSystemBackdropConfiguration();
        Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme ConvertToSystemBackdropTheme(Microsoft::UI::Xaml::ElementTheme const& theme);
    public:
        MainWindow();
        void OnWindowClosed(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::WindowEventArgs const&);

        Dx12VolumeRender::NavigationCategoryViewModel MainWindowNavigationCategoryViewModel();
        void OnMainNavigationLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void OnMainNavigationItemInvoked(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args);
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow> {};
}
