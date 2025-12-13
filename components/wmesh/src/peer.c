#include "wmesh/peer.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "Mesh";


void wmesh_peer_new(wmesh_peer_t *peer, const wmesh_address_t address) {
	memcpy(peer->address, address, sizeof(wmesh_address_t));
	peer->sequence = 0;
	peer->dirty = false;
}


wmesh_decrypt_status_t wmesh_peer_decrypt(
	wmesh_peer_t *peer,
	wmesh_encryption_ctx_t *encryption_ctx,

	const uint8_t *ciphertext,
	size_t ciphertext_size,

	uint8_t *plaintext
) {
	wmesh_decrypt_status_t err;
	err = wmesh_decrypt(encryption_ctx, &peer->sequence, ciphertext, ciphertext_size, plaintext);

	if(err == WMESH_DECRYPT_OK) {
		peer->dirty = true;
	}

	return err;
}


wmesh_storage_result_t wmesh_peer_store(
	wmesh_peer_t *peer,
	wmesh_storage_handle_t *storage_handle
) {
	if(!peer->dirty) {
		return WMESH_STORAGE_OK;
	}

	char address_str[sizeof(wmesh_address_t) * 2 + 1] = "";
	sprintf(
		address_str, "%02X%02X%02X%02X%02X%02X",
		peer->address[0], peer->address[1], peer->address[2],
		peer->address[3], peer->address[4], peer->address[5]
	);
	wmesh_storage_result_t err = wmesh_storage_write_ctr(storage_handle, address_str, &peer->sequence);

	if(err == WMESH_STORAGE_OK) {
		peer->dirty = false;
	}

	return err;
}


wmesh_storage_result_t wmesh_peer_load(
	wmesh_peer_t *peer,
	wmesh_storage_handle_t *storage_handle,
	const wmesh_address_t address
) {

	char address_str[sizeof(wmesh_address_t) * 2 + 1] = "";
	sprintf(
		address_str, "%02X%02X%02X%02X%02X%02X",
		address[0], address[1], address[2],
		address[3], address[4], address[5]
	);

	memcpy(peer->address, address, sizeof(wmesh_address_t));
	peer->dirty = false;
	return wmesh_storage_read_ctr(storage_handle, address_str, &peer->sequence);
}


wmesh_storage_result_t wmesh_peer_list_store(
	wmesh_peer_list_t *list,
	wmesh_storage_handle_t *storage_handle
) {
	wmesh_storage_result_t err = WMESH_STORAGE_OK;

	for(size_t i = 0; i < list->peer_count; i++) {
		wmesh_storage_result_t current_err =
			wmesh_peer_store(&list->peers[i], storage_handle);

		if(current_err != WMESH_STORAGE_OK) {
			err = current_err;
		}
	}

	return err;
}


wmesh_peer_t* wmesh_peer_list_get(
	wmesh_peer_list_t *list,
	const wmesh_address_t address
) {
	for(size_t i = 0; i < list->peer_count; i++) {
		wmesh_peer_t *peer = &list->peers[i];

		if(memcmp(peer->address, address, sizeof(wmesh_address_t)) == 0) {
			return peer;
		}
	}

	return NULL;
}


wmesh_peer_t* wmesh_peer_list_get_or_create(
	wmesh_peer_list_t *list,
	const wmesh_address_t address,
	wmesh_storage_handle_t *storage_handle
) {
	wmesh_peer_t *peer_ptr = wmesh_peer_list_get(list, address);
	if(peer_ptr != NULL) {
		return peer_ptr;
	}

	wmesh_peer_t peer;
	wmesh_storage_result_t load_err =
		wmesh_peer_load(
			&peer,
			storage_handle,
			address
		);

	if(load_err == WMESH_STORAGE_ERROR) {
		ESP_LOGE(TAG, "Error loading peer.");
		return NULL;
	}

	if(load_err == WMESH_STORAGE_NOT_FOUND) {
		wmesh_peer_new(&peer, address);
	}

	if(wmesh_peer_list_add(list, peer) != ESP_OK) {
		return NULL;
	}

	return &list->peers[list->peer_count - 1];
}


esp_err_t wmesh_peer_list_add(
	wmesh_peer_list_t *list,
	wmesh_peer_t peer
) {
	if(list->peer_count == CONFIG_WMESH_PEER_LIST_SIZE) {
		return ESP_ERR_NO_MEM;
	}

	memcpy(&list->peers[list->peer_count], &peer, sizeof(peer));
	list->peer_count++;

	return ESP_OK;
}
