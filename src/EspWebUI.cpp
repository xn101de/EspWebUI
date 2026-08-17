// WebUI.cpp
#include "EspWebUI.h"
#include <stdio.h>
#include <string.h>

EspWebUI::EspWebUI(uint16_t port, const char *faviconSvgSrc) : server(port), ws("/ws"), lastHeartbeatTime(0), lastOnLoadTime(0) {
  // Setze den Session Token auf null/0
  memset(sessionToken, 0, sizeof(sessionToken));

  if (faviconSvgSrc != nullptr) {
    faviconSvgPtr = faviconSvgSrc;
  } else {
    faviconSvgPtr = faviconLibSvg;
  }
}

void EspWebUI::begin() {

  generateSessionToken(sessionToken, sizeof(sessionToken));
  setupWebSocket();
  setupRoutes();

  server.addHandler(&ws).addMiddleware([this](AsyncWebServerRequest *request, ArMiddlewareNext next) {
    if (!isAuthenticated(request)) {
      // the WS upgrade request carries the same session cookie as any other
      // request - reject it here so an unauthenticated client can never open
      // a socket at all, instead of trying to gate every message afterwards.
      request->send(401, "text/plain", "not authenticated");
    } else if (ws.count() >= WEBUI_MAX_WS_CLIENT) {
      // too many clients - answer back immediately and stop processing next middlewares and handler
      request->send(429, "text/plain", "no more client connection allowed");
    } else {
      // process next middleware and at the end the handler
      next();
    }
  });

  server.begin();
}

void EspWebUI::loop() {

  unsigned long currentTime = millis();
  if (currentTime - lastHeartbeatTime >= 3000) {
    lastHeartbeatTime = currentTime;
    wsSendHeartbeat();
  }
}
