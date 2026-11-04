#include "global.h"
#include "event_data.h"
#include "money.h"
#include "polished_mom_bank.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "test/test.h"

TEST("Johto savings transfers are atomic at both account limits and preserve seven-digit balances")
{
    u32 wallet = GetMoney(&gSaveBlock1Ptr->money);
    u16 hi = VarGet(VAR_POLISHED_MOM_BALANCE_HI);
    u16 lo = VarGet(VAR_POLISHED_MOM_BALANCE_LO);
    bool32 open = FlagGet(FLAG_POLISHED_MOM_BANK_OPEN);
    FlagSet(FLAG_POLISHED_MOM_BANK_OPEN);
    SetMoney(&gSaveBlock1Ptr->money, 500);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, 1);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, 65530);
    EXPECT_EQ(PolishedMomTransfer(TRUE, 10), POLISHED_BANK_SUCCESS);
    EXPECT_EQ(PolishedMomBalance(), 131076);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), 490);
    EXPECT_EQ(PolishedMomTransfer(TRUE, 491), POLISHED_BANK_FUNDS);
    EXPECT_EQ(PolishedMomTransfer(FALSE, 131077), POLISHED_BANK_FUNDS);
    EXPECT_EQ(PolishedMomTransfer(FALSE, 0), POLISHED_BANK_CANCEL);
    SetMoney(&gSaveBlock1Ptr->money, MAX_MONEY - 9);
    EXPECT_EQ(PolishedMomTransfer(FALSE, 10), POLISHED_BANK_ROOM);
    EXPECT_EQ(PolishedMomBalance(), 131076);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), MAX_MONEY - 9);
    EXPECT_EQ(PolishedMomTransfer(FALSE, 9), POLISHED_BANK_SUCCESS);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), MAX_MONEY);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, POLISHED_MOM_BANK_MAX >> 16);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, (POLISHED_MOM_BANK_MAX - 2) & 0xffff);
    EXPECT_EQ(PolishedMomTransfer(TRUE, 3), POLISHED_BANK_ROOM);
    EXPECT_EQ(PolishedMomBalance(), POLISHED_MOM_BANK_MAX - 2);
    EXPECT_EQ(GetMoney(&gSaveBlock1Ptr->money), MAX_MONEY);
    EXPECT_EQ(PolishedMomTransfer(TRUE, 2), POLISHED_BANK_SUCCESS);
    EXPECT_EQ(PolishedMomBalance(), POLISHED_MOM_BANK_MAX);
    SetMoney(&gSaveBlock1Ptr->money, wallet);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, hi);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, lo);
    if (!open)
        FlagClear(FLAG_POLISHED_MOM_BANK_OPEN);
}

TEST("Johto trainer prizes share a quarter only while saving and the bank has space")
{
    u16 hi = VarGet(VAR_POLISHED_MOM_BALANCE_HI);
    u16 lo = VarGet(VAR_POLISHED_MOM_BALANCE_LO);
    bool32 saving = FlagGet(FLAG_POLISHED_MOM_SAVES_MONEY);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, 0);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, 0);
    FlagClear(FLAG_POLISHED_MOM_SAVES_MONEY);
    EXPECT_EQ(PolishedMomSavePrize(103), 0);
    EXPECT_EQ(PolishedMomBalance(), 0);
    FlagSet(FLAG_POLISHED_MOM_SAVES_MONEY);
    EXPECT_EQ(PolishedMomSavePrize(103), 25);
    EXPECT_EQ(PolishedMomBalance(), 25);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, POLISHED_MOM_BANK_MAX >> 16);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, (POLISHED_MOM_BANK_MAX - 1) & 0xffff);
    EXPECT_EQ(PolishedMomSavePrize(100), 25);
    EXPECT_EQ(PolishedMomBalance(), POLISHED_MOM_BANK_MAX);
    EXPECT_EQ(PolishedMomSavePrize(100), 0);
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, hi);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, lo);
    if (!saving)
        FlagClear(FLAG_POLISHED_MOM_SAVES_MONEY);
}
