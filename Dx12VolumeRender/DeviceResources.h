#pragma once
#include <pch.h>
#include <bitset>

namespace MyDirectX12 {
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~IDeviceNotify() = default;
    };

    // Controls all the DirectX device resources.
    class DeviceResources {
    public:
        static constexpr std::bitset<1> allowTearing = 0x1;
        static constexpr std::bitset<1> enableHDR = 0x2;
        static constexpr std::bitset<1> reverseDepth = 0x4;

        DeviceResources(winrt::Microsoft::UI::Xaml::Window const& window, DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT, UINT backBufferCount = 2, D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0, unsigned int flags = 0) noexcept(false);
        ~DeviceResources();

        DeviceResources(DeviceResources&&) = default;
        DeviceResources& operator= (DeviceResources&&) = default;

        DeviceResources(DeviceResources const&) = delete;
        DeviceResources& operator= (DeviceResources const&) = delete;

        void CreateDeviceResources();
        void SetSwapChainPanel(winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel const& swapChainPanel, double const& rasterizationScale);
        void CreateWindowSizeDependentResources();
        void ValidateDevice();
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { deviceNotify = deviceNotify; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void WaitForGpu() noexcept;
        void UpdateColorSpace();
        void ChangeSwapChainSize(winrt::Windows::Foundation::Size const& newSize);

        // Device Accessors.
        winrt::Windows::Foundation::Size GetOutputSize() const noexcept { return outputSize; }
        DXGI_MODE_ROTATION GetRotation() const noexcept { return rotation; }

        // Direct3D Accessors.
        auto GetD3DDevice() const noexcept { return d3dDevice.get(); }
        auto GetSwapChain() const noexcept { return swapChain.get(); }
        auto GetDXGIFactory() const noexcept { return dxgiFactory.get(); }
        D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const noexcept { return d3dFeatureLevel; }
        ID3D12Resource* GetRenderTarget() const noexcept { return renderTargets[backBufferIndex].get(); }
        ID3D12Resource* GetDepthStencil() const noexcept { return depthStencil.get(); }
        ID3D12CommandQueue* GetCommandQueue() const noexcept { return commandQueue.get(); }
        ID3D12CommandAllocator* GetCommandAllocator() const noexcept { return commandAllocators[backBufferIndex].get(); }
        auto GetCommandList() const noexcept { return commandList.get(); }
        DXGI_FORMAT GetBackBufferFormat() const noexcept { return backBufferFormat; }
        DXGI_FORMAT GetDepthBufferFormat() const noexcept { return depthBufferFormat; }
        D3D12_VIEWPORT GetScreenViewport() const noexcept { return screenViewport; }
        D3D12_RECT GetScissorRect() const noexcept { return scissorRect; }
        UINT GetCurrentFrameIndex() const noexcept { return backBufferIndex; }
        UINT GetBackBufferCount() const noexcept { return backBufferCount; }
        DirectX::XMFLOAT4X4 GetOrientationTransform3D() const noexcept { return orientationTransform3D; }
        DXGI_COLOR_SPACE_TYPE GetColorSpace() const noexcept { return colorSpace; }
        std::bitset<1> GetDeviceOptions() const noexcept { return options; }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const noexcept { return CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),static_cast<INT>(backBufferIndex), rtvDescriptorSize); }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept { return CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }

    private:
        void MoveToNextFrame();
        void GetAdapter(IDXGIAdapter1** ppAdapter);

        DXGI_MODE_ROTATION ComputeDisplayRotation();
        void UpdateRenderTargetSize();
        void FlushGpu();

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT backBufferIndex;

        // Direct3D objects.
        winrt::com_ptr<ID3D12Device> d3dDevice;
        winrt::com_ptr<ID3D12GraphicsCommandList> commandList;
        winrt::com_ptr<ID3D12CommandQueue> commandQueue;
        winrt::com_ptr<ID3D12CommandAllocator> commandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
        winrt::com_ptr<IDXGIFactory4> dxgiFactory;
        winrt::com_ptr<IDXGISwapChain3> swapChain;
        winrt::com_ptr<ID3D12Resource> renderTargets[MAX_BACK_BUFFER_COUNT];
        winrt::com_ptr<ID3D12Resource> depthStencil;

        // XAML composition reference.
        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel swapChainPanel;

        // Presentation fence objects.
        winrt::com_ptr<ID3D12Fence> fence;
        UINT64 fenceValues[MAX_BACK_BUFFER_COUNT];
        winrt::event<HANDLE> fenceEvent;

        // Direct3D rendering objects.
        winrt::com_ptr<ID3D12DescriptorHeap> rtvDescriptorHeap;
        winrt::com_ptr<ID3D12DescriptorHeap> dsvDescriptorHeap;
        UINT rtvDescriptorSize;
        D3D12_VIEWPORT screenViewport;
        D3D12_RECT scissorRect;

        // Direct3D properties.
        DXGI_FORMAT backBufferFormat;
        DXGI_FORMAT depthBufferFormat;
        UINT backBufferCount;
        D3D_FEATURE_LEVEL d3dMinFeatureLevel;

        // Cached device properties.
        winrt::Microsoft::UI::Xaml::Window window;
        D3D_FEATURE_LEVEL d3dFeatureLevel;
        winrt::Windows::Foundation::Size logicalSize;
        winrt::Windows::Foundation::Size d3dRenderTargetSize;
        ABI::Windows::Graphics::Display::DisplayOrientations nativeOrientation;
        ABI::Windows::Graphics::Display::DisplayOrientations currentOrientation;
        double rasterizationScale;
        float compositionScaleX;
        float compositionScaleY;
        DXGI_MODE_ROTATION  rotation;
        DWORD dxgiFactoryFlags;
        winrt::Windows::Foundation::Size outputSize;

        // Transforms used for display orientation.
        DirectX::XMFLOAT4X4 orientationTransform3D;

        // HDR Support
        DXGI_COLOR_SPACE_TYPE colorSpace;

        // DeviceResources options (see flags above)
        std::bitset<1> options;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* deviceNotify;
    };

    // Helper class for COM exceptions
    class com_exception : public std::exception {
    public:
        com_exception(HRESULT hr) : result(hr) {}

        const char* what() const override {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr) {
        if (FAILED(hr)) {
            throw com_exception(hr);
        }
    }
}