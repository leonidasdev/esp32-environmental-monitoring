#ifndef WMESH_PEER_H_
#define WMESH_PEER_H_

#include "esp_now.h"
#include "wmesh/common.h"
#include "wmesh/encryption.h"
#include "wmesh/storage.h"

/// @brief Stores a peer's data.
typedef struct {

	/// @brief Peer's address.
	wmesh_address_t address;

	/// @brief Last known sequence number.
	///
	/// Any received messages with a sequence number lower than this value are
	/// discarded.
	///
	/// Automatically increased when a message is received.
	wmesh_encryption_ctr_t sequence;

	/// @brief Used to signal whether this peer's data has changed. Set when
	/// the peer's sequence number is updated.
	bool dirty:1;

} wmesh_peer_t;


/// @brief Creates a new peer.
///
/// @param[out] peer Pointer to peer struct.
/// @param[in] address Peer address.
void wmesh_peer_new(wmesh_peer_t *peer, const wmesh_address_t address);


/// @brief Same as `wmesh_decrypt_aad`. Decrypts and verifies the given
/// ciphertext.
///
/// @param[inout] peer Peer info. Sequence number is automatically updated.
/// @param[in] encryption_ctx Encryption context.
/// @param[in] ciphertext Input ciphertext.
/// @param[in] ciphertext_size Ciphertext size in bytes.
/// @param[out] plaintext Decrypted data. Use `WMESH_PLAINTEXT_LENGTH` to get
/// the minimum length for the buffer.
///
/// @return See `wmesh_decrypt_aad`.
wmesh_decrypt_status_t wmesh_peer_decrypt(
	wmesh_peer_t *peer,
	wmesh_encryption_ctx_t *encryption_ctx,

	const uint8_t *ciphertext,
	size_t ciphertext_size,

	uint8_t *plaintext
);


/// @brief Saves peer info to persistent storage. Allows saving a peer's
/// sequence number between reboots, preventing replay attacks.
///
/// @param[in] peer Peer data.
/// @param[in] storage_handle Storage handle.
///
/// @return See `wmesh_peer_store_result_t`.
///
/// - `WMESH_STORAGE_OK`: Data stored successfully.
///
/// - `WMESH_STORAGE_ERROR`: Error.
wmesh_storage_result_t wmesh_peer_store(
	wmesh_peer_t *peer,
	wmesh_storage_handle_t *storage_handle
);


/// @brief Tries to restore a peer's data.
///
/// @param[out] peer Loaded peer data. Only valid when `WMESH_PEER_LOAD_OK` is
/// returned.
/// @param[in] storage_handle Storage handle.
/// @param[in] address Peer address.
///
/// @return See `wmesh_peer_load_result_t`.
///
/// - `WMESH_STORAGE_OK`: Data loaded successfully.
///
/// - `WMESH_STORAGE_NOT_FOUND`: Unknown peer.
///
/// - `WMESH_STORAGE_ERROR`: Error.
wmesh_storage_result_t wmesh_peer_load(
	wmesh_peer_t *peer,
	wmesh_storage_handle_t *storage_handle,
	const wmesh_address_t address
);


/// @brief Cotains a list of peers.
typedef struct {

	/// @brief Number of stored peers.
	size_t peer_count;

	/// @brief
	wmesh_peer_t peers[CONFIG_WMESH_PEER_LIST_SIZE];
} wmesh_peer_list_t;


/// @brief Stores information of every peer.
///
/// @param[in] list List of peers to save.
/// @param[in] storage_handle Storage handle.
wmesh_storage_result_t wmesh_peer_list_store(
	wmesh_peer_list_t *list,
	wmesh_storage_handle_t *storage_handle
);


/// @brief Searches for and returns a peer contained in the list.
///
/// @param[in] list Peer list.
/// @param[in] address Address to search.
///
/// @return Pointer inside the list to the peer, or `NULL` if not found.
wmesh_peer_t* wmesh_peer_list_get(
	wmesh_peer_list_t *list,
	const wmesh_address_t address
);


/// @brief Searches for a peer in the list. If not found, creates one.
///
/// @param[in] list Peer list.
/// @param[in] address Address to search.
/// @param[in] storage_handle Storage handle.
///
/// @return Pointer to peer, or `NULL` in case of an error.
wmesh_peer_t* wmesh_peer_list_get_or_create(
	wmesh_peer_list_t *list,
	const wmesh_address_t address,
	wmesh_storage_handle_t *storage_handle
);


/// @brief Adds a peer to the list.
///
/// @param[in] list Peer list.
/// @param peer Peer to add to the list. Owned by the list.
///
/// @return `ESP_OK` or `ESP_ERR_NO_MEM` if the list is full.
esp_err_t wmesh_peer_list_add(
	wmesh_peer_list_t *list,
	wmesh_peer_t peer
);

#endif
