#ifndef INCLUDE_CONTROLLER_H_
#define INCLUDE_CONTROLLER_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>

class Controller
{
  public:
    virtual void begin() {};
    virtual void notifyChange(String object, String value) {};
    virtual void registerHttpHandlers(HttpServer &server) {};
    virtual void registerCommandHandlers() {};
};

#endif //INCLUDE_CONTROLLER_H_
