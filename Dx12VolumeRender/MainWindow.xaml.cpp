#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Dx12VolumeRender::implementation {
    MainWindow::MainWindow() {
        InitializeComponent();

        Title(L"Volume Render in DirectX 12");

        try_as<::IWindowNative>()->get_WindowHandle(&nativeWindow);

        if (Microsoft::UI::Composition::SystemBackdrops::MicaController::IsSupported()) {
            EnsureDispatcherQueue();
            SetupSystemBackdropConfiguration();
            micaController = Microsoft::UI::Composition::SystemBackdrops::MicaController();
            micaController.SetSystemBackdropConfiguration(systemBackdropConfiguration);
            micaController.AddSystemBackdropTarget(try_as<Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop>());
        }

        mainWindowNavigationCategoryViewModel.NavigationCategories().Append(make<Dx12VolumeRender::implementation::NavigationCategory>(hstring{ L"Volume Render" }, hstring{ L"体渲染" }, hstring{ L"World" }));
    }

    Dx12VolumeRender::NavigationCategoryViewModel MainWindow::MainWindowNavigationCategoryViewModel() {
        return mainWindowNavigationCategoryViewModel;
    }
}

void winrt::Dx12VolumeRender::implementation::MainWindow::EnsureDispatcherQueue() {
    if (dispatcherQueueController == nullptr) {
        DispatcherQueueOptions options{ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE };
        Windows::System::DispatcherQueueController controller{ nullptr };
        check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(put_abi(controller))));
        dispatcherQueueController = controller;
    }
}

void winrt::Dx12VolumeRender::implementation::MainWindow::SetupSystemBackdropConfiguration() {
    systemBackdropConfiguration = Microsoft::UI::Composition::SystemBackdrops::SystemBackdropConfiguration();
    activatedRevoker = this->Activated(auto_revoke,
        [&](auto&&, Microsoft::UI::Xaml::WindowActivatedEventArgs const& args) {
            systemBackdropConfiguration.IsInputActive(
                Microsoft::UI::Xaml::WindowActivationState::Deactivated != args.WindowActivationState());
        });
    systemBackdropConfiguration.IsInputActive(true);
    rootElement = this->Content().try_as<Microsoft::UI::Xaml::FrameworkElement>();
    if (rootElement != nullptr) {
        themeChangedRevoker = rootElement.ActualThemeChanged(auto_revoke,
            [&](auto&&, auto&&) {
                systemBackdropConfiguration.Theme(ConvertToSystemBackdropTheme(rootElement.ActualTheme()));
            });

        systemBackdropConfiguration.Theme(ConvertToSystemBackdropTheme(rootElement.ActualTheme()));
    }
}

Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme winrt::Dx12VolumeRender::implementation::MainWindow::ConvertToSystemBackdropTheme(Microsoft::UI::Xaml::ElementTheme const& theme) {
    switch (theme) {
    case Microsoft::UI::Xaml::ElementTheme::Dark:
        return Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Dark;
    case Microsoft::UI::Xaml::ElementTheme::Light:
        return Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Light;
    default:
        return Microsoft::UI::Composition::SystemBackdrops::SystemBackdropTheme::Default;
    }
}

void winrt::Dx12VolumeRender::implementation::MainWindow::OnMainNavigationLoaded(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e) {
    //mainNavigation().SelectedItem(mainNavigation().MenuItems().First());
    //mainNavigation().SelectedItem(mainNavigation().MenuItemsSource().as<Windows::Foundation::Collections::IObservableVector<Dx12VolumeRender::NavigationCategory>>().First());
}

void winrt::Dx12VolumeRender::implementation::MainWindow::OnMainNavigationItemInvoked(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args) {
    activeItem = args.InvokedItem().as<hstring>();
    if (activeItem == L"Volume Render") {
        contentFrame().Navigate(xaml_typename<VolumeRenderPage>());
    }
}

void winrt::Dx12VolumeRender::implementation::MainWindow::OnWindowClosed(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::WindowEventArgs const&) {
    micaController = nullptr;
}