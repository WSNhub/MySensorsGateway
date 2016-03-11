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

    void addWsCommand(String command, WebSocketMessageDelegate callback);
    void notifyWsClients(String message);

  private:
    /* Websocket handlers */
    void wsConnected(WebSocket& socket);
    void wsMessageReceived(WebSocket& socket, const String& message);
    void wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size);
    void wsDisconnected(WebSocket& socket);

  private:
    HttpServer server;
    HashMap<String, WebSocketMessageDelegate> wsCommandHandlers;
};

extern HTTPClass HTTP;

#endif //INCLUDE_HTTP_H_
