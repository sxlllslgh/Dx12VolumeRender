#include "pch.h"
#include "DeviceResources.h"

namespace DisplayMetrics {
    //高分辨率显示可能需要大量 GPU 和电量才能呈现。
    // 例如，出现以下情况时，高分辨率电话可能会缩短电池使用时间
    // 游戏尝试以全保真度按 60 帧/秒的速度呈现。
    // 跨所有平台和外形规格以全保真度呈现的决定
    // 应当审慎考虑。
    static const bool SupportHighResolutions = false;

    // 用于定义“高分辨率”显示的默认阈值，如果该阈值
    // 超出范围，并且 SupportHighResolutions 为 false，将缩放尺寸
    // 50%。
    static const float DpiThreshold = 192.0f;		// 200% 标准桌面显示。
    static const float WidthThreshold = 1920.0f;	// 1080p 宽。
    static const float HeightThreshold = 1080.0f;	// 1080p 高。
};

// Constants used to calculate screen rotations
namespace ScreenRotation {
    // 0-degree Z-rotation
    static const DirectX::XMFLOAT4X4 Rotation0(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // 90-degree Z-rotation
    static const DirectX::XMFLOAT4X4 Rotation90(
        0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // 180-degree Z-rotation
    static const DirectX::XMFLOAT4X4 Rotation180(
        -1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    // 270-degree Z-rotation
    static const DirectX::XMFLOAT4X4 Rotation270(
        0.0f, -1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

namespace {
    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept {
        switch (fmt) {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:                                return fmt;
        }
    }

    inline long ComputeIntersectionArea(
        long ax1, long ay1, long ax2, long ay2,
        long bx1, long by1, long bx2, long by2) noexcept {
        return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
    }
}

MyDirectX12::DeviceResources::DeviceResources(winrt::Microsoft::UI::Xaml::Window const& window, DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, UINT backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, unsigned int flags) noexcept(false) : backBufferIndex(0), fenceValues{}, rtvDescriptorSize(0), screenViewport{}, scissorRect{}, backBufferFormat(backBufferFormat), depthBufferFormat(depthBufferFormat), backBufferCount(backBufferCount), d3dMinFeatureLevel(minFeatureLevel), window(window), d3dFeatureLevel(D3D_FEATURE_LEVEL_11_0), rotation(DXGI_MODE_ROTATION_IDENTITY), dxgiFactoryFlags(0), outputSize{ 1, 1 }, orientationTransform3D(ScreenRotation::Rotation0), colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709), options(flags), deviceNotify(nullptr) {
    if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT) {
        throw std::out_of_range("invalid backBufferCount");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0) {
        throw std::out_of_range("minFeatureLevel too low");
    }
}

MyDirectX12::DeviceResources::~DeviceResources() {
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

void MyDirectX12::DeviceResources::CreateDeviceResources() {
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        winrt::com_ptr<ID3D12Debug> debugController;
        auto enableDebugHR = D3D12GetDebugInterface(__uuidof(debugController), debugController.put_void());
        if (enableDebugHR == S_OK) {
            debugController->EnableDebugLayer();
        } else {
            std::string errorInfo;
            errorInfo = "WARNING: Direct3D Debug Device is not available, error code: " + std::to_string(enableDebugHR) + "\n";
            OutputDebugStringA(errorInfo.c_str());
        }

        winrt::com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, __uuidof(dxgiInfoQueue), dxgiInfoQueue.put_void()))) {
            dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] = { 80 }; // IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides.
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }
#endif

    SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(dxgiFactory), dxgiFactory.put_void()));

    // Determines whether tearing support is available for fullscreen borderless windows.
    if ((options & allowTearing).all()) {
        BOOL allowTearing = FALSE;

        winrt::com_ptr<IDXGIFactory5> factory5;
        dxgiFactory = factory5.try_as<IDXGIFactory4>();
        auto hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

        if (FAILED(hr) || !allowTearing) {
            options &= ~allowTearing;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
    }

    winrt::com_ptr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.put());

    // Create the DX12 API device object.
    SUCCEEDED(D3D12CreateDevice(adapter.get(), d3dMinFeatureLevel, __uuidof(d3dDevice), d3dDevice.put_void()));

    d3dDevice->SetName(L"DeviceResources");

#ifndef NDEBUG
    // Configure debug device (if active).
    winrt::com_ptr<ID3D12InfoQueue> d3dInfoQueue;
    d3dInfoQueue = d3dDevice.try_as<ID3D12InfoQueue>();
    if (d3dInfoQueue != nullptr) {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
#endif
    }

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] = {
#if defined(NTDDI_WIN10_FE) && (NTDDI_VERSION >= NTDDI_WIN10_FE)
        D3D_FEATURE_LEVEL_12_2,
#endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels = {
        static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_12_1
    };

    HRESULT hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (hr == S_OK) {
        d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    } else {
        d3dFeatureLevel = d3dMinFeatureLevel;
    }

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    SUCCEEDED(d3dDevice->CreateCommandQueue(&queueDesc, __uuidof(commandQueue), commandQueue.put_void()));

    commandQueue->SetName(L"DeviceResources");

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = backBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    SUCCEEDED(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, __uuidof(rtvDescriptorHeap), rtvDescriptorHeap.put_void()));

    rtvDescriptorHeap->SetName(L"DeviceResources");

    rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        SUCCEEDED(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, __uuidof(dsvDescriptorHeap), dsvDescriptorHeap.put_void()));

        dsvDescriptorHeap->SetName(L"DeviceResources");
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < backBufferCount; n++) {
        SUCCEEDED(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(commandAllocators[n]), commandAllocators[n].put_void()));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        commandAllocators[n]->SetName(name);
    }

    // Create a command list for recording graphics commands.
    SUCCEEDED(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].get(), nullptr, __uuidof(commandList), commandList.put_void()));
    SUCCEEDED(commandList->Close());

    commandList->SetName(L"DeviceResources");

    // Create a fence for tracking GPU execution progress.
    SUCCEEDED(d3dDevice->CreateFence(fenceValues[backBufferIndex], D3D12_FENCE_FLAG_NONE, __uuidof(fence), fence.put_void()));
    fenceValues[backBufferIndex]++;

    fence->SetName(L"DeviceResources");

    fenceEvent.add(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!fenceEvent) {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }
}

void MyDirectX12::DeviceResources::SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& panel, double const& deviceDpi) {
    swapChainPanel = panel;
    logicalSize = winrt::Windows::Foundation::Size(static_cast<float>(panel.ActualWidth()), static_cast<float>(panel.ActualHeight()));
    dpi = deviceDpi;
    compositionScaleX = panel.CompositionScaleX();
    compositionScaleY = panel.CompositionScaleY();
    CreateWindowSizeDependentResources();
}

void MyDirectX12::DeviceResources::CreateWindowSizeDependentResources() {
    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < backBufferCount; n++) {
        renderTargets[n] = nullptr;
        fenceValues[n] = fenceValues[backBufferIndex];
    }

    UpdateRenderTargetSize();

    DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

    bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
    d3dRenderTargetSize.Width = swapDimensions ? outputSize.Height : outputSize.Width;
    d3dRenderTargetSize.Height = swapDimensions ? outputSize.Width : outputSize.Height;

    // Determine the render target size in pixels.
    //const UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(outputSize.right - outputSize.left), 1u);
    //const UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(outputSize.bottom - outputSize.top), 1u);
    const DXGI_FORMAT tempBackBufferFormat = NoSRGB(backBufferFormat);

    // If the swap chain already exists, resize it, otherwise create one.
    if (swapChain) {
        // If the swap chain already exists, resize it.
        HRESULT hr = swapChain->ResizeBuffers(
            backBufferCount,
            d3dRenderTargetSize.Width,
            d3dRenderTargetSize.Height,
            tempBackBufferFormat,
            (options & allowTearing).all() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        } else {
            SUCCEEDED(hr);
        }
    } else {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = d3dRenderTargetSize.Width;
        swapChainDesc.Height = d3dRenderTargetSize.Height;
        swapChainDesc.Format = tempBackBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = (options & allowTearing).all() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

        // Create a swap chain for the window.
        winrt::com_ptr<IDXGISwapChain1> tempSwapChain;
        SUCCEEDED(dxgiFactory->CreateSwapChainForComposition(commandQueue.get(), &swapChainDesc, nullptr, tempSwapChain.put()));
        swapChain = tempSwapChain.try_as<IDXGISwapChain3>();

        swapChainPanel.DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::High, [=]() {
            winrt::com_ptr<ISwapChainPanelNative> panelNative;
            SUCCEEDED(winrt::get_unknown(swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative)));
            SUCCEEDED(panelNative->SetSwapChain(swapChain.get()));
            });

        /*winrt::com_ptr<IDXGIDevice3> dxgiDevice;
        dxgiDevice = d3dDevice.try_as<IDXGIDevice3>();

        SUCCEEDED(dxgiDevice->SetMaximumFrameLatency(1));*/

        // Handle color space settings for HDR
        UpdateColorSpace();

        // Set the proper orientation for the swap chain, and generate
        // matrix transformations for rendering to the rotated swap chain.
        switch (rotation) {
        default:
        case DXGI_MODE_ROTATION_IDENTITY:
            orientationTransform3D = ScreenRotation::Rotation0;
            break;

        case DXGI_MODE_ROTATION_ROTATE90:
            orientationTransform3D = ScreenRotation::Rotation270;
            break;

        case DXGI_MODE_ROTATION_ROTATE180:
            orientationTransform3D = ScreenRotation::Rotation180;
            break;

        case DXGI_MODE_ROTATION_ROTATE270:
            orientationTransform3D = ScreenRotation::Rotation90;
            break;
        }

        SUCCEEDED(swapChain->SetRotation(rotation));

        // Obtain the back buffers for this window which will be the final render targets
        // and create render target views for each of them.
        for (UINT n = 0; n < backBufferCount; n++) {
            SUCCEEDED(swapChain->GetBuffer(n, __uuidof(renderTargets[n]), renderTargets[n].put_void()));

            wchar_t name[25] = {};
            swprintf_s(name, L"Render target %u", n);
            renderTargets[n]->SetName(name);

            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = backBufferFormat;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

            const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
                rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(n), rtvDescriptorSize);
            d3dDevice->CreateRenderTargetView(renderTargets[n].get(), &rtvDesc, rtvDescriptor);
        }

        // Reset the index to the current back buffer.
        backBufferIndex = swapChain->GetCurrentBackBufferIndex();

        if (depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
            // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
            // on this surface.
            const CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                depthBufferFormat,
                d3dRenderTargetSize.Width,
                d3dRenderTargetSize.Height,
                1, // This depth stencil view has only one texture.
                1  // Use a single mipmap level.
            );
            depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

            D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
            depthOptimizedClearValue.Format = depthBufferFormat;
            depthOptimizedClearValue.DepthStencil.Depth = (options & reverseDepth).all() ? 0.0f : 1.0f;
            depthOptimizedClearValue.DepthStencil.Stencil = 0;

            SUCCEEDED(d3dDevice->CreateCommittedResource(
                &depthHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &depthStencilDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &depthOptimizedClearValue,
                __uuidof(depthStencil), depthStencil.put_void()
            ));

            depthStencil->SetName(L"Depth stencil");

            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = depthBufferFormat;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

            d3dDevice->CreateDepthStencilView(depthStencil.get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // Set the 3D rendering viewport and scissor rectangle to target the entire window.
        screenViewport.TopLeftX = screenViewport.TopLeftY = 0.f;
        screenViewport.Width = d3dRenderTargetSize.Width;
        screenViewport.Height = d3dRenderTargetSize.Height;
        screenViewport.MinDepth = D3D12_MIN_DEPTH;
        screenViewport.MaxDepth = D3D12_MAX_DEPTH;

        scissorRect.left = scissorRect.top = 0;
        scissorRect.right = static_cast<LONG>(d3dRenderTargetSize.Width);
        scissorRect.bottom = static_cast<LONG>(d3dRenderTargetSize.Height);
    }
}

void MyDirectX12::DeviceResources::ValidateDevice() {
    // The D3D Device is no longer valid if the default adapter changed since the device was created or if the device has been removed.

    DXGI_ADAPTER_DESC previousDesc;
    {
        winrt::com_ptr<IDXGIAdapter1> previousDefaultAdapter;
        SUCCEEDED(dxgiFactory->EnumAdapters1(0, previousDefaultAdapter.put()));

        SUCCEEDED(previousDefaultAdapter->GetDesc(&previousDesc));
    }

    DXGI_ADAPTER_DESC currentDesc;
    {
        winrt::com_ptr<IDXGIFactory4> currentFactory;
        SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(currentFactory), currentFactory.put_void()));

        winrt::com_ptr<IDXGIAdapter1> currentDefaultAdapter;
        SUCCEEDED(currentFactory->EnumAdapters1(0, currentDefaultAdapter.put()));

        SUCCEEDED(currentDefaultAdapter->GetDesc(&currentDesc));
    }

    // If the adapter LUIDs don't match, or if the device reports that it has been removed, a new D3D device must be created.

    if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart || previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart || d3dDevice->GetDeviceRemovedReason() < 0) {
#ifdef _DEBUG
        OutputDebugStringA("Device Lost on ValidateDevice\n");
#endif

        // Create a new device and swap chain.
        HandleDeviceLost();
    }
}

void MyDirectX12::DeviceResources::HandleDeviceLost() {
    if (deviceNotify) {
        deviceNotify->OnDeviceLost();
    }

    for (UINT n = 0; n < backBufferCount; n++) {
        commandAllocators[n]->Reset();
        renderTargets[n] = nullptr;
    }

    depthStencil = nullptr;
    commandQueue = nullptr;
    commandList = nullptr;
    fence = nullptr;
    rtvDescriptorHeap = nullptr;
    dsvDescriptorHeap = nullptr;
    swapChain = nullptr;
    d3dDevice = nullptr;
    dxgiFactory = nullptr;

#ifdef _DEBUG
    {
        winrt::com_ptr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (deviceNotify) {
        deviceNotify->OnDeviceRestored();
    }
}

void MyDirectX12::DeviceResources::Prepare(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {
    // Reset command list and allocator.
    SUCCEEDED(commandAllocators[backBufferIndex]->Reset());
    SUCCEEDED(commandList->Reset(commandAllocators[backBufferIndex].get(), nullptr));

    if (beforeState != afterState) {
        // Transition the render target into the correct state to allow for drawing into it.
        const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].get(), beforeState, afterState);
        commandList->ResourceBarrier(1, &barrier);
    }
}

void MyDirectX12::DeviceResources::Present(D3D12_RESOURCE_STATES beforeState) {
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT) {
        // Transition the render target to the state that allows it to be presented to the display.
        const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[backBufferIndex].get(), beforeState, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);
    }

    // Send the command list off to the GPU for processing.
    SUCCEEDED(commandList->Close());
    commandQueue->ExecuteCommandLists(1, CommandListCast(commandList.put()));
    
    HRESULT hr;
    if ((options & allowTearing).all()) {
        // Recommended to always use tearing if supported when using a sync interval of 0.
        hr = swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
    } else {
        // The first argument instructs DXGI to block until VSync, putting the application to sleep until the next VSync. This ensures we don't waste any cycles rendering frames that will never be displayed to the screen.
        hr = swapChain->Present(1, 0);
    }

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
            static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? d3dDevice->GetDeviceRemovedReason() : hr));
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    } else {
        SUCCEEDED(hr);

        MoveToNextFrame();

        if (!dxgiFactory->IsCurrent()) {
            UpdateColorSpace();
        }
    }
}

void MyDirectX12::DeviceResources::WaitForGpu() noexcept {
    if (commandQueue && fence && fenceEvent) {
        // Schedule a Signal command in the GPU queue.
        const UINT64 fenceValue = fenceValues[backBufferIndex];
        if (SUCCEEDED(commandQueue->Signal(fence.get(), fenceValue))) {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(fence->SetEventOnCompletion(fenceValue, &fenceEvent))) {
                std::ignore = WaitForSingleObjectEx(&fenceEvent, INFINITE, FALSE);

                // Increment the fence value for the current frame.
                fenceValues[backBufferIndex]++;
            }
        }
    }
}

void MyDirectX12::DeviceResources::UpdateColorSpace() {
    if (!dxgiFactory)
        return;

    if (!dxgiFactory->IsCurrent()) {
        // Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
        SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, __uuidof(dxgiFactory), dxgiFactory.put_void()));
    }

    DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    bool isDisplayHDR10 = false;

    if (swapChain) {
        // To detect HDR support, we will need to check the color space in the primary DXGI output associated with the app at this point in time (using window/display intersection).

        auto windowBounds = window.Bounds();

        const long ax1 = windowBounds.X;
        const long ay1 = windowBounds.Y;
        const long ax2 = windowBounds.X + windowBounds.Width;
        const long ay2 = windowBounds.Y + windowBounds.Height;

        winrt::com_ptr<IDXGIOutput> bestOutput;
        long bestIntersectArea = -1;

        winrt::com_ptr<IDXGIAdapter> adapter;
        for (UINT adapterIndex = 0; dxgiFactory->EnumAdapters(adapterIndex, adapter.put()) >= 0; ++adapterIndex) {
            winrt::com_ptr<IDXGIOutput> output;
            for (UINT outputIndex = 0; adapter->EnumOutputs(outputIndex, output.put()) >= 0; ++outputIndex) {
                // Get the rectangle bounds of current output.
                DXGI_OUTPUT_DESC desc;
                SUCCEEDED(output->GetDesc(&desc));
                const auto& r = desc.DesktopCoordinates;

                // Compute the intersection
                const long intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, r.left, r.top, r.right, r.bottom);
                if (intersectArea > bestIntersectArea) {
                    bestOutput = output;
                    bestIntersectArea = intersectArea;
                }
            }
        }

        if (bestOutput) {
            winrt::com_ptr<IDXGIOutput6> output6;
            output6 = bestOutput.try_as<IDXGIOutput6>();
            DXGI_OUTPUT_DESC1 desc;
            SUCCEEDED(output6->GetDesc1(&desc));

            if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
                // Display output is HDR10.
                isDisplayHDR10 = true;
            }
        }
    }

    if ((options & enableHDR).all() && isDisplayHDR10) {
        switch (backBufferFormat) {
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            // The application creates the HDR10 signal.
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            // The system creates the HDR10 signal; application uses linear values.
            colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
            break;

        default:
            break;
        }
    }

    colorSpace = colorSpace;

    UINT colorSpaceSupport = 0;
    if (swapChain && SUCCEEDED(swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)) && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)) {
        SUCCEEDED(swapChain->SetColorSpace1(colorSpace));
    }
}

void MyDirectX12::DeviceResources::MoveToNextFrame() {
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = fenceValues[backBufferIndex];
    SUCCEEDED(commandQueue->Signal(fence.get(), currentFenceValue));

    // Update the back buffer index.
    backBufferIndex = swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (fence->GetCompletedValue() < fenceValues[backBufferIndex]) {
        SUCCEEDED(fence->SetEventOnCompletion(fenceValues[backBufferIndex], &fenceEvent));
        std::ignore = WaitForSingleObjectEx(&fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    fenceValues[backBufferIndex] = currentFenceValue + 1;
}

void MyDirectX12::DeviceResources::GetAdapter(IDXGIAdapter1** ppAdapter) {
    *ppAdapter = nullptr;

    winrt::com_ptr<IDXGIAdapter1> adapter;

    winrt::com_ptr<IDXGIFactory6> factory6;
    factory6 = dxgiFactory.try_as<IDXGIFactory6>();
    for (UINT adapterIndex = 0; factory6->EnumAdapterByGpuPreference(adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(adapter), adapter.put_void()) >= 0; adapterIndex++) {
        DXGI_ADAPTER_DESC1 desc;
        SUCCEEDED(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.get(), d3dMinFeatureLevel, __uuidof(ID3D12Device), nullptr))) {
#ifdef _DEBUG
            wchar_t buff[256] = {};
            swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
            OutputDebugStringW(buff);
#endif
            break;
        }
    }

    if (!adapter) {
        for (UINT adapterIndex = 0;
            SUCCEEDED(dxgiFactory->EnumAdapters1(
                adapterIndex,
                adapter.put()));
            ++adapterIndex) {
            DXGI_ADAPTER_DESC1 desc;
            SUCCEEDED(adapter->GetDesc1(&desc));

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                // Don't select the Basic Render Driver adapter.
                continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.get(), d3dMinFeatureLevel, __uuidof(ID3D12Device), nullptr))) {
#ifdef _DEBUG
                wchar_t buff[256] = {};
                swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
                OutputDebugStringW(buff);
#endif
                break;
            }
        }
    }

#if !defined(NDEBUG)
    if (!adapter) {
        // Try WARP12 instead
        if (dxgiFactory->EnumWarpAdapter(__uuidof(adapter), adapter.put_void()) < 0) {
            throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter) {
        throw std::runtime_error("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.detach();
}

DXGI_MODE_ROTATION MyDirectX12::DeviceResources::ComputeDisplayRotation() {
    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

    switch (nativeOrientation) {
    case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Landscape:
        switch (currentOrientation) {
        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Landscape:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Portrait:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            break;
        }
        break;

    case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Portrait:
        switch (currentOrientation) {
        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Landscape:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_Portrait:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            break;

        case ABI::Windows::Graphics::Display::DisplayOrientations::DisplayOrientations_PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            break;
        }
        break;
    }
    return rotation;
}

void MyDirectX12::DeviceResources::UpdateRenderTargetSize() {
    effectiveDpi = dpi;
    effectiveCompositionScaleX = compositionScaleX;
    effectiveCompositionScaleY = compositionScaleY;

    // 为了延长高分辨率设备上的电池使用时间，请呈现到较小的呈现器目标
    // 并允许 GPU 在显示输出时缩放输出。
    if (!DisplayMetrics::SupportHighResolutions && dpi > DisplayMetrics::DpiThreshold) {
        float width = ConvertDipsToPixels(logicalSize.Width, dpi);
        float height = ConvertDipsToPixels(logicalSize.Height, dpi);

        // 当设备为纵向时，高度大于宽度。将
        // 较大尺寸与宽度阈值进行比较，将较小尺寸
        // 与高度阈值进行比较。
        if (std::max(width, height) > DisplayMetrics::WidthThreshold && std::min(width, height) > DisplayMetrics::HeightThreshold) {
            // 为了缩放应用，我们更改了有效 DPI。逻辑大小不变。
            effectiveDpi /= 2.0f;
            effectiveCompositionScaleX /= 2.0f;
            effectiveCompositionScaleY /= 2.0f;
        }
    }

    // 计算必要的呈现器目标大小(以像素为单位)。
    outputSize.Width = ConvertDipsToPixels(logicalSize.Width, effectiveDpi);
    outputSize.Height = ConvertDipsToPixels(logicalSize.Height, effectiveDpi);

    // 防止创建大小为零的 DirectX 内容。
    outputSize.Width = std::max(outputSize.Width, 1.0f);
    outputSize.Height = std::max(outputSize.Height, 1.0f);
}
