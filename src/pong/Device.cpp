#include "pong/Device.h"

namespace pong
{
    void WithDevice(void (*callback)(const DeviceContext &context))
    {
        wgpu::Instance instance = wgpu::CreateInstance();

        struct UserData
        {
            void (*callback)(const DeviceContext &context);
            wgpu::Instance instance;
        } userData = {callback, instance};

        instance.RequestAdapter(
            nullptr,
            [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
               const char *message, void *userdata)
            {
                if (status != WGPURequestAdapterStatus_Success)
                {
                    exit(0);
                }
                wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
                adapter.RequestDevice(
                    nullptr,
                    [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
                       const char *message, void *userdata)
                    {
                        wgpu::Device device = wgpu::Device::Acquire(cDevice);
                        UserData *userData = reinterpret_cast<UserData *>(userdata);
                        DeviceContext context = {userData->instance, device};
                        userData->callback(context);
                    },
                    userdata);
            },
            &userData);
    }
}