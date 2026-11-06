"""Exercise Violet Mart with normal controller input and read-only probes."""
import struct

from test_johto_demo_emulator import A, B, UP, DOWN, LEFT, RIGHT, START
from test_new_bark_emulator import map_data, walk, save_game, continue_game, close_resident_dialogue, talk_to_resident
from test_new_bark_intro_emulator import layout, advance, task_active
from test_cherrygrove_emulator import enter, leave, interact, money, extra_layout
from test_academy_emulator import wait_for, cb


def stock(game):
    return struct.unpack("<11H",game.read(game.sym["PolishedVioletMart_Stock"],22))


def count(game, item):
    key = game.u32(game.u32(game.sym["gSaveBlock2Ptr"])+layout()["encryption_key"]) & 0xffff
    # Bag pockets retain their native eight-byte pointer/capacity records.
    total = 0
    for pocket in range(5):
        record = game.sym["gBagPockets"]+pocket*8
        pointer,capacity = struct.unpack("<IH",game.read(record,6))
        capacity &= 0x3ff
        total += sum(qty^key for ident,qty in struct.iter_unpack("<HH",game.read(pointer,capacity*4)) if ident==item)
    return total


def open_shop(game, mart):
    interact(game,mart,(3,3),LEFT)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(A,180)
    assert task_active(game,"Task_BuyMenu")


def purchase(game,index,item):
    before,cash = count(game,item),money(game)
    price = game.u32(game.sym["gItemsInfo"]+item*extra_layout()["item_size"])
    assert cash >= price,(item,cash,price)
    for _ in range(index):
        game.press(DOWN)
    game.press(A)
    advance(game,lambda:task_active(game,"Task_BuyHowManyDialogueHandleInput"))
    game.press(A)
    advance(game,lambda:task_active(game,"Task_ReturnToItemListAfterItemPurchase"))
    assert count(game,item)==before+1 and money(game)==cash-price,(item,count(game,item),money(game))
    game.press(A)
    assert task_active(game,"Task_BuyMenu")


def close_shop(game):
    game.press(B,180)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(B)
    close_resident_dialogue(game)


def open_bag_item(game,item):
    game.press(START)
    actions = game.read(game.sym["sCurrentStartMenuActions"],9)
    for _ in range(9):
        if game.read(game.sym["sStartMenuCursorPos"],1)[0]==actions.index(2):
            break
        game.press(DOWN)
    game.press(A,160)
    select_bag_item(game,item)


def select_bag_item(game,item,pocket=0):
    for _ in range(5):
        if game.read(game.sym["gBagPosition"]+5,1)[0]==pocket:
            break
        game.press(RIGHT)
    base=game.sym["gBagPockets"]+8*pocket
    ptr,capacity=struct.unpack("<IH",game.read(base,6))
    capacity &= 0x3ff
    index=next(i for i,(ident,qty) in enumerate(struct.iter_unpack("<HH",game.read(ptr,capacity*4))) if ident==item)
    for _ in range(capacity+1):
        cursor,scroll=struct.unpack("<HH",game.read(game.sym["gBagPosition"]+8+2*pocket,2)+game.read(game.sym["gBagPosition"]+18+2*pocket,2))
        if cursor+scroll==index:
            break
        game.press(DOWN if cursor+scroll<index else UP)
    game.press(A)
    game.frames(60)


def check_mart(game,city,checks):
    mart = map_data("PolishedVioletMart")
    game.snapshot("mart-city")
    # Earl can be off-camera while this route crosses the city.
    walk(game,city,(9,18),avoid=((13,16),))
    enter(game,city,mart)
    assert game.position()==(3,7),game.position()
    game.snapshot("mart-inside")
    game.screenshot("violet-mart")
    for actor in (2,3):
        talk_to_resident(game,mart,actor,"violet-mart-resident-"+str(actor))
    interact(game,mart,(3,2),UP,"PolishedVioletMart_MerchandiseShelf_Text")
    close_resident_dialogue(game)
    for index in (0,1):
        game.restore("mart-inside")
        leave(game,mart,city,index)
        assert game.position()==(9,18),game.position()
    checks.append("Violet Mart opens at the source doorway, both exit tiles return correctly, and both customers and shelves are usable")
    items=stock(game)
    for index,item in enumerate(items):
        game.restore("mart-inside")
        open_shop(game,mart)
        purchase(game,index,item)
        game.screenshot("violet-mart-purchase-"+str(index))
        close_shop(game)
        print("PASS: Violet Mart purchase",index,item,flush=True)
    checks.append("All 11 source-stock items can be purchased from the scrolling native shop with exact live-price deductions")
    # Cancel quantity selection, then spend down through the regular shop so
    # a real unaffordable purchase is rejected without losing any inventory.
    game.restore("mart-inside")
    open_shop(game,mart)
    for _ in range(3):
        game.press(DOWN)
    cash,qty=money(game),count(game,items[3])
    game.press(A)
    advance(game,lambda:task_active(game,"Task_BuyHowManyDialogueHandleInput"))
    game.press(B)
    assert task_active(game,"Task_BuyMenu") and money(game)==cash and count(game,items[3])==qty
    price=game.u32(game.sym["gItemsInfo"]+items[3]*extra_layout()["item_size"])
    purchases=cash//price
    for _ in range(purchases):
        purchase(game,0,items[3])
    game.press(A,150)
    assert money(game)==cash-purchases*price and count(game,items[3])==qty+purchases
    assert not task_active(game,"Task_BuyHowManyDialogueHandleInput")
    game.press(A,80)
    assert task_active(game,"Task_BuyMenu")
    close_shop(game)
    save_game(game)
    continue_game(game)
    assert count(game,items[3])==qty+purchases and money(game)==cash-purchases*price
    checks.append("Canceled and unaffordable purchases preserve money and inventory; purchased consumable Escape Ropes survive Save/Continue")
    game.restore("mart-inside")
    open_shop(game,mart)
    purchase(game,10,items[10])
    close_shop(game)
    cash,qty=money(game),count(game,items[10])
    save_game(game)
    continue_game(game)
    assert money(game)==cash and count(game,items[10])==qty
    open_bag_item(game,items[10])
    game.press(A,180)
    wait_for(game,lambda:cb(game,"CB2_MailRead"),"read Flower Mail")
    game.frames(90)
    game.screenshot("violet-flower-mail")
    game.press(B,180)
    wait_for(game,lambda:task_active(game,"Task_BagMenu_HandleInput"),"return from Flower Mail")
    assert count(game,items[10])==qty
    game.press(B,140)
    game.press(B,60)
    checks.append("Flower Mail purchase and money survive Save/Continue; its source-derived stationery opens from the Bag and returns with the item preserved")
    # Sell through the regular Bag. The displayed quote is read back, so the
    # same check supports the engine's configured resale fraction.
    interact(game,mart,(3,3),LEFT)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(DOWN)
    game.press(A,180)
    wait_for(game,lambda:task_active(game,"Task_BagMenu_HandleInput"),"sell Bag")
    select_bag_item(game,items[10])
    wait_for(game,lambda:task_active(game,"Task_CallYesOrNoCallback"),"sale confirmation")
    quote=game.read(game.sym["gStringVar1"],10).split(b"\xff",1)[0]
    value=int("".join(str(ch-0xa1) for ch in quote))
    game.press(A)
    advance(game,lambda:task_active(game,"WaitAfterItemSell"))
    assert count(game,items[10])==qty-1 and money(game)==cash+value
    game.press(A,100)
    game.press(B,180)
    advance(game,lambda:task_active(game,"Task_ShopMenu"))
    game.press(B)
    close_resident_dialogue(game)
    checks.append("Selling Flower Mail removes exactly one item and credits the displayed native resale value")
    game.restore("mart-city")
