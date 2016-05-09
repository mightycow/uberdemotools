#pragma once


#include "uberdemotools.h"


extern void BuildLookUpTables();
extern bool GetIdNumber(s32& idNumber, udtMagicNumberType::Id numberType, u32 udtNumber, udtProtocol::Id protocol, udtMod::Id mod = udtMod::None);
extern bool GetUDTNumber(u32& udtNumber, udtMagicNumberType::Id numberType, s32 idNumber, udtProtocol::Id protocol, udtMod::Id mod = udtMod::None);
extern s32 GetIdNumber(udtMagicNumberType::Id numberType, u32 udtNumber, udtProtocol::Id protocol, udtMod::Id mod = udtMod::None); // Returns S32_MIN when not available.
extern s32 GetIdEntityStateFlagMask(udtEntityFlag::Id udtFlagId, udtProtocol::Id protocol);
