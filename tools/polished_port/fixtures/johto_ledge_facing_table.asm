; Verbatim source facing permissions, .TryJump/.ledge_table in
; Rangi42/polishedcrystal @ 3fa43192379df5c3e7b09a08e4d5d79af4f02f42
; engine/overworld/player_movement.asm (first 8 entries: right, left,
; up, down, down-right, down-left, up-right, up-left).
.ledge_table:
    db FACE_RIGHT
    db FACE_LEFT
    db FACE_UP
    db FACE_DOWN
    db FACE_RIGHT | FACE_DOWN
    db FACE_DOWN | FACE_LEFT
    db FACE_UP | FACE_RIGHT
    db FACE_UP | FACE_LEFT
