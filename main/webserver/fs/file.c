#include "file.h"

#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "FILE";

esp_err_t fs_file_read(const char *path, size_t *size, char **output, bool null_terminate) {
	ESP_LOGI(TAG, "Reading file '%s'", path);
	FILE *file = fopen(path, "r");

	if(file == NULL) {
		ESP_LOGE(TAG, "Error opening file '%s'", path);
		return ESP_FAIL;
	}

	int fseek_result = fseek(file, 0, SEEK_END);
	ssize_t file_size = ftell(file);
	fseek_result |= fseek(file, 0, SEEK_SET);

	if(fseek_result != 0 || file_size < 0) {
		ESP_LOGE(TAG, "Error seeking file '%s'", path);
		fclose(file);
		return ESP_FAIL;
	}

	char *file_buffer = malloc(file_size + null_terminate);
	assert(file_buffer != NULL);

	fread(file_buffer, 1, file_size, file);
	fclose(file);

	if(null_terminate) {
		file_buffer[file_size] = '\0';
	}

	*output = file_buffer;
	*size = file_size + null_terminate;

	ESP_LOGI(TAG, "Read %d bytes from file '%s'", file_size, path);
	return ESP_OK;
}
