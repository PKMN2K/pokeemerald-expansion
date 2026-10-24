; Pinned Polished Crystal source extracts for offline semantic auditing.
; Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42
; engine/overworld/player_movement.asm and data/collision/collision_permissions.asm

.CheckSurfPerms:
; Return 0 if moving in water, or 1 if moving onto land.
; Otherwise, return carry.

	ld a, [wTilePermissions]
	ld d, a
	ld a, [wFacingDirection]
	and d
	jr nz, .NotSurfable

	ld a, [wWalkingTileCollision]
	call .CheckSurfable
	jr c, .NotSurfable

	and a
	ret

.NotSurfable:
	scf
	ret

.CheckSurfable:
; Return 0 if tile a is water, or 1 if land.
; Otherwise, return carry.

	call GetTilePermission
; Can walk back onto land from water.
	and a ; cp LAND_TILE
	jr z, .Land
	dec a ; cp WATER_TILE
	jr nz, .Neither

.Water:
	xor a
	ret

.Land:
	ld a, 1
	and a
	ret

.Neither:
	scf
	ret

; Collision permission for the source WATER symbol:
	db WATER_TILE ; COLL_WATER
