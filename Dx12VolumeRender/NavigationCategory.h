#pragma once

#include "NavigationCategory.g.h"

#include <string>

namespace winrt::Dx12VolumeRender::implementation {
    struct NavigationCategory : NavigationCategoryT<NavigationCategory> {
    private:
        std::wstring name;
        std::wstring tooltip;
        std::wstring icon;
    public:
        NavigationCategory(hstring const& name, hstring const& tooltip, hstring const& icon);
        hstring Name();
        hstring Tooltip();
        hstring Icon();
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct NavigationCategory : NavigationCategoryT<NavigationCategory, implementation::NavigationCategory> {};
}