"""Violet Mart, its source stock and usable Flower Mail."""
import re
import struct
import tempfile
from pathlib import Path

from convert_player_walking import convert, _read_png_2bit_grayscale, _write_indexed_4bit_png
from polished_route29 import object_at
from polished_route30 import palette

MAPS = {"VioletMart": "VIOLET_MART"}
ITEMS = {"PARALYZEHEAL": "PARALYZE_HEAL", "X_DEFEND": "X_DEFENSE",
         "FLOWER_MAIL": "POLISHED_FLOWER_MAIL", "ESCAPE_ROPE": "POLISHED_ESCAPE_ROPE"}


def stock(files):
    body = re.search(r"^VioletMart:\n(.*?)^\s*db -1", files["data/items/marts.asm"].decode(), re.M | re.S)[1]
    items = re.findall(r"^\s*db ([A-Z_]+)\s*$", body, re.M)
    assert len(items) == int(re.search(r"db (\d+) ; # items", body)[1]) == 11
    return items


def stationery(files, temporary):
    # Data-only audits import MAPS without needing the image build dependency.
    from PIL import Image

    # Original one-bit tiles and RGB555 colors, arranged across the GBA's
    # 30x20 screen, leaving space for all nine native Easy Chat words.
    tiles = [bytes([4] * 64), bytes([4] * 64)]
    for name, count, color in (("flower_mail_border", 8, 1), ("oddish", 4, 3),
                               ("flower_mail_border", 1, 2), ("flower_1", 4, 1), ("flower_2", 4, 2)):
        temporary.write_bytes(files[f"gfx/mail/{name}.png"])
        im = Image.open(temporary).convert("L")
        offsets = [(x, y) for y in range(0, im.height, 8) for x in range(0, im.width, 8)]
        if name == "flower_mail_border" and color == 2:
            offsets = offsets[6:7]
        for tx, ty in offsets[:count]:
            tiles.append(bytes(color if im.getpixel((tx+x,ty+y)) == 0 else 4
                               for y in range(8) for x in range(8)))
    assert len(tiles) == 23
    tilemap = [0] * 1024
    def put(x, y, tile):
        tilemap[y*32+x] = tile
    def stamp(x, y, tile):
        for dy in range(2):
            for dx in range(2):
                put(x+dx,y+dy,tile+dy*2+dx)
    for x in range(1, 29):
        put(x,0,3)
        put(x,19,8)
    for y in range(1,19):
        put(0,y,5)
        put(29,y,6)
    for x,y,tile in ((0,0,2),(29,0,4),(0,19,7),(29,19,9)):
        put(x,y,tile)
    for x in range(2,28):
        put(x,17,14)
    stamp(2,15,10)
    stamp(26,15,10)
    for x,y in ((2,2),(5,3),(20,2),(26,3),(5,15),(26,14)):
        stamp(x,y,15)
    for x,y in ((3,4),(22,3),(24,2),(3,16),(24,15)):
        stamp(x,y,19)
    rows = re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)", files["gfx/mail/mail.pal"].decode())[:4]
    colors = [int(r) | int(g)<<5 | int(b)<<10 for r,g,b in rows]
    graphic = bytes(value for tile in tiles for y in range(8)
                    for x in range(0,8,2) for value in [tile[y*8+x] | tile[y*8+x+1]<<4])
    return {"graphics/mail/polished_flower/tiles.bin": graphic,
            "graphics/mail/polished_flower/map.bin": struct.pack("<1024H", *tilemap),
            "graphics/mail/polished_flower/palette.bin": struct.pack("<16H", *colors, colors[0], *([0]*11))}


def assets(files):
    with tempfile.TemporaryDirectory() as directory:
        src, out = Path(directory)/"src.png", Path(directory)/"out.png"
        src.write_bytes(files["gfx/sprites/granny.png"])
        convert(src, out)
        result = {"graphics/object_events/polished_granny/walking.png": out.read_bytes()}
        src.write_bytes(files["gfx/items/flower_mail.png"])
        width, height, pixels = _read_png_2bit_grayscale(src)
        assert (width,height) == (24,24)
        _write_indexed_4bit_png(out, [[3-p for p in row] for row in pixels])
        result["graphics/items/icons/polished_flower_mail.png"] = out.read_bytes()
        body = files["gfx/items/items.pal"].decode().split("; FLOWER MAIL\n")[1]
        colors = [tuple(map(int, row)) for row in re.findall(r"RGB\s+(\d+),\s*(\d+),\s*(\d+)",body)[:2]]
        result["graphics/items/icon_palettes/polished_flower_mail.pal"] = palette(colors)
        result.update(stationery(files,src))
        return result


def events(script, header, files, text, define):
    prefix = "PolishedVioletMart"
    x,y = map(int,re.search(r"mart_clerk_event\s+(\d+),\s*(\d+), MARTTYPE_STANDARD, MART_VIOLET",script).groups())
    header["object_events"].append(object_at("LOCALID_VIOLET_MART_CLERK", "POLISHED_CLERK", x,y,prefix+"_Clerk"))
    header["object_events"][0]["movement_type"] = "MOVEMENT_TYPE_FACE_RIGHT"
    code = ""
    for raw in re.findall(r"^\s*object_event (.*)$",script,re.M):
        x,y,sprite,move,ry,rx,schedule,color,kind,command,label,flag = [part.strip() for part in raw.split(",")]
        assert (schedule,color,kind,command,flag) == ("-1","0","OBJECTTYPE_COMMAND","jumptextfaceplayer","-1")
        sprite = sprite.removeprefix("SPRITE_")
        event = prefix+"_"+label
        obj = object_at("LOCALID_VIOLET_MART_"+sprite,"POLISHED_"+sprite,int(x),int(y),event)
        obj.update(movement_range_x=int(rx),movement_range_y=int(ry),
                   movement_type={"SPRITEMOVEDATA_WALK_LEFT_RIGHT":"MOVEMENT_TYPE_WANDER_LEFT_AND_RIGHT",
                                  "SPRITEMOVEDATA_SPINRANDOM_SLOW":"MOVEMENT_TYPE_LOOK_AROUND"}[move])
        header["object_events"].append(obj)
        code += f"{event}::\n\tmsgbox {event}_Text, MSGBOX_NPC\n\tend\n\n"
        code += define(event+"_Text",text(script,label))
    code += f"""{prefix}_Clerk::
    lock
    faceplayer
    message gText_HowMayIServeYou
    waitmessage
    pokemart {prefix}_Stock
    msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
    release
    end

    .align 2
{prefix}_Stock:
""" + "".join("    .2byte ITEM_"+ITEMS.get(item,item)+"\n" for item in stock(files)) + "    pokemartlistend\n"
    return "",code,{"stock":stock(files),"item_mapping":ITEMS,"prices":"native Expansion item prices",
                    "mail":"source Flower Mail artwork with native stationery layout and Easy Chat"}
