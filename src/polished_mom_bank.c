#include "global.h"
#include "event_data.h"
#include "main.h"
#include "menu.h"
#include "money.h"
#include "polished_mom_bank.h"
#include "rtc.h"
#include "script.h"
#include "script_menu.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/flags.h"
#include "constants/songs.h"
#include "constants/vars.h"

u32 PolishedMomBalance(void)
{
    return ((u32)VarGet(VAR_POLISHED_MOM_BALANCE_HI) << 16) | VarGet(VAR_POLISHED_MOM_BALANCE_LO);
}

static void SetBalance(u32 balance)
{
    VarSet(VAR_POLISHED_MOM_BALANCE_HI, balance >> 16);
    VarSet(VAR_POLISHED_MOM_BALANCE_LO, balance & 0xffff);
}

u32 PolishedMomSavePrize(u32 prize)
{
    if (!FlagGet(FLAG_POLISHED_MOM_SAVES_MONEY))
        return 0;
    u32 balance = PolishedMomBalance();
    if (balance >= POLISHED_MOM_BANK_MAX)
        return 0;
    // Crystal sends one of four prize shares and saturates each account separately.
    u32 share = prize / 4;
    SetBalance(balance + min(share, POLISHED_MOM_BANK_MAX - balance));
    return share;
}

enum PolishedBankResult PolishedMomTransfer(bool32 deposit, u32 amount)
{
    u32 bank = PolishedMomBalance();
    u32 wallet = GetMoney(&gSaveBlock1Ptr->money);
    if (!FlagGet(FLAG_POLISHED_MOM_BANK_OPEN) || amount == 0)
        return POLISHED_BANK_CANCEL;
    if (amount > (deposit ? wallet : bank))
        return POLISHED_BANK_FUNDS;
    if (amount > (deposit ? POLISHED_MOM_BANK_MAX - bank : MAX_MONEY - wallet))
        return POLISHED_BANK_ROOM;
    SetBalance(deposit ? bank + amount : bank - amount);
    SetMoney(&gSaveBlock1Ptr->money, deposit ? wallet - amount : wallet + amount);
    return POLISHED_BANK_SUCCESS;
}

bool32 PolishedMomAdjustClock(void)
{
    bool32 summer = FlagGet(FLAG_POLISHED_DAYLIGHT_SAVING);
    RtcCalcLocalTime();
    if ((summer && gLocalTime.hours == 0) || (!summer && gLocalTime.hours == 23))
        return FALSE;
    RtcCalcLocalTimeOffset(gLocalTime.days, gLocalTime.hours + (summer ? -1 : 1), gLocalTime.minutes, gLocalTime.seconds);
    if (summer)
        FlagClear(FLAG_POLISHED_DAYLIGHT_SAVING);
    else
        FlagSet(FLAG_POLISHED_DAYLIGHT_SAVING);
    RtcCalcLocalTime();
    return TRUE;
}

enum { BANK_WINDOW, BANK_DIGIT, BANK_DEPOSIT, BANK_AMOUNT, BANK_AMOUNT_HI, BANK_DELAY };
static const u32 sPlaceValues[] = {1000000, 100000, 10000, 1000, 100, 10, 1};

static void DrawTransfer(u8 taskId)
{
    const s16 *data = gTasks[taskId].data;
    u8 window = data[BANK_WINDOW];
    u8 digits[8];
    const u8 *const labels[] = {COMPOUND_STRING("Saved"), COMPOUND_STRING("Held"),
        data[BANK_DEPOSIT] ? COMPOUND_STRING("Save") : COMPOUND_STRING("Get")};
    const u32 values[] = {PolishedMomBalance(), GetMoney(&gSaveBlock1Ptr->money), GetWordTaskArg(taskId, BANK_AMOUNT)};
    FillWindowPixelBuffer(window, PIXEL_FILL(1));
    for (u32 i = 0; i < ARRAY_COUNT(values); i++)
    {
        AddTextPrinterParameterized(window, FONT_NORMAL, labels[i], 2, 2 + i * 17, TEXT_SKIP_DRAW, NULL);
        ConvertIntToDecimalStringN(digits, values[i], i == 2 ? STR_CONV_MODE_LEADING_ZEROS : STR_CONV_MODE_RIGHT_ALIGN, 7);
        // Use fixed cells both for the digits and for the selector beneath them.
        for (u32 j = 0; j < ARRAY_COUNT(sPlaceValues); j++)
        {
            u8 character[] = {digits[j], EOS};
            AddTextPrinterParameterized(window, FONT_NORMAL, character, 70 + j * 8, 2 + i * 17, TEXT_SKIP_DRAW, NULL);
        }
    }
    FillWindowPixelRect(window, TEXT_COLOR_DARK_GRAY, 70 + data[BANK_DIGIT] * 8, 50, 6, 2);
    AddTextPrinterParameterized(window, FONT_NORMAL, COMPOUND_STRING("A: OK   B: Cancel"), 2, 64, TEXT_SKIP_DRAW, NULL);
    CopyWindowToVram(window, COPYWIN_FULL);
}

static void Task_PolishedMomTransfer(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    if (data[BANK_DELAY] && --data[BANK_DELAY])
        return;
    u32 amount = GetWordTaskArg(taskId, BANK_AMOUNT);
    if (JOY_NEW(A_BUTTON | B_BUTTON))
    {
        gSpecialVar_Result = JOY_NEW(A_BUTTON)
            ? PolishedMomTransfer(data[BANK_DEPOSIT], amount) : POLISHED_BANK_CANCEL;
        ClearToTransparentAndRemoveWindow(data[BANK_WINDOW]);
        DestroyTask(taskId);
        ScriptContext_Enable();
        return;
    }
    u32 place = sPlaceValues[data[BANK_DIGIT]];
    if (JOY_REPEAT(DPAD_LEFT) && data[BANK_DIGIT] > 0)
        data[BANK_DIGIT]--;
    else if (JOY_REPEAT(DPAD_RIGHT) && data[BANK_DIGIT] < ARRAY_COUNT(sPlaceValues) - 1)
        data[BANK_DIGIT]++;
    else if (JOY_REPEAT(DPAD_UP))
        amount = min(amount + place, POLISHED_MOM_BANK_MAX);
    else if (JOY_REPEAT(DPAD_DOWN))
        amount -= min(amount, place);
    else
        return;
    SetWordTaskArg(taskId, BANK_AMOUNT, amount);
    PlaySE(SE_SELECT);
    DrawTransfer(taskId);
}

bool32 PolishedMomOpenTransfer(void)
{
    u8 task = CreateTask(Task_PolishedMomTransfer, 80);
    if (gTasks[task].func != Task_PolishedMomTransfer)
        return FALSE;
    const struct WindowTemplate window = {0, 2, 2, 18, 10, 15, 100};
    s16 *data = gTasks[task].data;
    data[BANK_WINDOW] = AddWindow(&window);
    if (data[BANK_WINDOW] == WINDOW_NONE)
    {
        DestroyTask(task);
        return FALSE;
    }
    PutWindowTilemap(data[BANK_WINDOW]);
    data[BANK_DEPOSIT] = gSpecialVar_0x8004;
    data[BANK_DIGIT] = ARRAY_COUNT(sPlaceValues) - 1;
    data[BANK_DELAY] = 10;
    SetStandardWindowBorderStyle(data[BANK_WINDOW], FALSE);
    DrawTransfer(task);
    return TRUE;
}
