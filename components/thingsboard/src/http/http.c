#include "http.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#define MAX_HTTP_OUTPUT_BUFFER 2048

static char* uri_concat(const char *uri_parts[]) {
	size_t required_size = 0;
	for(const char **part = uri_parts; *part != NULL; part++) {
		required_size += strlen(*part);
	}

	char *buffer = malloc(required_size + 1);
	assert(buffer);
	buffer[0] = '\0';

	for(const char **part = uri_parts; *part != NULL; part++) {
		strcat(buffer, *part);
	}

	return buffer;
}

static esp_err_t http_client_event_handler(esp_http_client_event_handle_t event) {
	struct api_http_response *response = event->user_data;

	switch(event->event_id) {
		case HTTP_EVENT_ERROR:
			response->response = realloc(response->response, 0);
			return ESP_FAIL;

		case HTTP_EVENT_ON_DATA:

			size_t required_size = event->data_len + response->size;
			if(required_size == 0) {
				return ESP_OK;
			}

			char *new_buffer = realloc(response->response, required_size);

			if(new_buffer == NULL) {
				response->response = realloc(response->response, 0);
				return ESP_FAIL;
			}

			response->response = new_buffer;

			memcpy(response->response + response->size, event->data, event->data_len);
			response->size = required_size;

			return ESP_OK;

		default:
			return ESP_OK;
	}
}


static esp_http_client_handle_t http_client_create(
	const char *uri,
	esp_http_client_method_t method,
	struct api_http_response *response,
	int timeout
) {
	response->response = NULL;
	response->size = 0;

	esp_http_client_config_t http_client_config = {
		.url = uri,
		.method = method,
		.user_data = response,
		.event_handler = http_client_event_handler,
		.crt_bundle_attach = esp_crt_bundle_attach,
		.timeout_ms = timeout
	};

	return esp_http_client_init(&http_client_config);
}


esp_err_t api_http_get(
	const char *uri_parts[],
	struct api_http_response *response,
	bool null_terminate,
	int timeout_ms
) {
	char *uri = uri_concat(uri_parts);
	esp_http_client_handle_t http_client = http_client_create(
		uri,
		HTTP_METHOD_GET,
		response,
		timeout_ms
	);

	if(esp_http_client_perform(http_client) != ESP_OK) {
		esp_http_client_cleanup(http_client);
		free(uri);
		return ESP_FAIL;
	}
	free(uri);

	if(null_terminate) {
		response->response = realloc(response->response, response->size + 1);
		response->size++;
		assert(response->response);
		response->response[response->size - 1] = '\0';
	}

	response->code = esp_http_client_get_status_code(http_client);
	esp_http_client_cleanup(http_client);

	return ESP_OK;
}


esp_err_t api_http_post(
	const char *uri_parts[],

	const char *post_data,
	size_t post_data_size,
	const char *post_content_type,

	struct api_http_response *response,
	bool null_terminate
) {
	char *uri = uri_concat(uri_parts);
	esp_http_client_handle_t http_client = http_client_create(
		uri,
		HTTP_METHOD_POST,
		response,
		0
	);

	esp_http_client_set_post_field(http_client, post_data, post_data_size);
	esp_http_client_set_header(http_client, "Content-Type", post_content_type);

	if(esp_http_client_perform(http_client) != ESP_OK) {
		free(uri);
		return ESP_FAIL;
	}
	free(uri);

	if(null_terminate) {
		response->response = realloc(response->response, response->size + 1);
		response->size++;
		assert(response->response);
		response->response[response->size - 1] = '\0';
	}

	response->code = esp_http_client_get_status_code(http_client);
	esp_http_client_cleanup(http_client);

	return ESP_OK;
}

esp_err_t api_http_post_json(
	const char *uri_parts[],

	cJSON *post_json,

	struct api_http_response *response,
	bool null_terminate
) {
	char *json = cJSON_Print(post_json);
	assert(json);
	esp_err_t error = api_http_post(uri_parts, json, strlen(json), "application/json", response, null_terminate);
	free(json);
	return error;
}

void api_http_response_free(struct api_http_response *response) {
	free(response->response);
}
