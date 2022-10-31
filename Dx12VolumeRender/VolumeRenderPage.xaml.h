#pragma once

#include "VolumeRenderPage.g.h"

namespace winrt::Dx12VolumeRender::implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage> {
        VolumeRenderPage();
    };
}

namespace winrt::Dx12VolumeRender::factory_implementation {
    struct VolumeRenderPage : VolumeRenderPageT<VolumeRenderPage, implementation::VolumeRenderPage> {
    };
}
