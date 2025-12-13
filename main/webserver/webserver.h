#ifndef SPV_WEBSERVER_WEBSERVER_H_
#define SPV_WEBSERVER_WEBSERVER_H_

#include "esp_http_server.h"

#define WEBSERVER_POST_EVENT BIT0

struct webserver_handle {
	httpd_handle_t httpd_handle;
	char *root;
	EventGroupHandle_t event_group;
};

struct webserver_handle* webserver_start(const char *root);
void webserver_stop(struct webserver_handle *handle);

#endif
