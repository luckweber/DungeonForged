// Source/DungeonForged/Public/Localization/DFLocalizationMacros.h
#pragma once

#include "Internationalization/StringTableRegistry.h"

// Requires matching String Table assets; see DFLocalizationTableIds.h.
#define DF_LOCTABLE_UI(Key) LOCTABLE("ST_UI", Key)
#define DF_LOCTABLE_ABILITIES(Key) LOCTABLE("ST_Abilities", Key)
#define DF_LOCTABLE_ITEMS(Key) LOCTABLE("ST_Items", Key)
#define DF_LOCTABLE_ENEMIES(Key) LOCTABLE("ST_Enemies", Key)
#define DF_LOCTABLE_EVENTS(Key) LOCTABLE("ST_Events", Key)
#define DF_LOCTABLE_TUTORIALS(Key) LOCTABLE("ST_Tutorials", Key)
