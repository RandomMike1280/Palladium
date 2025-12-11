#pragma once

#ifdef _WIN32

#include <d3d11.h>
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <cstdio>

// Link required libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwrite.lib")

namespace palladium {

using Microsoft::WRL::ComPtr;

/**
 * D2DContext - Singleton managing Direct2D/Direct3D device resources
 * 
 * This class initializes and maintains the GPU rendering pipeline:
 * - D3D11 Device (GPU access)
 * - DXGI Factory (swap chain creation)
 * - D2D1 Factory and Device (2D rendering)
 * - DWrite Factory (text rendering)
 */
class D2DContext {
public:
    static D2DContext& instance();
    
    // Prevent copying
    D2DContext(const D2DContext&) = delete;
    D2DContext& operator=(const D2DContext&) = delete;
    
    // Device accessors
    ID3D11Device* get_d3d_device() const { return d3d_device_.Get(); }
    ID3D11DeviceContext* get_d3d_context() const { return d3d_context_.Get(); }
    IDXGIFactory2* get_dxgi_factory() const { return dxgi_factory_.Get(); }
    ID2D1Factory1* get_d2d_factory() const { return d2d_factory_.Get(); }
    ID2D1Device* get_d2d_device() const { return d2d_device_.Get(); }
    IDWriteFactory* get_dwrite_factory() const { return dwrite_factory_.Get(); }
    
    // Create a new D2D device context for rendering
    ComPtr<ID2D1DeviceContext> create_device_context();
    
    // Check if GPU acceleration is available
    bool is_available() const { return initialized_; }
    
    // Get feature level
    D3D_FEATURE_LEVEL get_feature_level() const { return feature_level_; }

private:
    D2DContext();
    ~D2DContext();
    
    void initialize();
    void shutdown();
    
    bool initialized_ = false;
    D3D_FEATURE_LEVEL feature_level_ = D3D_FEATURE_LEVEL_9_1;
    
    // D3D11 resources
    ComPtr<ID3D11Device> d3d_device_;
    ComPtr<ID3D11DeviceContext> d3d_context_;
    ComPtr<IDXGIDevice1> dxgi_device_;
    ComPtr<IDXGIFactory2> dxgi_factory_;
    
    // D2D1 resources
    ComPtr<ID2D1Factory1> d2d_factory_;
    ComPtr<ID2D1Device> d2d_device_;
    
    // DWrite for text
    ComPtr<IDWriteFactory> dwrite_factory_;
};

// Helper to check HRESULT and throw on failure
inline void throw_if_failed(HRESULT hr, const char* message) {
    if (FAILED(hr)) {
        char hex_buf[32];
        snprintf(hex_buf, sizeof(hex_buf), "0x%08lX", static_cast<unsigned long>(hr));
        throw std::runtime_error(std::string(message) + " (HRESULT: " + hex_buf + ")");
    }
}

} // namespace palladium

#endif // _WIN32
