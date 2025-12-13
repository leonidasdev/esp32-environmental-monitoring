#include "webserver/request.h"

#include "esp_log.h"

static const char *TAG = "WEBSERVER REQUEST";

void webserver_request_redirect(httpd_req_t *req, const char *path) {
	httpd_resp_set_status(req, "302");
	httpd_resp_set_hdr(req, "Location", path);
	httpd_resp_send(req, "", HTTPD_RESP_USE_STRLEN);
}

esp_err_t webserver_request_read(
		httpd_req_t *req,
		size_t *size, char **output,
		bool null_terminate
	)
{
	if(req->content_len == 0) {
		*size = 0;
		*output = NULL;
		return ESP_OK;
	}

	char *buffer = malloc(req->content_len + null_terminate);
	if(buffer == NULL) {
		return ESP_ERR_HTTPD_ALLOC_MEM;
	}

	size_t remaining_bytes = req->content_len;
	while(remaining_bytes > 0) {
		ssize_t read_bytes;
		if((read_bytes = httpd_req_recv(req, buffer, remaining_bytes)) <= 0) {
			ESP_LOGE(TAG, "Timed out while reading request");
			free(buffer);
			return ESP_FAIL;
		}
		remaining_bytes -= read_bytes;
	}

	if(null_terminate) {
		buffer[req->content_len] = '\0';
	}

	*size = req->content_len + null_terminate;
	*output = buffer;

	return ESP_OK;
}

esp_err_t webserver_read_post(
		httpd_req_t *req,
		struct webserver_post_format *format
	)
{
	size_t buffer_size;
	char *buffer;

	if(webserver_request_read(req, &buffer_size, &buffer, true) != ESP_OK) {
		return ESP_FAIL;
	}

	// Set all pointers to NULL.
	for(
		struct webserver_post_format *current_format = format;
		current_format->name != NULL;
		current_format++
	) {
		*current_format->output = NULL;
	}

	char *line_start = buffer;
	char *line_end;

	char *tag_start, *tag_end;
	char *content_start, *content_end;

	while(*line_start != '\0') {
		line_end = line_start + strcspn(line_start, "\r\n");

		tag_start = line_start;
		tag_end = strchr(line_start, '=');

		// Current line is empty.
		if(tag_end == NULL || tag_end > line_end) {
			line_start = line_end + strspn(line_end, "\r\n");
			continue;
		}

		*tag_end = '\0';

		// Check if format contains the current tag.
		struct webserver_post_format *current_format = format;
		while(
			current_format->name != NULL &&
			strcmp(current_format->name, tag_start) != 0
		) {
			current_format++;
		}

		// Format list does not contain the found tag.
		if(current_format->name == NULL) {
			line_start = line_end + strspn(line_end, "\r\n");
			continue;
		}

		content_start = tag_end + 1;
		content_end = line_end - 1;

		// Empty content:
		//	tag=\n
		if(content_start >= content_end) {
			char *content_buffer = malloc(sizeof(""));
			assert(content_buffer);
			*content_buffer = '\0';
			*current_format->output = content_buffer;
		} else {
			size_t buffer_size = content_end - content_start + 2;
			char *content_buffer = malloc(buffer_size);
			content_buffer[buffer_size - 1] = '\0';
			memcpy(content_buffer, content_start, buffer_size - 1);
			*current_format->output = content_buffer;
		}

		line_start = line_end + strspn(line_end, "\r\n");
	}

	// Check if all required fields were processed.
	bool all_processed = true;
	for(
		struct webserver_post_format *current_format = format;
		current_format->name != NULL && all_processed;
		current_format++
	) {
		if(current_format->required) {
			all_processed = *current_format->output != NULL;
		}
	}

	// Not all fields were found.
	if(!all_processed) {
		for(
			struct webserver_post_format *current_format = format;
			current_format->name != NULL;
			current_format++
		) {
			if(*current_format->output != NULL) {
				free(*current_format->output);
			}
		}
		return ESP_FAIL;
	}

	return ESP_OK;
}
