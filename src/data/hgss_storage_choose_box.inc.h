/*
 * Intentional transition sentinel for HGSS Choose Box.
 *
 * Verification of the complete 87-member /a/0/1/9 archive plus the HGSS PC
 * overlay call sites found no standalone NCGR+NCLR+NSCR Choose Box background.
 * HGSS builds this interaction dynamically from normal box presentation,
 * runtime text/count windows, and sprite-driven controls.
 *
 * Do not generate this descriptor from an unrelated NSCR or substitute
 * hand-drawn "HGSS-style" artwork. The next integration step must replace the
 * current static DrawHgssChooseBoxGrid path with the verified native
 * composition. Until then, zero data is deliberate and fails closed.
 *
 * Verification record:
 *   graphics/gen4_ui/hgss_storage/verified/choose_box_native.json
 */
static const struct HgssStorageBgAsset sHgssStorageChooseBoxAsset = {0};
