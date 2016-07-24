#pragma once


#include "uberdemotools.h"


#define CONFIG_VARS(N) \
	N(f32, GlobalScale, 1.0f) \
	N(f32, StaticZScale, 0.0f) \
	N(f32, DynamicZScale, 0.4f) \
	N(f32, HeatMapOpacity, 0.75f) \
	N(bool, TimerShowsServerTime, false) \
	N(bool, DrawMapOverlays, true) \
	N(bool, DrawMapScores, true) \
	N(bool, DrawMapClock, true) \
	N(bool, DrawMapFollowMsg, true) \
	N(bool, DrawMapHealth, true) \
	N(bool, HeatMapSquaredRamp, false) \
	N(bool, OnlyKeepFirstMatchSnapshots, true) \
	N(bool, RemoveTimeOutSnapshots, true) \
	N(bool, DrawHeatMap, true) \
	N(bool, DrawHeatMapOnly, true)

#define ITEM(Type, Name, Value) Type Name = Value;
struct Config
{
	CONFIG_VARS(ITEM)
};
#undef ITEM


extern bool LoadConfig(Config& config, const char* filePath);
extern bool SaveConfig(const Config& config, const char* filePath);
