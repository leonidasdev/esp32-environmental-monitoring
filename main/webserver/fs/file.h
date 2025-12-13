#ifndef MAIN_FS_FILE_H_
#define MAIN_FS_FILE_H_

#include <stdbool.h>
#include "esp_err.h"

esp_err_t fs_file_read(const char *path, size_t *size, char **output, bool null_terminate);

#endif
