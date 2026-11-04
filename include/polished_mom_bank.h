#ifndef GUARD_POLISHED_MOM_BANK_H
#define GUARD_POLISHED_MOM_BANK_H

#define POLISHED_MOM_BANK_MAX 9999999u
enum PolishedBankResult {
    POLISHED_BANK_CANCEL,
    POLISHED_BANK_SUCCESS,
    POLISHED_BANK_FUNDS,
    POLISHED_BANK_ROOM,
};

u32 PolishedMomBalance(void);
u32 PolishedMomSavePrize(u32 prize);
enum PolishedBankResult PolishedMomTransfer(bool32 deposit, u32 amount);
bool32 PolishedMomOpenTransfer(void);
bool32 PolishedMomAdjustClock(void);

#endif
