; Excerpt: engine/overworld/wildmons.asm, SetBadgeBaseLevel through AdjustLevelForBadges.
; Rangi42/polishedcrystal@3fa43192379df5c3e7b09a08e4d5d79af4f02f42
SetBadgeBaseLevel:
	ld hl, wBadges
	ld b, wBadgesEnd - wBadges
	call CountSetBits
	ld hl, BadgeBaseLevels
	ld b, 0
	add hl, bc
	ld a, [hl]
	ld [wBadgeBaseLevel], a
	ret

INCLUDE "data/wild/badge_base_levels.asm"

AdjustLevelForBadges:
	cp MAX_LEVEL + 1
	ret c
	sub LEVEL_FROM_BADGES
	ld b, a
	ld a, [wBadgeBaseLevel]
	add b
; cap underflow at level 2
	cp 2
	jr c, .underflow
	cp MAX_LEVEL
	ret c
; cap overflow at level 99
	cp LEVEL_FROM_BADGES
	ld a, MAX_LEVEL - 1
	ret c
; cap overflow at level 2
.underflow
	ld a, 2
	ret

