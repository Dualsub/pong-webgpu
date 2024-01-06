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

        wgpu::RequestAdapterOptions options = {};
        options.powerPreference = wgpu::PowerPreference::HighPerformance;

        instance.RequestAdapter(
            &options,
            [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter,
               const char *message, void *userdata)
            {
                if (status != WGPURequestAdapterStatus_Success)
                {
                    std::string messageString = message ? message : "";
                    std::cerr << "Failed to initialize adapter:\n"
                              << "Message: " << messageString << "\n"
                              << "Status: " << status;
                    return;
                }

                wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
                adapter.RequestDevice(
                    nullptr,
                    [](WGPURequestDeviceStatus status, WGPUDevice cDevice,
                       const char *message, void *userdata)
                    {
                        if (status != WGPURequestDeviceStatus_Success)
                        {
                            std::string messageString = message ? message : "";
                            std::cerr << "Failed to initialize device:\n"
                                      << "Message: " << messageString << "\n"
                                      << "Status: " << status;
                            return;
                        }

                        wgpu::Device device = wgpu::Device::Acquire(cDevice);
                        UserData *userData = static_cast<UserData *>(userdata);
                        DeviceContext context = {userData->instance, device};
                        userData->callback(context);
                    },
                    userdata);
            },
            &userData);
    }
}