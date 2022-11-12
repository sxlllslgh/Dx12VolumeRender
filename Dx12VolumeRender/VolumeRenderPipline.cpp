#include "pch.h"
#include "VolumeRenderPipline.h"

void MyDirectX12::VolumeRenderPipline::Update(StepTimer const& timer) {
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    elapsedTime;

    PIXEndEvent();
}

void MyDirectX12::VolumeRenderPipline::Render(StepTimer const& timer) {
    // Don't try to render anything before the first Update.
    if (timer.GetFrameCount() == 0) {
        return;
    }

    // Prepare the command list to render a new frame.
    deviceResources->Prepare();
    Clear();

    auto commandList = deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // TODO: Add your rendering code here.

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(deviceResources->GetCommandQueue(), PIX_COLOR_DEFAULT, L"Present");
    deviceResources->Present();

    // If using the DirectX Tool Kit for DX12, uncomment this line:
    // m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    PIXEndEvent(deviceResources->GetCommandQueue());
}

void MyDirectX12::VolumeRenderPipline::Clear() {
    auto commandList = deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = deviceResources->GetDepthStencilView();
    
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ConvertColorToArray(winrt::Windows::UI::Colors::CornflowerBlue()), 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = deviceResources->GetScreenViewport();
    auto const scissorRect = deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

inline const FLOAT* MyDirectX12::VolumeRenderPipline::ConvertColorToArray(winrt::Windows::UI::Color const& color) {
    auto array = new FLOAT[4];
    array[0] = color.R;
    array[1] = color.G;
    array[2] = color.B;
    array[3] = color.A;
    return array;
}
