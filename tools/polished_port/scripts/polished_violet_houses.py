"""Violet's nickname house and Kyle's original Poliwag-for-Voltorb trade."""
import re
import tempfile
from pathlib import Path

from convert_player_walking import convert
from polished_route29 import object_at

MAPS = {"VioletNicknameSpeechHouse": "VIOLET_NICKNAME_SPEECH_HOUSE",
        "VioletOnixTradeHouse": "VIOLET_ONIX_TRADE_HOUSE"}


def trade_data(files):
    body = files["data/events/npc_trades.asm"].decode().split("; NPC_TRADE_KYLE in Violet City\n")[1].split("; NPC_TRADE_TIM")[0]
    assert re.search(r"dp POLIWAG, NO_FORM\s*; wants", body)
    assert re.search(r"dp VOLTORB, MALE\s*; gives", body)
    assert "TRADE_DIALOGSET_COLLECTOR" in body
    assert "$EE, $EE, $EE, HIDDEN_ABILITY | NAT_SPE_UP_DEF_DOWN" in body
    assert "PREMIER_BALL, PERSIM_BERRY" in body
    assert re.search(r'rawchar "Mimic@+"', body) and re.search(r'rawchar "Kyle@+"', body)
    assert re.search(r"dw 48926", body)
    # Preserve the source stat contribution: 2 * DV 14 = IV 28, as for Lyra.
    # The source MALE form bit is zero; Voltorb remains a genderless species.
    return '''// Generated from the pinned NPC_TRADE_KYLE entry.
    [INGAME_TRADE_POLISHED_KYLE] =
    {
        .nickname = _("Mimic"),
        .species = SPECIES_VOLTORB,
        .ivs = {28, 28, 28, 28, 28, 28},
        .abilityNum = 2,
        .otId = 48926,
        .personality = NATURE_HASTY,
        .heldItem = ITEM_PERSIM_BERRY,
        .mailNum = -1,
        .otName = _("Kyle"),
        .otGender = MALE,
        .requestedSpecies = SPECIES_POLIWAG,
        .ball = ITEM_PREMIER_BALL,
    },
'''


def assets(files):
    result = {"src/data/polished_kyle_trade.h": trade_data(files).encode()}
    with tempfile.TemporaryDirectory() as directory:
        src, out = Path(directory)/"source.png", Path(directory)/"walking.png"
        for sprite in ("twin", "eevee"):
            src.write_bytes(files[f"gfx/sprites/{sprite}.png"])
            convert(src, out)
            result[f"graphics/object_events/polished_{sprite}/walking.png"] = out.read_bytes()
    return result


def events(name, script, header, files, text, define):
    prefix = "Polished" + name
    if name == "VioletNicknameSpeechHouse":
        obj = object_at("LOCALID_VIOLET_STRAWBERRY", "POLISHED_EEVEE", 5, 2, prefix+"_Eevee")
        obj.update(movement_type="MOVEMENT_TYPE_WANDER_AROUND", movement_range_x=1, movement_range_y=1)
        header["object_events"].insert(2,obj)
        code = f'''{prefix}_Eevee::
    lock
    faceplayer
    playmoncry SPECIES_EEVEE, CRY_MODE_NORMAL
    msgbox {prefix}_EeveeText, MSGBOX_DEFAULT
    waitmoncry
    release
    end

'''
        code += define(prefix+"_EeveeText", text(script, "VioletNicknameSpeechHouseBirdText"))
        return code, {"cry": "EEVEE", "scheduled_resident": "GRAMPS, 17:00 through 04:59"}

    fields = re.search(r"object_event\s+(\d+),\s*(\d+), SPRITE_SCHOOLBOY, SPRITEMOVEDATA_WALK_UP_DOWN, (\d+), (\d+), -1, PAL_NPC_RED, OBJECTTYPE_COMMAND, trade, NPC_TRADE_KYLE, -1", script)
    assert fields
    x,y,ry,rx = map(int,fields.groups())
    obj = object_at("LOCALID_VIOLET_KYLE", "POLISHED_SCHOOLBOY_RED", x,y,prefix+"_Kyle")
    obj.update(movement_type="MOVEMENT_TYPE_WANDER_UP_AND_DOWN", movement_range_x=rx, movement_range_y=ry)
    header["object_events"].append(obj)
    code = f'''{prefix}_Kyle::
    lock
    faceplayer
    goto_if_set FLAG_POLISHED_KYLE_TRADE, {prefix}_After
    ingame_trade INGAME_TRADE_POLISHED_KYLE, {prefix}_IntroText, {prefix}_Cancel, {prefix}_Wrong, {prefix}_CompleteText
    setflag FLAG_POLISHED_KYLE_TRADE
    release
    end

{prefix}_Cancel::
    msgbox {prefix}_CancelText, MSGBOX_DEFAULT
    release
    end

{prefix}_Wrong::
    bufferspeciesname STR_VAR_1, VAR_0x8009
    msgbox {prefix}_WrongText, MSGBOX_DEFAULT
    release
    end

{prefix}_After::
    bufferspeciesname STR_VAR_2, SPECIES_VOLTORB
    msgbox {prefix}_AfterText, MSGBOX_DEFAULT
    release
    end

'''
    common = files["data/text/common.asm"].decode()
    for i in (1,2):
        common = common.replace(f"text_ram wStringBuffer{i}", 'text "{STR_VAR_'+str(i)+'}"')
    for kind in ("Intro", "Cancel", "Wrong", "Complete", "After"):
        code += define(prefix+"_"+kind+"Text",text(common,"_NPCTrade"+kind+"Text1"))
    return code, {"trade": "NPC_TRADE_KYLE", "wanted": "POLIWAG", "given": "VOLTORB",
                  "nickname": "Mimic", "ot": "Kyle", "id": 48926,
                  "ball": "PREMIER_BALL", "item": "PERSIM_BERRY", "ability": "AFTERMATH",
                  "nature": "HASTY", "ivs": [28]*6, "level": "same as offered Pokemon",
                  "native_adaptations": ["PC or party selection", "Expansion moves, stats and trade animation"]}
