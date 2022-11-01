#pragma once
namespace MyDirectX12 {
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
    ~IDeviceNotify() = default;
    };

    // Controls all the DirectX device resources.
    class DirectX12Wrapper {
    public:
        static constexpr unsigned int c_AllowTearing = 0x1;
        static constexpr unsigned int c_EnableHDR = 0x2;
        static constexpr unsigned int c_ReverseDepth = 0x4;

        DirectX12Wrapper(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT, UINT backBufferCount = 2, D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0, unsigned int flags = 0) noexcept(false);
        ~DirectX12Wrapper();

        DirectX12Wrapper(DirectX12Wrapper&&) = default;
        DirectX12Wrapper& operator= (DirectX12Wrapper&&) = default;

        DirectX12Wrapper(DirectX12Wrapper const&) = delete;
        DirectX12Wrapper& operator= (DirectX12Wrapper const&) = delete;

        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void SetWindow(IUnknown* window, int width, int height, DXGI_MODE_ROTATION rotation) noexcept;
        bool WindowSizeChanged(int width, int height, DXGI_MODE_ROTATION rotation);
        void ValidateDevice();
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { deviceNotify = deviceNotify; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void WaitForGpu() noexcept;
        void UpdateColorSpace();

        // Device Accessors.
        RECT GetOutputSize() const noexcept { return outputSize; }
        DXGI_MODE_ROTATION GetRotation() const noexcept { return rotation; }

        // Direct3D Accessors.
        auto                        GetD3DDevice() const noexcept { return d3dDevice.get(); }
        auto                        GetSwapChain() const noexcept { return swapChain.get(); }
        auto                        GetDXGIFactory() const noexcept { return dxgiFactory.get(); }
        D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const noexcept { return d3dFeatureLevel; }
        ID3D12Resource* GetRenderTarget() const noexcept { return renderTargets[backBufferIndex].get(); }
        ID3D12Resource* GetDepthStencil() const noexcept { return depthStencil.get(); }
        ID3D12CommandQueue* GetCommandQueue() const noexcept { return commandQueue.get(); }
        ID3D12CommandAllocator* GetCommandAllocator() const noexcept { return commandAllocators[backBufferIndex].get(); }
        auto                        GetCommandList() const noexcept { return commandList.get(); }
        DXGI_FORMAT                 GetBackBufferFormat() const noexcept { return backBufferFormat; }
        DXGI_FORMAT                 GetDepthBufferFormat() const noexcept { return depthBufferFormat; }
        D3D12_VIEWPORT              GetScreenViewport() const noexcept { return screenViewport; }
        D3D12_RECT                  GetScissorRect() const noexcept { return scissorRect; }
        UINT                        GetCurrentFrameIndex() const noexcept { return backBufferIndex; }
        UINT                        GetBackBufferCount() const noexcept { return backBufferCount; }
        DirectX::XMFLOAT4X4         GetOrientationTransform3D() const noexcept { return orientationTransform3D; }
        DXGI_COLOR_SPACE_TYPE       GetColorSpace() const noexcept { return colorSpace; }
        unsigned int                GetDeviceOptions() const noexcept { return options; }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const noexcept {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(backBufferIndex), rtvDescriptorSize);
        }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        }

    private:
        void MoveToNextFrame();
        void GetAdapter(IDXGIAdapter1** ppAdapter);

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT                                        backBufferIndex;

        // Direct3D objects.
        winrt::com_ptr<ID3D12Device>                d3dDevice;
        winrt::com_ptr<ID3D12GraphicsCommandList>   commandList;
        winrt::com_ptr<ID3D12CommandQueue>          commandQueue;
        winrt::com_ptr<ID3D12CommandAllocator>      commandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
        winrt::com_ptr<IDXGIFactory4>               dxgiFactory;
        winrt::com_ptr<IDXGISwapChain3>             swapChain;
        winrt::com_ptr<ID3D12Resource>              renderTargets[MAX_BACK_BUFFER_COUNT];
        winrt::com_ptr<ID3D12Resource>              depthStencil;

        // Presentation fence objects.
        winrt::com_ptr<ID3D12Fence>                 fence;
        UINT64                                      fenceValues[MAX_BACK_BUFFER_COUNT];
        winrt::event<HANDLE>                        fenceEvent;

        // Direct3D rendering objects.
        winrt::com_ptr<ID3D12DescriptorHeap>        rtvDescriptorHeap;
        winrt::com_ptr<ID3D12DescriptorHeap>        dsvDescriptorHeap;
        UINT                                                rtvDescriptorSize;
        D3D12_VIEWPORT                                      screenViewport;
        D3D12_RECT                                          scissorRect;

        // Direct3D properties.
        DXGI_FORMAT                                         backBufferFormat;
        DXGI_FORMAT                                         depthBufferFormat;
        UINT                                                backBufferCount;
        D3D_FEATURE_LEVEL                                   d3dMinFeatureLevel;

        // Cached device properties.
        IUnknown* window;
        D3D_FEATURE_LEVEL                                   d3dFeatureLevel;
        DXGI_MODE_ROTATION                                  rotation;
        DWORD                                               dxgiFactoryFlags;
        RECT                                                outputSize;

        // Transforms used for display orientation.
        DirectX::XMFLOAT4X4                                 orientationTransform3D;

        // HDR Support
        DXGI_COLOR_SPACE_TYPE                               colorSpace;

        // DeviceResources options (see flags above)
        unsigned int                                        options;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* deviceNotify;
    };
}