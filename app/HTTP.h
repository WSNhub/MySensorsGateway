#ifndef INCLUDE_HTTP_H_
#define INCLUDE_HTTP_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

class HTTPClass
{
  public:
    void begin();
    bool isHttpClientAllowed(HttpRequest &request,
                             HttpResponse &response);

  private:
    HttpServer server;
};

extern HTTPClass HTTP;

#endif //INCLUDE_HTTP_H_
