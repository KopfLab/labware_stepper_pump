/*
 * GsWebhook.h
 * Created on: Marc 3, 2016
 * Author: Sebastian Kopf <seb.kopf@gmail.com>
 * Convenience wrapper for logging to google spreadsheet via a webhook
 */

#pragma once

class GsWebhook {
  // config
	char webhook[20];

  // data
  char date_time_buffer[20];
  char value_buffer[200];
  char json_buffer[255];

public:
  // constructor
  GsWebhook(const char* webhook) {
    strcpy( this->webhook, webhook );
  };

  // methods
  bool init ();
  bool send (const char* type, const char* variable) ;
  bool send (const char* type, const char* variable, const char* value) ;
  bool send (const char* type, const char* variable, const String &value) ;
  bool send (const char* type, const char* variable, int value) ;
  bool send (const char* type, const char* variable, double value) ;
  bool send (const char* type, const char* variable, const char* value, const char* msg) ;
  bool send (const char* type, const char* variable, const String &value, const char* msg) ;
  bool send (const char* type, const char* variable, int value, const char* msg) ;
  bool send (const char* type, const char* variable, double value, const char* msg) ;
  bool send (const char* type, const char* variable, const char* value, const char* unit, const char* msg) ;
  bool send (const char* type, const char* variable, const String &value, const char* unit, const char* msg) ;
  bool send (const char* type, const char* variable, int value, const char* unit, const char* msg) ;
  bool send (const char* type, const char* variable, double value, const char* unit, const char* msg) ;
};

bool GsWebhook::init() {
  return(send("event", "startup", "", "", "complete"));
}

bool GsWebhook::send(const char* type, const char* variable, const char* value, const char* unit, const char* msg) {
  Time.format(Time.now(), "%Y-%m-%d %H:%M:%S").toCharArray(date_time_buffer, sizeof(date_time_buffer));
  snprintf(json_buffer, sizeof(json_buffer),
    "{\"datetime\":\"%s\",\"type\":\"%s\",\"var\":\"%s\",\"value\":\"%s\", \"units\":\"%s\",\"msg\":\"%s\"}",
    date_time_buffer, type, variable, value, unit, msg);
  return(Particle.publish(webhook, json_buffer));
}

bool GsWebhook::send(const char* type, const char* variable, const String &value, const char* unit, const char* msg) {
  value.toCharArray(value_buffer, sizeof(value_buffer));
  return(send(type, variable, value_buffer, unit, msg));
}

bool GsWebhook::send(const char* type, const char* variable, int value, const char* unit, const char* msg) {
  sprintf(value_buffer, "%d", value);
  return(send(type, variable, value_buffer, unit, msg));
}

bool GsWebhook::send(const char* type, const char* variable, double value, const char* unit, const char* msg) {
  sprintf(value_buffer, "%.3f", value);
  return(send(type, variable, value_buffer, unit, msg));
}

bool GsWebhook::send(const char* type, const char* variable) { return(send(type, variable, "", "")); }
bool GsWebhook::send(const char* type, const char* variable, const char* value) { return(send(type, variable, value, "")); }
bool GsWebhook::send(const char* type, const char* variable, const String &value) { return(send(type, variable, value, "")); }
bool GsWebhook::send(const char* type, const char* variable, int value) { return(send(type, variable, value, "")); }
bool GsWebhook::send(const char* type, const char* variable, double value) { return(send(type, variable, value, "")); }
bool GsWebhook::send(const char* type, const char* variable, const char* value, const char* unit) { return(send(type, variable, value, unit, "")); }
bool GsWebhook::send(const char* type, const char* variable, const String &value, const char* unit) { return(send(type, variable, value, unit, "")); }
bool GsWebhook::send(const char* type, const char* variable, int value, const char* unit) { return(send(type, variable, value, unit, "")); }
bool GsWebhook::send(const char* type, const char* variable, double value, const char* unit) { return(send(type, variable, value, unit, "")); }
