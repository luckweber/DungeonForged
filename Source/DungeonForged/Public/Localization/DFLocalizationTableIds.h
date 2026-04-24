// Source/DungeonForged/Public/Localization/DFLocalizationTableIds.h
#pragma once

#include "CoreMinimal.h"

/**
 * String table **IDs** (first argument to `LOCTABLE("ID", "Key")` and to `DF_LOCTABLE_*` macros).
 * Create one String Table **asset** per ID under Content (e.g. `Content/Localization/StringTables/`).
 * In each asset, set the table **namespace / ID** to match these constants, then add row keys and translations per culture (pt-BR default, en, es, fr).
 *
 * | Asset suggestion        | ID constant     | Use |
 * |-------------------------|-----------------|-----|
 * | ST_UI                   | `DFL_Table_UI`  | HUD, menus, generic buttons |
 * | ST_Abilities            | `DFL_Table_Ab`  | Ability display names / tooltips |
 * | ST_Items                | `DFL_Table_Items` | Item names, descriptions |
 * | ST_Enemies              | `DFL_Table_Enemies` | Pawn / enemy display names |
 * | ST_Events               | `DFL_Table_Events` | Random event text, choice labels |
 * | ST_Tutorials            | `DFL_Table_Tut`  | Hints, tutorial popups |
 */
namespace DFL
{
inline const TCHAR* Table_UI = TEXT("ST_UI");
inline const TCHAR* Table_Abilities = TEXT("ST_Abilities");
inline const TCHAR* Table_Items = TEXT("ST_Items");
inline const TCHAR* Table_Enemies = TEXT("ST_Enemies");
inline const TCHAR* Table_Events = TEXT("ST_Events");
inline const TCHAR* Table_Tutorials = TEXT("ST_Tutorials");
}
