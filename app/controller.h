#ifndef INCLUDE_CONTROLLER_H_
#define INCLUDE_CONTROLLER_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>

class Controller
{
  public:
    virtual void begin() = 0;
    virtual void notifyChange(String object, String value) = 0;
    virtual void registerHttpHandlers(HttpServer &server) = 0;
    virtual void registerCommandHandlers() = 0;
};

#endif //INCLUDE_CONTROLLER_H_
