#include "pong/Device.h"

#include <iostream>

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
                std::cout << "Adapter status: " << status << std::endl;
                if (status != WGPURequestAdapterStatus_Success)
                {
                    std::cout << "Failed to initialize adapter" << std::endl;
                    exit(1);
                }
                wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
                adapter.RequestDevice(
                    nullptr,
                    [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
                       const char *message, void *userdata)
                    {
                        if (status != WGPURequestDeviceStatus_Success)
                        {
                            exit(1);
                        }

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