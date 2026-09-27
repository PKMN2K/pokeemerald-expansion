/*
 * Placeholder for the verified HGSS party-panel asset.
 *
 * Generate the real include with:
 *   python tools/gen4_ui/emit_hgss_storage_c.py \
 *     graphics/gen4_ui/hgss_storage/packed/party_panel.json \
 *     --symbol HgssStoragePartyPanel \
 *     --require-role party_panel \
 *     --output src/data/hgss_storage_party_panel.inc.h
 *
 * Until the user's HGSS /a/0/1/9 members have been visually verified and
 * packed, this zero descriptor keeps the temporary compatibility fallback.
 */
static const struct HgssStorageBgAsset sHgssStoragePartyPanelAsset = {0};
