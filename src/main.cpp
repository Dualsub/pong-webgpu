#include "pong/Application.h"
#include "pong/Device.h"

pong::Application app;

int main()
{
    pong::WithDevice(
        [](const pong::DeviceContext &context)
        {
            app.Run(context);
        });
    return 0;
}
