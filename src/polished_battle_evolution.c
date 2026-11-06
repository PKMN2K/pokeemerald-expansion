#include "global.h"
#include "battle.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "config_changes.h"
#include "evolution_scene.h"
#include "m4a.h"
#include "main.h"
#include "malloc.h"
#include "palette.h"
#include "polished_evolution.h"
#include "reshow_battle_screen.h"
#include "sound.h"
#include "constants/rgb.h"

// Pinned engine/battle/core.asm, evolve_battle.asm and abilities.asm. The
// default release enables EVOLVE_IN_BATTLE_OPT. Battle allocations and script
// stacks stay alive; only the display is handed to the ordinary evolution UI.
struct BattleEvolution
{
    struct BattleScripting scripting;
    struct MusicPlayerInfo music;
    struct MusicPlayerTrack tracks[MAX_MUSICPLAYER_TRACKS];
    MainCallback afterEvolution;
    enum Species oldSpecies, targetSpecies;
    u8 communication[BATTLE_COMMUNICATION_ENTRIES_COUNT];
    u8 text1[sizeof(gBattleTextBuff1)], text2[sizeof(gBattleTextBuff2)], text3[sizeof(gBattleTextBuff3)];
    u8 environment, partyId, state, abilityState;
    enum BattlerId evolvedBattler;
};

static EWRAM_DATA struct BattleEvolution *sBattleEvolution = NULL;

bool32 PolishedBattleEvolutionIsRunning(void)
{
    return sBattleEvolution != NULL;
}

bool32 PolishedCanEvolveDuringBattle(u32 partyId)
{
    if (!GetConfig(POLISHED_EVOLUTIONS) || partyId >= PARTY_SIZE
        || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER) || gBattleOutcome != 0
        || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK
            | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SAFARI
            | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_CATCH_TUTORIAL))
        || PolishedGetSpeciesEvolutions(GetMonData(&gParties[B_TRAINER_PLAYER][partyId], MON_DATA_SPECIES)) == NULL)
        return FALSE;

    // Check the opponent's entire party, not just the current battler's HP.
    // Wild battles and the trainer's last Pokemon defer until after battle.
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
        if (!IsOnPlayerSide(battler))
            for (u32 i = 0; i < PARTY_SIZE; i++)
                if (IsValidForBattle(&GetBattlerParty(battler)[i]))
                    return TRUE;
    return FALSE;
}

void PolishedReloadBattleMonAfterEvolution(enum BattlerId battler, bool32 evolved)
{
    struct Pokemon *mon = GetBattlerMon(battler);
    struct BattlePokemon *battleMon = &gBattleMons[battler];

    // The source reloads the active mon even for cancellation/bench evolution.
    // In particular, do not copy a bench recipient over the active battler or
    // reset stat stages, status counters, substitute, or the opponent's data.
    battleMon->species = GetMonData(mon, MON_DATA_SPECIES);
    CopyMonLevelAndBaseStatsToBattleMon(battler, mon, TRUE);
    battleMon->status1 = GetMonData(mon, MON_DATA_STATUS);
    battleMon->personality = GetMonData(mon, MON_DATA_PERSONALITY);
    battleMon->hpIV = GetMonData(mon, MON_DATA_HP_IV);
    battleMon->attackIV = GetMonData(mon, MON_DATA_ATK_IV);
    battleMon->defenseIV = GetMonData(mon, MON_DATA_DEF_IV);
    battleMon->speedIV = GetMonData(mon, MON_DATA_SPEED_IV);
    battleMon->spAttackIV = GetMonData(mon, MON_DATA_SPATK_IV);
    battleMon->spDefenseIV = GetMonData(mon, MON_DATA_SPDEF_IV);
    battleMon->abilityNum = GetMonData(mon, MON_DATA_ABILITY_NUM);
    GetMonData(mon, MON_DATA_NICKNAME, battleMon->nickname);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        battleMon->moves[i] = GetMonData(mon, MON_DATA_MOVE1 + i);
        battleMon->pp[i] = GetMonData(mon, MON_DATA_PP1 + i);
    }
    battleMon->ppBonuses = GetMonData(mon, MON_DATA_PP_BONUSES);
    battleMon->types[0] = GetSpeciesType(battleMon->species, 0);
    battleMon->types[1] = GetSpeciesType(battleMon->species, 1);
    battleMon->types[2] = TYPE_MYSTERY;
    if (gBattleSpritesDataPtr != NULL)
        gBattleSpritesDataPtr->battlerData[battler].transformSpecies = SPECIES_NONE;
    if (evolved)
    {
        battleMon->item = GetMonData(mon, MON_DATA_HELD_ITEM);
        battleMon->ability = GetMonAbility(mon);
        battleMon->volatiles.overwrittenAbility = ABILITY_NONE;
        battleMon->volatiles.gastroAcid = FALSE;
        battleMon->volatiles.traceActivated = FALSE;
        battleMon->volatiles.unnerveActivated = FALSE;
        battleMon->volatiles.neutralizingGas = FALSE;
        battleMon->volatiles.weatherAbilityDone = FALSE;
    }
}

static void ReturnToBattle(void)
{
    struct BattleEvolution *evo = sBattleEvolution;
    bool32 evolved = GetMonData(&gParties[B_TRAINER_PLAYER][evo->partyId], MON_DATA_SPECIES) != evo->oldSpecies;

    gBattleScripting = evo->scripting;
    memcpy(gBattleCommunication, evo->communication, sizeof(evo->communication));
    memcpy(gBattleTextBuff1, evo->text1, sizeof(evo->text1));
    memcpy(gBattleTextBuff2, evo->text2, sizeof(evo->text2));
    memcpy(gBattleTextBuff3, evo->text3, sizeof(evo->text3));
    gBattleEnvironment = evo->environment;
    gCB2_AfterEvolution = evo->afterEvolution;
    evo->evolvedBattler = MAX_BATTLERS_COUNT;
    for (enum BattlerId battler = 0; battler < gBattlersCount; battler++)
        if (GetBattlerTrainer(battler) == B_TRAINER_PLAYER && !(gAbsentBattlerFlags & (1u << battler)))
        {
            bool32 activeEvolved = evolved && gBattlerPartyIndexes[battler] == evo->partyId;
            PolishedReloadBattleMonAfterEvolution(battler, activeEvolved);
            if (activeEvolved)
                evo->evolvedBattler = battler;
        }

    // MPlayStop detaches sound channels. Restoring the paused sequencer keeps
    // its tempo, loop position and track program counters across the animation.
    m4aMPlayAllStop();
    gMPlayInfo_BGM = evo->music;
    memcpy(gMPlayInfo_BGM.tracks, evo->tracks, gMPlayInfo_BGM.trackCount * sizeof(evo->tracks[0]));
    m4aMPlayContinue(&gMPlayInfo_BGM);
    AllocateMonSpritesGfx();
    evo->state = 2;
    ReshowBattleScreenAfterMenu();
}

static bool32 EntryAbilityNeedsOpponent(enum Ability ability)
{
    switch (ability)
    {
    case ABILITY_TRACE:
    case ABILITY_IMPOSTER:
    case ABILITY_INTIMIDATE:
    case ABILITY_DOWNLOAD:
    case ABILITY_ANTICIPATION:
    case ABILITY_FOREWARN:
    case ABILITY_FRISK:
    case ABILITY_UNNERVE:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 RunEvolutionAbility(void)
{
    enum BattlerId battler = sBattleEvolution->evolvedBattler;
    enum Ability ability;
    bool32 hasOpponent = FALSE;
    if (battler == MAX_BATTLERS_COUNT || !IsBattlerAlive(battler))
        return FALSE;
    ability = GetBattlerAbility(battler);
    for (enum BattlerId i = 0; i < gBattlersCount; i++)
        if (!IsOnPlayerSide(i) && IsBattlerAlive(i))
            hasOpponent = TRUE;
    // Source BattleEntryAbilities skips foe-dependent effects while the
    // defeated trainer mon is still on screen. Status cures still run.
    while (sBattleEvolution->abilityState < 4)
    {
        enum AbilityEffect effect;
        switch (sBattleEvolution->abilityState++)
        {
        case 0: effect = ABILITYEFFECT_NEUTRALIZINGGAS; break;
        case 1:
            if (!hasOpponent) continue;
            effect = ABILITYEFFECT_UNNERVE;
            break;
        case 2:
            if (!hasOpponent && EntryAbilityNeedsOpponent(ability)) continue;
            effect = ABILITYEFFECT_ON_SWITCHIN;
            break;
        default: effect = ABILITYEFFECT_IMMUNITY; break;
        }
        if (AbilityBattleEffects(effect, battler, ability, MOVE_NONE, TRUE))
            return TRUE; // Resume at the next effect when its script returns.
    }
    return FALSE;
}

bool32 PolishedTryBattleEvolution(u32 partyId)
{
    struct BattleEvolution *evo = sBattleEvolution;
    if (partyId >= PARTY_SIZE)
        return FALSE;
    if (evo == NULL)
    {
        struct Pokemon *mon;
        enum Species target;
        if (!gBattleStruct->polishedLevelUp)
            return FALSE;
        gBattleStruct->polishedLevelUp = FALSE;
        if (!PolishedCanEvolveDuringBattle(partyId))
            return FALSE;
        // A mid-battle attempt, including cancellation/no matching rule, does
        // not add the source's deferred level-up flag. Critical-hit flags stay.
        gLeveledUpInBattle &= ~(1u << partyId);
        mon = &gParties[B_TRAINER_PLAYER][partyId];
        target = PolishedGetEvolutionTargetSpecies(mon, EVO_MODE_NORMAL, ITEM_NONE, NULL);
        if (target == SPECIES_NONE)
            return FALSE;
        sBattleEvolution = evo = AllocZeroed(sizeof(*evo));
        evo->partyId = partyId;
        evo->oldSpecies = GetMonData(mon, MON_DATA_SPECIES);
        evo->targetSpecies = target;
        evo->scripting = gBattleScripting;
        evo->environment = gBattleEnvironment;
        evo->afterEvolution = gCB2_AfterEvolution;
        memcpy(evo->communication, gBattleCommunication, sizeof(evo->communication));
        memcpy(evo->text1, gBattleTextBuff1, sizeof(evo->text1));
        memcpy(evo->text2, gBattleTextBuff2, sizeof(evo->text2));
        memcpy(evo->text3, gBattleTextBuff3, sizeof(evo->text3));
        BattleStopLowHpSound();
        m4aMPlayStop(&gMPlayInfo_BGM);
        evo->music = gMPlayInfo_BGM;
        memcpy(evo->tracks, gMPlayInfo_BGM.tracks, gMPlayInfo_BGM.trackCount * sizeof(evo->tracks[0]));
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        return TRUE;
    }
    // callback1 still executes while callback2 rebuilds the battle display.
    // Do not run ability scripts or advance to another EXP recipient until
    // reshow has recreated all windows/sprites and finished fading in.
    if (gMain.callback2 != BattleMainCB2 || gPaletteFade.active || gBattleControllerExecFlags)
        return TRUE;
    if (evo->state == 0)
    {
        CloseMainBattleScreen();
        FreeMonSpritesGfx();
        gCB2_AfterEvolution = ReturnToBattle;
        evo->state = 1;
        EvolutionScene(&gParties[B_TRAINER_PLAYER][partyId], evo->targetSpecies, TRUE, partyId);
        return TRUE;
    }
    if (evo->state != 2 || RunEvolutionAbility())
        return TRUE;
    HandleBattleLowHpMusicChange();
    FREE_AND_SET_NULL(sBattleEvolution);
    return FALSE;
}
