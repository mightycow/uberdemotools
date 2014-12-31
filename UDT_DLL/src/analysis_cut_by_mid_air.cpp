#include "analysis_cut_by_mid_air.hpp"


static bool IsAllowedWeapon(u32 udtWeapon, u32 allowedWeapons)
{
	const u32 result = allowedWeapons & (1 << udtWeapon);

	return result != 0;
}


void udtCutByMidAirAnalyzer::FinishAnalysis()
{
	const udtCutByPatternArg& info = PlugIn->GetInfo();
	const udtCutByMidAirArg& extraInfo = GetExtraInfo<udtCutByMidAirArg>();
	const u32 allowedWeapons = extraInfo.AllowedWeapons;
	const u32 minDistance = extraInfo.MinDistance;
	const u32 minAirTimeMs = extraInfo.MinAirTimeMs;

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
		
		if(minAirTimeMs > 0 && midAir.VictimAirTimeMs < minAirTimeMs)
		{
			continue;
		}
		
		if(midAir.Weapon != (u32)-1 && !IsAllowedWeapon(midAir.Weapon, allowedWeapons))
		{
			continue;
		}
		
		if(midAir.TravelInfoAvailable == 0 || midAir.TravelDistance < minDistance)
		{
			continue;
		}

		udtCutSection cut;
		cut.GameStateIndex = midAir.GameStateIndex;
		cut.StartTimeMs = midAir.ServerTimeMs - info.StartOffsetSec * 1000;
		cut.EndTimeMs = midAir.ServerTimeMs + info.EndOffsetSec * 1000;
		CutSections.Add(cut);
	}
}
