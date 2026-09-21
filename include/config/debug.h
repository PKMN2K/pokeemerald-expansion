#ifndef GUARD_CONFIG_DEBUG_H
#define GUARD_CONFIG_DEBUG_H

// Overworld Debug
#define DEBUG_OVERWORLD_MENU            TRUE                // Debug IPS build: enable the overworld debug menu.
#define DEBUG_OVERWORLD_HELD_KEYS       (R_BUTTON)          // The keys required to be held to open the debug menu.
#define DEBUG_OVERWORLD_TRIGGER_EVENT   pressedStartButton  // The event that opens the menu when holding the key(s) defined in DEBUG_OVERWORLD_HELD_KEYS.
#define DEBUG_OVERWORLD_IN_MENU         TRUE                // Expose Debug directly in the updated start menu.

// Battle Debug Menu
#define DEBUG_BATTLE_MENU               TRUE  // Enable battle debug menu with Select.
#define DEBUG_AI_DELAY_TIMER            FALSE // If set to TRUE, displays the number of frames it takes for the AI to choose a move.

// Pokémon Debug
#define DEBUG_POKEMON_SPRITE_VISUALIZER TRUE  // Enable sprite/icon visualizer from the summary screen with Select.

#endif // GUARD_CONFIG_DEBUG_H
