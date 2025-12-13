#ifndef MAIN_API_HTTP_H_
#define MAIN_API_HTTP_H_

#include <stdbool.h>
#include "esp_err.h"
#include "cJSON.h"

struct api_http_response {
	int code;

	size_t size;
	char *response;
};

/// @brief Performs an HTTP GET operation and returns the result.
/// @param[in] uri_parts `NULL`-terminated array of URI segments.
///			Concatenated when the function is called.
/// @param[out] response Server response. Only populated when return value is `ESP_OK`.
/// @param[in] null_terminate If true, adds a `NUL` character to the end of the response.
/// @param[in] timeout Timeout in milliseconds.
/// @return `ESP_OK` when successful.
///
/// @code{c}
///	#include "api/http.h"
///	const char *API_KEY = "123ABC";
///	const char *uri[] = {
///		"http://myservice.com/",
///		"api/v1/",
///		API_KEY,
///		NULL
///	};
/// struct api_http_response response;
///
///	if(api_http_get(uri, &response, true, -1) != ESP_OK) {
///		puts("HTTP GET error");
///		return;
/// }
///
///	puts("HTTP response:");
///	printf("Size: %d\n", response.size);
///	printf("Response: %s\n", response.response);
///
///	api_http_response_free(&response);
/// @endcode
esp_err_t api_http_get(
	const char *uri_parts[],
	struct api_http_response *response,
	bool null_terminate,
	int timeout
);

/// @brief Performs an HTTP POST operation and returns the result.
/// @param[in] uri_parts `NULL`-terminated array of URI segments.
///			Concatenated when the function is called.
/// @param[in] post_data Data for POST body.
///	@param[in] post_data_size Size of `post_data`, without including a NUL byte.
///	@param[in] post_content_type Value for `Content-Type` header.
/// @param[out] response Server response. Only populated when return value is `ESP_OK`.
/// @param[in] null_terminate If true, adds a `NUL` character to the end of the response.
/// @return `ESP_OK` when successful.
///
/// @code{c}
///	#include "api/http.h"
///	const char *API_KEY = "123ABC";
///	const char *content_type = "application/json";
///	const char *uri[] = {
///		"http://myservice.com/",
///		"api/v1/",
///		API_KEY,
///		NULL
///	};
///	const char *post_data = "{'temperature': 33}"
/// struct api_http_response response;
///
///	if(api_http_post(uri, post_data, strlen(post_data), content_type, &response, true) != ESP_OK) {
///		puts("HTTP GET error");
///		return;
/// }
///
///	puts("HTTP response:");
///	printf("Size: %d\n", response.size);
///	printf("Response: %s\n", response.response);
///
///	api_http_response_free(&response);
/// @endcode
esp_err_t api_http_post(
	const char *uri_parts[],

	const char *post_data,
	size_t post_data_size,
	const char *post_content_type,

	struct api_http_response *response,
	bool null_terminate
);

/// @brief Performs an HTTP POST operation and returns the result.
/// @param[in] uri_parts `NULL`-terminated array of URI segments.
///			Concatenated when the function is called.
/// @param[in] post_json JSON struct for POST body.
/// @param[out] response Server response. Only populated when return value is `ESP_OK`.
/// @param[in] null_terminate If true, adds a `NUL` character to the end of the response.
/// @return `ESP_OK` when successful.
esp_err_t api_http_post_json(
	const char *uri_parts[],

	cJSON *post_json,

	struct api_http_response *response,
	bool null_terminate
);

/// @brief Frees a `struct api_http_response`.
void api_http_response_free(struct api_http_response *response);


#endif
