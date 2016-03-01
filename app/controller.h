#ifndef INCLUDE_CONTROLLER_H_
#define INCLUDE_CONTROLLER_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <globals.h>

class Controller
{
  public:
    virtual void begin() = 0;
    virtual void notifyChange(String object, String value) = 0;
    virtual void registerHttpHandlers(HttpServer &server) = 0;
    virtual void registerCommandHandlers() = 0;
};

#if CONTROLLER_TYPE == CONTROLLER_TYPE_OPENHAB
  #include <openHabMqttController.h>
  class OpenHabMqttController;
  extern OpenHabMqttController controller;
#elif CONTROLLER_TYPE == CONTROLLER_TYPE_CLOUD
  #include <CloudController.h>
  class CloudController;
  extern CloudController controller;
#else
  #error Wrong CONTROLLER_TYPE value
#endif

#endif //INCLUDE_CONTROLLER_H_
