#ifndef SPV_WEBSERVER_REQUEST_H_
#define SPV_WEBSERVER_REQUEST_H_

#include "esp_http_server.h"

void webserver_request_redirect(httpd_req_t *req, const char *path);
esp_err_t webserver_request_read(
	httpd_req_t *req,
	size_t *size, char **output,
	bool null_terminate
);

struct webserver_post_format {
	const char *name;
	bool required;

	char **output;
};
esp_err_t webserver_read_post(
	httpd_req_t *req,
	struct webserver_post_format *format
);


#endif
