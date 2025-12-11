#ifdef _WIN32

#include "d2d_context.hpp"

namespace palladium {

D2DContext& D2DContext::instance() {
    static D2DContext instance;
    return instance;
}

D2DContext::D2DContext() {
    initialize();
}

D2DContext::~D2DContext() {
    shutdown();
}

void D2DContext::initialize() {
    if (initialized_) return;
    
    HRESULT hr;
    
    // Create D3D11 device with BGRA support for D2D interop
    UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    
#ifdef _DEBUG
    creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };
    
    hr = D3D11CreateDevice(
        nullptr,                    // Default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // Hardware acceleration
        nullptr,                    // No software rasterizer
        creation_flags,
        feature_levels,
        ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION,
        &d3d_device_,
        &feature_level_,
        &d3d_context_
    );
    
    // Fallback to WARP (software) if hardware fails
    if (FAILED(hr)) {
        hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            creation_flags,
            feature_levels,
            ARRAYSIZE(feature_levels),
            D3D11_SDK_VERSION,
            &d3d_device_,
            &feature_level_,
            &d3d_context_
        );
    }
    
    if (FAILED(hr)) {
        // GPU not available
        return;
    }
    
    // Get DXGI device
    hr = d3d_device_.As(&dxgi_device_);
    if (FAILED(hr)) return;
    
    // Get DXGI adapter
    ComPtr<IDXGIAdapter> dxgi_adapter;
    hr = dxgi_device_->GetAdapter(&dxgi_adapter);
    if (FAILED(hr)) return;
    
    // Get DXGI factory
    hr = dxgi_adapter->GetParent(IID_PPV_ARGS(&dxgi_factory_));
    if (FAILED(hr)) return;
    
    // Create D2D1 factory
    D2D1_FACTORY_OPTIONS options = {};
#ifdef _DEBUG
    options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    
    hr = D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory1),
        &options,
        reinterpret_cast<void**>(d2d_factory_.GetAddressOf())
    );
    if (FAILED(hr)) return;
    
    // Create D2D1 device
    hr = d2d_factory_->CreateDevice(dxgi_device_.Get(), &d2d_device_);
    if (FAILED(hr)) return;
    
    // Create DWrite factory for text
    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(dwrite_factory_.GetAddressOf())
    );
    if (FAILED(hr)) return;
    
    initialized_ = true;
}

void D2DContext::shutdown() {
    dwrite_factory_.Reset();
    d2d_device_.Reset();
    d2d_factory_.Reset();
    dxgi_factory_.Reset();
    dxgi_device_.Reset();
    d3d_context_.Reset();
    d3d_device_.Reset();
    initialized_ = false;
}

ComPtr<ID2D1DeviceContext> D2DContext::create_device_context() {
    if (!initialized_) {
        throw std::runtime_error("D2DContext not initialized - GPU acceleration unavailable");
    }
    
    ComPtr<ID2D1DeviceContext> context;
    HRESULT hr = d2d_device_->CreateDeviceContext(
        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
        &context
    );
    throw_if_failed(hr, "Failed to create D2D device context");
    
    return context;
}

} // namespace palladium

#endif // _WIN32
