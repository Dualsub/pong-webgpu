#pragma once
#include <webgpu/webgpu_cpp.h>

namespace pong
{
    struct DeviceContext
    {
        wgpu::Instance instance = {};
        wgpu::Device device = {};
    };

    void WithDevice(void (*callback)(const DeviceContext &context));
}