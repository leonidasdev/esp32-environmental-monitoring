#include "persistence.h"

#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include <assert.h>
#include <stdio.h>
#include "file.h"

static const char *TAG = "FILESYSTEM";

void fat32_mount(const char *mountpoint, const char *partition) {
	const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 8,
        .format_if_mount_failed = false,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .use_one_fat = false,
    };

    wl_handle_t s_wl_handle;
    ESP_ERROR_CHECK(esp_vfs_fat_spiflash_mount_rw_wl(mountpoint, partition, &mount_config, &s_wl_handle));
    ESP_LOGI(TAG, "Mounted FAT32 `%s' on `%s'", partition, mountpoint);
}
