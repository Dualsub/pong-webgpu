#include "pong/Application.h"
#include "pong/Device.h"

int main()
{
    static pong::Application app;
    pong::WithDevice(
        [](const pong::DeviceContext &context)
        {
            app.Run(context);
        });
    return 0;
}
