#include "pch.h"
#include "DirectX12Wrapper.h"

MyDirectX12::DirectX12Wrapper::DirectX12Wrapper(DXGI_FORMAT backBufferFormat, DXGI_FORMAT depthBufferFormat, UINT backBufferCount, D3D_FEATURE_LEVEL minFeatureLevel, unsigned int flags) noexcept(false) : backBufferIndex(0), fenceValues{}, rtvDescriptorSize(0), screenViewport{}, scissorRect{}, backBufferFormat(backBufferFormat), depthBufferFormat(depthBufferFormat), backBufferCount(backBufferCount), d3dMinFeatureLevel(minFeatureLevel), window(nullptr), d3dFeatureLevel(D3D_FEATURE_LEVEL_11_0), rotation(DXGI_MODE_ROTATION_IDENTITY), dxgiFactoryFlags(0), outputSize{ 0, 0, 1, 1 }, orientationTransform3D(ScreenRotation::Rotation0), colorSpace(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709), options(flags), deviceNotify(nullptr) {
    if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT) {
        throw std::out_of_range("invalid backBufferCount");
    }

    if (minFeatureLevel < D3D_FEATURE_LEVEL_11_0) {
        throw std::out_of_range("minFeatureLevel too low");
    }
}

MyDirectX12::DirectX12Wrapper::~DirectX12Wrapper() {
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();
}

void MyDirectX12::DirectX12Wrapper::CreateDeviceResources() {
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        winrt::com_ptr<ID3D12Debug> debugController;
        if (winrt::check_hresult(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.put())))) {
            debugController->EnableDebugLayer();
        }
        else {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        winrt::com_ptr<IDXGIInfoQueue> dxgiInfoQueue;
        if (winrt::check_hresult(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.put())))) {
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

    winrt::check_hresult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(dxgiFactory.put())));

    // Determines whether tearing support is available for fullscreen borderless windows.
    if (options & c_AllowTearing) {
        BOOL allowTearing = FALSE;

        winrt::com_ptr<IDXGIFactory5> factory5;
        dxgiFactory.try_as<IDXGIFactory5>();
        auto hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));

        if (FAILED(hr) || !allowTearing) {
            options &= ~c_AllowTearing;
#ifdef _DEBUG
            OutputDebugStringA("WARNING: Variable refresh rate displays not supported");
#endif
        }
    }

    winrt::com_ptr<IDXGIAdapter1> adapter;
    GetAdapter(adapter.put());

    // Create the DX12 API device object.
    winrt::check_hresult(D3D12CreateDevice(adapter.get(), d3dMinFeatureLevel, IID_PPV_ARGS(d3dDevice.put())));

    d3dDevice->SetName(L"DeviceResources");

#ifndef NDEBUG
    // Configure debug device (if active).
    winrt::com_ptr<ID3D12InfoQueue> d3dInfoQueue;
    d3dDevice.try_as<ID3D12InfoQueue>();
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

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT hr = d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (winrt::check_hresult(hr)) {
        d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else {
        d3dFeatureLevel = d3dMinFeatureLevel;
    }

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    winrt::check_hresult(d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(commandQueue.put())));

    commandQueue->SetName(L"DeviceResources");

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = backBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    winrt::check_hresult(d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(rtvDescriptorHeap.put())));

    rtvDescriptorHeap->SetName(L"DeviceResources");

    rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (depthBufferFormat != DXGI_FORMAT_UNKNOWN) {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        winrt::check_hresult(d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(dsvDescriptorHeap.put())));

        dsvDescriptorHeap->SetName(L"DeviceResources");
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < backBufferCount; n++) {
        winrt::check_hresult(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocators[n].put())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        commandAllocators[n]->SetName(name);
    }

    // Create a command list for recording graphics commands.
    winrt::check_hresult(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[0].get(), nullptr, IID_PPV_ARGS(commandList.put())));
    winrt::check_hresult(commandList->Close());

    commandList->SetName(L"DeviceResources");

    // Create a fence for tracking GPU execution progress.
    winrt::check_hresult(d3dDevice->CreateFence(fenceValues[backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.put())));
    fenceValues[backBufferIndex]++;

    fence->SetName(L"DeviceResources");

    fenceEvent.add(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!fenceEvent) {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }
}
