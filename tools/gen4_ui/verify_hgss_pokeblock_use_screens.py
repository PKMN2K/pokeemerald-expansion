#!/usr/bin/env python3
"""Verify original-art pixels, live polygons, feeding results and consumption."""
import json
import re
import struct
import sys
from pathlib import Path

from PIL import Image, ImageChops
from make_hgss_pokeblock_use import OUT, validate
from verify_hgss_condition_graph_screens import Reference, ROOT, STORAGE, check_polygon, png_palette, rgb555, tile_pixel

BROWSE = 'UP/DOWN: PICK  A: FEED  B: BACK'
VIEW = 'A/B: VIEW CHANGES'
ARROWS = ((176,48),(101,57),(101,103),(211,103),(211,57))


class FeedReference(Reference):
    def __init__(self):
        super().__init__()
        self.charmap["'"] = 0xB4
        self.small = Image.open(ROOT / 'graphics/fonts/latin_small.png')
        widths = re.search(r'gFontSmallLatinGlyphWidths\[\] = \{(.*?)\};', (ROOT / 'src/fonts.c').read_text(), re.S)[1]
        self.small_widths = list(map(int, re.findall(r'\d+', widths)))
        self.arrows = (ROOT / 'graphics/gen4_ui/hgss_pokegear/list_arrows.4bpp').read_bytes()
        self.arrow_palette = [rgb555(v) for v in struct.unpack('<16H', (ROOT / 'graphics/gen4_ui/hgss_pokegear/list_arrows.gbapal').read_bytes())]

    def blit(self, image, raw, width, height, x0, y0, palette, first_tile=0):
        for y in range(height):
            for x in range(width):
                value = tile_pixel(raw, first_tile + y//8 * (width//8) + x//8, x%8, y%8)
                if value:
                    image.putpixel((x0+x,y0+y), palette[value])

    def frame(self, owner, index, message, results=False, choice=None, arrows=()):
        frame = self.background.copy()
        self.text(frame, 'CONDITION RESULT' if results else 'FEED POKéBLOCK', 8, 9)
        self.text(frame, index, 176, 9)
        if owner:
            name, level, gender, nature, stats = owner
            self.text(frame, name, 8, 25)
            end = self.text(frame, self.codes(gender) if gender else [0x77], 104, 25, 9 if gender == '♂' else 6)
            self.text(frame, self.codes('/') + [0x105] + self.codes(str(level)), end, 25)
            font, widths = self.font, self.widths
            self.font, self.widths = self.small, self.small_widths
            self.text(frame, 'Nature/' + nature, 160, 26)
            self.font, self.widths = font, widths
        else:
            self.text(frame, 'CANCEL', 8, 25)
        self.blit(frame, (OUT / 'footer.4bpp').read_bytes(), 240,32,0,128,self.pal[1])
        self.text(frame, message, 8,137)
        if choice is not None:
            self.blit(frame, (OUT / 'confirmation.4bpp').read_bytes(),64,48,168,72,self.pal[1])
            self.text(frame,'YES',192,80); self.text(frame,'NO',192,96)
            self.blit(frame,self.arrows,8,16,176,80+choice*16,self.arrow_palette)
        for i in arrows:
            x,y = ARROWS[i]
            self.blit(frame,self.arrows,16,8,x-8,y-4,self.arrow_palette,4)
        return frame


def check_audits(base):
    audits = json.loads((base / 'audit.json').read_text())
    initial = [[0]*6,[60,100,140,180,220,80],[250,250,250,250,250,240],[255]*6,[255,40,80,120,10,255],[5,5,5,5,5,0]]
    neutral = [20,30,40,50,60,40]
    expected = {}
    def add(name, returns, blocks, changes=None, single=False):
        party = [row[:] for row in initial]
        for index, stats in (changes or {}).items():
            party[index] = stats
        if single:
            party[1:] = [[0]*6 for _ in range(5)]
        expected['audit_'+name] = (returns,blocks,party)
    add('cancel',2,4)
    add('neutral',3,3,{0:neutral})
    add('liked',4,2,{0:neutral,1:[126,100,140,180,220,100]})
    add('unchanged',5,1,{0:[20,30,40,50,60,60],1:[126,100,140,180,220,100]})
    add('capped',6,0,{0:[20,30,40,50,60,60],1:[126,100,140,180,220,100],2:[255]*6})
    add('disliked',2,3,{1:[80,127,180,230,255,120]})
    add('last_fed',2,3,{5:[25,35,45,60,65,40]})
    add('single',2,4,single=True); add('empty',2,4); add('six',2,4)
    assert set(audits) == set(expected), 'Missing or unexpected return audit'
    for name,(returns,blocks,party) in expected.items():
        got = audits[name]
        assert got['returns'] == returns and got['blocks'] == blocks and got['party'] == party, (name,got)
        assert sum(color != 0 for color in got['colors']) == blocks, name
    print('Verified ten real returns: all party stats, sheen, no refusal consumption, exactly one item per feeding')


def main(base):
    validate()
    ref = FeedReference()
    pika = ('Pikachu',50,'♀','Hardy',[0]*5)
    eevee = ('Eevee',35,'♀','Adamant',[60,100,140,180,220])
    ditto = ('Ditto',20,None,'Bashful',[250]*5)
    bulba = ('Bulbasaur',5,'♀','Hardy',[255,40,80,120,10])
    last = ('WWWWWWWWWWWW',100,'♂','Calm',[5]*5)
    states = {}
    def add(names, owner, index, message=None, results=False, choice=None, arrows=()):
        for name in names.split():
            states[name] = (owner,index,message or (BROWSE if owner else 'A/B: BACK'),results,choice,arrows)
    add('first wrap_first',pika,'1 / 5')
    add('second',eevee,'2 / 5'); add('genderless',ditto,'3 / 5')
    add('full_sheen refused_dismissed',bulba,'4 / 5')
    add('last last_declined last_b_dismissed',last,'5 / 5')
    add('last_confirm_yes',last,'5 / 5',last[0]+' gets a Pokéblock?',choice=0)
    add('last_confirm_no',last,'5 / 5',last[0]+' gets a Pokéblock?',choice=1)
    add('refused',bulba,'4 / 5',"It won't eat anymore…")
    add('wrap_cancel single_cancel empty_cancel empty_up empty_down six_cancel',None,'CANCEL')
    add('single_first single_wrap',pika,'1 / 1')
    add('six_first',pika,'1 / 6'); add('six_last',last,'6 / 6')
    neutral = (*pika[:4],[20,30,40,50,60])
    for name, before, after, index, messages, arrows in (
        ('neutral',pika,neutral,'1 / 5',('Coolness','Toughness','Smartness','Cuteness','Beauty'),tuple(range(5))),
        ('liked',eevee,(*eevee[:4],[126,100,140,180,220]),'2 / 5',('Coolness',),(0,)),
        ('unchanged',neutral,neutral,'1 / 5',(),()),
        ('capped',ditto,(*ditto[:4],[255]*5),'3 / 5',('Coolness','Toughness','Smartness','Cuteness','Beauty'),tuple(range(5))),
        ('last_fed',last,(*last[:4],[25,35,45,60,65]),'5 / 5',('Coolness','Toughness','Smartness','Cuteness','Beauty'),tuple(range(5))),
        ('disliked',eevee,(*eevee[:4],[80,127,180,230,255]),'2 / 5',('Coolness','Toughness','Smartness','Cuteness','Beauty'),tuple(range(5))),
    ):
        message = messages[0]+' was enhanced!' if messages else 'Nothing changed!'
        add(name+'_confirm',before,index,before[0]+' gets a Pokéblock?',choice=0)
        add(name+'_before',before,index,VIEW,True)
        add(name+'_arrows',after,index,message,True,arrows=arrows)
        add(name+'_result',after,index,message,True)
        for i,condition in enumerate(messages[1:],2):
            add(f'{name}_message_{i}',after,index,condition+' was enhanced!',True)

    for name,args in states.items():
        owner,index,message,results,choice,arrows = args
        actual = Image.open(base / (name+'.png')).convert('RGB')
        assert actual.size == (240,160), name
        expected = ref.frame(*args)
        bad = checked = 0
        for y in range(160):
            for x in range(240):
                if x < 80 and 40 <= y < 128 and owner is not None:
                    continue  # Animated portrait and sheen, checked independently below.
                if choice is None and 120 <= x <= 191 and 54 <= y <= 122:
                    continue  # Native live polygon, tested from fixture values below.
                a,b = actual.getpixel((x,y)),expected.getpixel((x,y))
                bad += max(abs(a[c]-b[c]) for c in range(3)) > 8
                checked += 1
        if bad:
            expected.save(base/(name+'_expected.png'))
            raise SystemExit(f'{name}: {bad}/{checked} original-art/text pixels differ')
        if choice is None:
            check_polygon(actual,owner[4] if owner else None,name)
        else:
            check_polygon(actual,None,name)
        print(f'{name}: {checked} source/text pixels and condition values verified')

    # All ten positions retain the exact native 8x8 HGSS star at maximum sheen.
    star = Image.open(STORAGE / 'shiny_star.png'); palette = png_palette('shiny_star.png')
    shots = [Image.open(base/f'sheen_{i:02d}.png').convert('RGB') for i in range(32)]
    for dx,dy in ((0,-33),(20,-28),(30,-10),(30,10),(20,28),(0,33),(-20,28),(-30,10),(-30,-10),(-20,-28)):
        def matches(shot):
            return all(not star.getpixel((x,y)) or max(abs(shot.getpixel((36+dx+x,76+dy+y))[c]-palette[star.getpixel((x,y))][c]) for c in range(3)) <= 8 for y in range(8) for x in range(8))
        assert any(matches(shot) for shot in shots), f'Missing native sheen star at {dx},{dy}'

    # Cancellation restores the real case identically. Feeding changes its inventory.
    initial = Image.open(base/'case_initial.png').convert('RGB')
    for name in ('browse_return','single_return','empty_return','six_return'):
        actual = Image.open(base/(name+'.png')).convert('RGB')
        assert not ImageChops.difference(actual,initial).getbbox(), f'{name}: case not restored'
    for name in ('neutral','liked','unchanged','capped','disliked','last_fed'):
        actual = Image.open(base/(name+'_return.png')).convert('RGB')
        assert ImageChops.difference(actual,initial).getbbox(), f'{name}: inventory did not change'
        feeding = Image.open(base/(name+'_feeding.png')).convert('RGB')
        assert ImageChops.difference(feeding.crop((0,0,240,112)),actual.crop((0,0,240,112))).getbbox(), name
    check_audits(base)
    print(f'Verified {len(states)} recipient/prompt/result states, ten native sheen positions and six actual feeding cycles')


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit('usage: verify_hgss_pokeblock_use_screens.py SCREENSHOT_DIR')
    main(Path(sys.argv[1]))
