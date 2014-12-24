#include "analysis_cut_by_mid_air.hpp"

#include <assert.h>


static bool IsAllowedWeapon(u32 udtWeapon, u32 allowedWeapons)
{
	const u32 result = allowedWeapons & (1 << udtWeapon);

	return result != 0;
}


void udtCutByMidAirAnalyzer::FinishAnalysis()
{
	assert(_info != NULL);
	assert(_extraInfo != NULL);

	const udtCutByMidAirArg* const extraInfo = (const udtCutByMidAirArg*)_extraInfo;
	const u32 allowedWeapons = extraInfo->AllowedWeapons;
	const u32 minDistance = extraInfo->MinDistance;

	const udtVMArray<udtParseDataMidAir>& midAirs = _analyzer.MidAirs;

	// @TODO: handle cut section overlaps here like in the other analyzers.

	for(u32 i = 0, count = midAirs.GetSize(); i < count; ++i)
	{
		const udtParseDataMidAir midAir = midAirs[i];

		// @TODO: pass the player index in udtCutByMidAirArg!
		if(midAir.AttackerIdx != (u32)_analyzer.RecordingPlayerIndex)
		{
			continue;
		}

		if(midAir.Weapon != (u32)-1 && !IsAllowedWeapon(midAir.Weapon, allowedWeapons))
		{
			continue;
		}
		
		if(midAir.TravelInfoAvailable && midAir.TravelDistance < minDistance)
		{
			continue;
		}

		udtCutSection cut;
		cut.GameStateIndex = midAir.GameStateIndex;
		cut.StartTimeMs = midAir.ServerTimeMs - _info->StartOffsetSec * 1000;
		cut.EndTimeMs = midAir.ServerTimeMs + _info->EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}