#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <HTTP.h>

class DeviceHandler {

  public:
    DeviceHandler(void);

    void ICACHE_FLASH_ATTR registerHttpHandlers(HttpServer &server);

    static void onGpio(HttpRequest &request, HttpResponse &response); // Needs to be static to be able to be set as a handler.

  private:
    void onGpioMbr(HttpRequest &request, HttpResponse &response); // The real handler.

    // GPIO 0..16 values.
    static String hardwarePinVals[];
    // GPIO 0..16 direction (0 = input; 1 = output)
    static String hardwarePinDirs[];

};

extern DeviceHandler deviceHandler;

#endif // DEVICE_HANDLER_H
