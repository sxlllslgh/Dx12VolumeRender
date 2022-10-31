#include "pch.h"
#include "NavigationCategoryViewModel.h"
#if __has_include("NavigationCategoryViewModel.g.cpp")
#include "NavigationCategoryViewModel.g.cpp"
#endif

namespace winrt::Dx12VolumeRender::implementation {
    NavigationCategoryViewModel::NavigationCategoryViewModel() {
        navigationCategories = winrt::single_threaded_observable_vector<Dx12VolumeRender::NavigationCategory>();
    }

    Windows::Foundation::Collections::IObservableVector<Dx12VolumeRender::NavigationCategory> NavigationCategoryViewModel::NavigationCategories() {
        return navigationCategories;
    }
}