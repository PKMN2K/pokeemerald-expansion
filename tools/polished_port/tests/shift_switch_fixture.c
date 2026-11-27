// Production-linked fixture: construct the parties/options, then use the real
// battle, Yes/No and party controllers. The observer supplies only button input.
#include "global.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "event_data.h"
#include "load_save.h"
#include "main.h"
#include "new_game.h"
#include "option_menu.h"
#include "palette.h"
#include "party_menu.h"
#include "pokemon.h"
#include "save.h"
#include "constants/battle_script_commands.h"
#include "constants/characters.h"
#include "constants/flags.h"
#include "constants/opponents.h"

#define SHIFT_PROBE_TRAP 0
#define SHIFT_PROBE_RESERVE 0
#define SHIFT_PROBE_SET_MODE 0
#define SHIFT_PROBE_WILD_FAINT 0
#define SHIFT_PROBE_OPTIONS 0

const volatile u32 gShiftSwitchProbeLayout[] = {
    sizeof(struct BattlePokemon),
    offsetof(struct BattlePokemon, species),
    offsetof(struct BattlePokemon, hp),
    offsetof(struct BattlePokemon, maxHP),
    offsetof(struct BattlePokemon, statStages),
    offsetof(struct BattlePokemon, volatiles),
    sizeof(struct Volatiles),
    sizeof(struct Pokemon),
    offsetof(struct Pokemon, status),
    offsetof(struct PartyMenu, slotId),
    B_SCR_OP_YESNOBOX,
    STAT_ATK,
    SPECIES_CHANSEY,
    SPECIES_ARCANINE,
    SPECIES_SKARMORY,
    SPECIES_NINETALES,
    offsetof(struct Pokemon, hp),
    B_OUTCOME_RAN,
    B_OUTCOME_LOST,
    offsetof(struct BattleStruct, gimmick.activeGimmick),
    B_TRAINER_PLAYER,
    B_TRAINER_OPPONENT_A,
    PARTY_SIZE,
    GIMMICK_NONE,
    sizeof(struct PaletteFadeControl),
    offsetof(struct BattleScripting, battleStyle),
};
const volatile struct Volatiles gShiftSwitchTrapMask = { .escapePrevention = TRUE };
const volatile struct PaletteFadeControl gShiftSwitchFadeMask = { .active = TRUE };
const volatile u32 gShiftSwitchProbeSourceRules = B_POLISHED_MOVE_BALANCE;

static void SetProbeMon(struct Pokemon *mon, enum Species species, u32 hp, u32 speed, enum Move move)
{
    u32 value;
    CreateRandomMon(mon, species, 50);
    value = 0;
    SetMonData(mon, MON_DATA_ABILITY_NUM, &value);
    // Match ordinary trainer-mon construction: random wild-mon defaults
    // otherwise let the AI Dynamax/Terastallize this setup on its first turn.
    value = BLOCK_AI_DYNAMAX;
    SetMonData(mon, MON_DATA_DYNAMAX_LEVEL, &value);
    value = TYPE_MYSTERY;
    SetMonData(mon, MON_DATA_TERA_TYPE, &value);
    value = 160;
    SetMonData(mon, MON_DATA_MAX_HP, &value);
    SetMonData(mon, MON_DATA_HP, &hp);
    SetMonData(mon, MON_DATA_SPEED, &speed);
    SetMonMoveSlot(mon, move, 0);
    for (u32 slot = 1; slot < MAX_MON_MOVES; slot++)
        SetMonMoveSlot(mon, MOVE_NONE, slot);
}

static void CB2_ShiftSwitchDone(void) {}
static void StartProbeBattle(void);

EWRAM_DATA volatile u32 gShiftSwitchSaveResult = SAVE_STATUS_EMPTY;

static void SaveProbeOptions(void)
{
    gShiftSwitchSaveResult = TrySavingData(SAVE_NORMAL);
    SetMainCallback2(CB2_ShiftSwitchDone);
}

void CB2_MoveMachineAudit(void)
{
    (void)gShiftSwitchProbeLayout[0];
    (void)gShiftSwitchTrapMask.escapePrevention;
    (void)gShiftSwitchFadeMask.active;
    (void)gShiftSwitchProbeSourceRules;
    SetSaveBlocksPointers(0);
    ResetMenuAndMonGlobals();
    Save_ResetSaveCounters();
    Sav2_ClearSetDefault();
    MoveSaveBlocks_ResetHeap();
    if (SHIFT_PROBE_OPTIONS && LoadGameSave(SAVE_NORMAL) == SAVE_STATUS_OK)
    {
        // The observer reopens the saved choice, then leaves the menu for battle.
        gMain.savedCallback = StartProbeBattle;
        SetMainCallback2(CB2_InitOptionMenu);
        return;
    }
    NewGameInitData();
    // Native GEN_9 obedience also checks the met level of owned Pokemon.
    // The Rain Badge makes this level-50 setup obey in both production builds.
    FlagSet(FLAG_BADGE08_GET);
    gSaveBlock2Ptr->optionsTextSpeed = OPTIONS_TEXT_SPEED_FAST;
    gSaveBlock2Ptr->optionsBattleSceneOff = TRUE;
    if (SHIFT_PROBE_OPTIONS)
    {
        gMain.savedCallback = SaveProbeOptions;
        SetMainCallback2(CB2_InitOptionMenu);
        return;
    }
    // Exercise the original save encoding used before Predict was available.
    gSaveBlock2Ptr->optionsBattleStyle = SHIFT_PROBE_SET_MODE
        ? OPTIONS_BATTLE_STYLE_SET : OPTIONS_BATTLE_STYLE_SHIFT;
    StartProbeBattle();
}

static void StartProbeBattle(void)
{
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    SetProbeMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHANSEY, 160, 50, MOVE_SPIKES);
    SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_DRAGON_RAGE, 1);
    u32 status = STATUS1_POISON;
    SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_STATUS, &status);
    if (SHIFT_PROBE_RESERVE != 1)
        SetProbeMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_ARCANINE,
            SHIFT_PROBE_RESERVE == 2 ? 0 : 160, 200, MOVE_SPLASH);
    SetProbeMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_SKARMORY, 40, 100,
        SHIFT_PROBE_TRAP ? MOVE_MEAN_LOOK : MOVE_SPIKES);
    SetProbeMon(&gParties[B_TRAINER_OPPONENT_A][1], SPECIES_NINETALES, 160, 25, MOVE_SPLASH);
    static const u8 nickname[POKEMON_NAME_LENGTH + 1] = {CHAR_E, CHAR_m, CHAR_b, CHAR_e, CHAR_r, EOS};
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_NICKNAME, nickname);
    gPartiesCount[B_TRAINER_PLAYER] = SHIFT_PROBE_RESERVE == 1 ? 1 : 2;
    gPartiesCount[B_TRAINER_OPPONENT_A] = 2;
    TRAINER_BATTLE_PARAM.opponentA = TRAINER_POLISHED_JOEY;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    if (SHIFT_PROBE_WILD_FAINT)
    {
        SetProbeMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_CHANSEY, 160, 200, MOVE_SPLASH);
        SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][0], MOVE_EXPLOSION, 1);
        u32 stat = 1;
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ATK, &stat);
        SetProbeMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_SKARMORY, 1000, 25,
            SHIFT_PROBE_TRAP ? MOVE_MEAN_LOOK : MOVE_SPLASH);
        stat = 1000;
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MAX_HP, &stat);
        stat = 999;
        SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_DEF, &stat);
        ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
        gPartiesCount[B_TRAINER_OPPONENT_A] = 1;
        gBattleTypeFlags = 0;
    }
    gMain.savedCallback = CB2_ShiftSwitchDone;
    SetMainCallback2(CB2_InitBattle);
}
