#pragma once

#include "NavigationCategoryViewModel.g.h"

#include "NavigationCategory.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct NavigationCategoryViewModel : NavigationCategoryViewModelT<NavigationCategoryViewModel> {
    private:
        Windows::Foundation::Collections::IObservableVector<Dx12VolumeRender::NavigationCategory> navigationCategories;
    public:
        NavigationCategoryViewModel();
        Windows::Foundation::Collections::IObservableVector<Dx12VolumeRender::NavigationCategory> NavigationCategories();
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct NavigationCategoryViewModel : NavigationCategoryViewModelT<NavigationCategoryViewModel, implementation::NavigationCategoryViewModel> {};
}