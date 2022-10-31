#include "pch.h"
#include "NavigationCategory.h"
#if __has_include("NavigationCategory.g.cpp")
#include "NavigationCategory.g.cpp"
#endif

namespace winrt::Dx12VolumeRender::implementation {
    NavigationCategory::NavigationCategory(hstring const& name, hstring const& tooltip, hstring const& icon) : name(name), tooltip(tooltip), icon(icon) {}
    hstring NavigationCategory::Name() {
        return hstring{ name };
    }
    hstring NavigationCategory::Tooltip() {
        return hstring{ tooltip };
    }
    hstring NavigationCategory::Icon() {
        return hstring{ icon };
    }
}