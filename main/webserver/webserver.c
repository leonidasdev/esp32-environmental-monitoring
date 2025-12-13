#include "webserver.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_system.h"
#include "http_parser.h"
#include "cJSON.h"

#include "config/persistence.h"
#include "webserver/request.h"
#include "fs/persistence.h"
#include "fs/file.h"


const char *TAG = "SPV web server";

static esp_err_t webserver_get_handler(httpd_req_t *req);
static esp_err_t webserver_update_handler(httpd_req_t *req);

struct webserver_handle* webserver_start(const char *root) {
	ESP_LOGI(TAG, "Starting web server...");

	httpd_handle_t server = NULL;
	httpd_config_t conf = HTTPD_DEFAULT_CONFIG();
	conf.uri_match_fn = httpd_uri_match_wildcard;

	ESP_ERROR_CHECK(httpd_start(&server, &conf));

	struct webserver_handle *webserver_handle = malloc(sizeof(*webserver_handle));
	assert(webserver_handle != NULL);

	webserver_handle->httpd_handle = server,
	webserver_handle->root = malloc(strlen(root) + 1);
	webserver_handle->event_group = xEventGroupCreate();

	assert(webserver_handle->root != NULL);

	strcpy(webserver_handle->root, root);

	httpd_uri_t post_handler = {
		.uri		= "/change_config",
		.method		= HTTP_POST,
		.handler	= webserver_update_handler,
		.user_ctx	= webserver_handle,
	};
	httpd_uri_t get_handler = {
		.uri		= "/*",
		.method		= HTTP_GET,
		.handler	= webserver_get_handler,
		.user_ctx	= webserver_handle,
	};


	httpd_register_uri_handler(server, &post_handler);
	httpd_register_uri_handler(server, &get_handler);

	ESP_LOGI(TAG, "Webserver started");
	return webserver_handle;
}

void webserver_stop(struct webserver_handle *handle) {
	httpd_stop(handle->httpd_handle);
	free(handle->root);
	vEventGroupDelete(handle->event_group);
	free(handle);
}


static esp_err_t webserver_get_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");

	struct webserver_handle *ctx = req->user_ctx;
	char uri[sizeof(req->uri)];
	strcpy(uri, req->uri);

	// Check for parameters.
	uri[strcspn(uri, "?")] = '\0';

	if(strcmp(uri, "/") == 0) {
		strcpy(uri, "/index.htm");
	}

	if(strlen(ctx->root) + strlen(uri) >= sizeof(req->uri)) {
		ESP_LOGE(TAG, "Path too long.");
		httpd_resp_send_500(req);
		return ESP_FAIL;
	}

	char *extension = strrchr(uri, '.');
	if(extension != NULL) {
		const char *content_type = "text/html";
		if(strcmp(".js", extension) == 0) {
			content_type = "text/javascript";
		} else if(strcmp(".css", extension) == 0) {
			content_type = "text/css";
		}
		httpd_resp_set_type(req, content_type);
	}

	char *path = malloc(sizeof(req->uri));
	assert(path);
	*path = '\0';
	strcat(path, ctx->root);
	strcat(path, uri);

	char *file;
	size_t file_size;
	if(fs_file_read(path, &file_size, &file, true) != ESP_OK) {
		free(path);
		ESP_LOGE(TAG, "GET 404 %s", req->uri);
		httpd_resp_send_404(req);
		return ESP_ERR_NOT_FOUND;
	}
	free(path);

	ESP_LOGI(TAG, "GET 200 %s", req->uri);
	httpd_resp_send(req, file, file_size - 1);

	free(file);
	return ESP_OK;
}

static bool parse_key(spv_config_t *config, char *string) {
    size_t count = 0;
    char *save;
    char *tok = strtok_r(string, ":", &save);

    while (tok) {
        if (strlen(tok) != 2 || count >= sizeof(config->mesh_key))
            return false;

        config->mesh_key[count++] = (uint8_t)strtoul(tok, NULL, 16);
        tok = strtok_r(NULL, ":", &save);
    }

    return (int)count;
}

static esp_err_t webserver_update_handler(httpd_req_t *req) {
	httpd_resp_set_type(req, "text/html");

	struct webserver_handle *ctx = req->user_ctx;

	spv_config_t config = {0};
	struct {
		char *mesh_key;
		char *name;
		char *role;

		struct {
			char *wifi_ssid;
			char *wifi_password;
			char *mqtt_key;
		} gateway_config;

	} raw_config;
	struct webserver_post_format post_format[] = {
		{
			.name = "meshKey",
			.required = true,
			.output = &raw_config.mesh_key,
		},
		{
			.name = "nodeType",
			.required = true,
			.output = &raw_config.role,
		},
		{
			.name = "wifiSsid",
			.required = false,
			.output = &raw_config.gateway_config.wifi_ssid
		},
		{
			.name = "wifiPassword",
			.required = false,
			.output = &raw_config.gateway_config.wifi_password
		},
		{
			.name = "mqttKey",
			.required = false,
			.output = &raw_config.gateway_config.mqtt_key
		},
		{
			.name = "nodeName",
			.required = true,
			.output = &raw_config.name
		},
		{ 0 }
	};
	if(webserver_read_post(req, post_format) != ESP_OK) {
		httpd_resp_set_status(req, "400");
		httpd_resp_send(req, "", 0);
		return ESP_FAIL;
	}

	bool ok = true;

	ok = ok && parse_key(&config, raw_config.mesh_key);
	strncpy(
		config.name, raw_config.name,
		sizeof(config.name)
	);
	if(strcmp("node", raw_config.role) == 0) {
		config.role = SPV_CONFIG_ROLE_NODE;

	} else if(strcmp("gateway", raw_config.role) == 0) {
		config.role = SPV_CONFIG_ROLE_GATEWAY;
		if(
			raw_config.gateway_config.mqtt_key &&
			raw_config.gateway_config.wifi_ssid &&
			raw_config.gateway_config.wifi_password
		) {
			strncpy(
				config.gateway_config.mqtt_key, raw_config.gateway_config.mqtt_key,
				sizeof(config.gateway_config.mqtt_key)
			);
			strncpy(
				config.gateway_config.wifi_ssid, raw_config.gateway_config.wifi_ssid,
				sizeof(config.gateway_config.wifi_ssid)
			);
			strncpy(
				config.gateway_config.wifi_password, raw_config.gateway_config.wifi_password,
				sizeof(config.gateway_config.wifi_password)
			);
		} else {
			ok = false;
		}
	} else {
		ok = false;
	}


	free(raw_config.gateway_config.mqtt_key);
	free(raw_config.gateway_config.wifi_ssid);
	free(raw_config.gateway_config.wifi_password);
	free(raw_config.name);

	if(!ok) {
		httpd_resp_set_status(req, "400");
		httpd_resp_send(req, "", 0);
		return ESP_FAIL;
	}

	if(spv_config_write(&config) != ESP_OK) {
		httpd_resp_set_status(req, "500");
		httpd_resp_send(req, "", 0);
		return ESP_FAIL;
	}

	webserver_request_redirect(req, "/?ok");
	ESP_LOGI(TAG, "Configuration saved, restarting...");

	xEventGroupSetBits(ctx->event_group, WEBSERVER_POST_EVENT);

	return ESP_OK;
}
