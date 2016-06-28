#pragma once


#include "uberdemotools.h"
#include "shared.hpp"
#include "array.hpp"
#include "timer.hpp"
#include "demo.hpp"
#include "sprites.hpp"
#include "string.hpp"
#include "gui.hpp"
#include "shared.hpp"
#include "math_2d.hpp"
#include "nanovg/nanovg.h"


struct Platform;
struct PlatformReadOnly;
struct PlatformReadWrite;


extern const f32 ViewerClearColor[4];

struct Config
{
	f32 StaticZScale = 0.0f;
	f32 DynamicZScale = 0.4f;
};

struct Viewer
{
	static void DemoProgressCallback(f32 progress, void* userData);

	Viewer(Platform& platform);
	~Viewer();

	bool Init(int argc, char** argv);
	void ProcessEvent(const Event& event);
	void Render(RenderParams& renderParams);

private:
	UDT_NO_COPY_SEMANTICS(Viewer);

	struct SpriteDrawParams
	{
		f32 X;
		f32 Y;
		f32 ImageScale;
		f32 CoordsScale;
	};

	bool LoadMapAliases();
	bool LoadSprites();
	void LoadMap(const udtString& mapName);
	bool CreateTextureRGBA(int& textureId, u32 width, u32 height, const u8* pixels);
	void LoadDemo(const char* filePath);
	void RenderDemo(RenderParams& renderParams);
	void RenderNoDemo(RenderParams& renderParams);
	void RenderProgress(RenderParams& renderParams);
	void RenderDemoScore(RenderParams& renderParams);
	void RenderDemoTimer(RenderParams& renderParams);
	void RenderDemoFollowedPlayer(RenderParams& renderParams);
	void DrawMapSpriteAt(const SpriteDrawParams& params, u32 spriteId, const f32* pos, f32 size, f32 zScale, f32 a = 0.0f);
	u32  GetCurrentSnapshotIndex();
	u32  GetSapshotIndexFromTime(u32 elapsedMs);
	f32  GetProgressFromTime(u32 elapsedMs);
	u32  GetTimeFromProgress(f32 progress);
	void PausePlayback();
	void ResumePlayback();
	void TogglePlayback();
	void StopPlayback();
	void ReversePlayback();
	void SetPlaybackProgress(f32 progress);
	void OnKeyPressed(VirtualKey::Id virtualKeyId, bool repeat);
	void OffsetSnapshot(s32 snapshotCount);
	void OffsetTimeMs(s32 durationMs);
	void ComputeMapPosition(f32* result, const f32* input, f32 mapScale, f32 zScale);

	struct MapAlias
	{
		udtString NameFound;
		udtString NameToUse;
	};

	int _sprites[Sprite::Count];
	udtVMLinearAllocator _tempAllocator;
	udtVMLinearAllocator _persistAllocator;
	udtVMArray<MapAlias> _mapAliases;
	Demo _demo;
	Config _config;
	udtTimer _demoPlaybackTimer;
	DemoProgressBar _demoProgressBar;
	PlayPauseButton _playPauseButton;
	StopButton _stopButton;
	ReverseButton _reversePlaybackButton;
	WidgetGroup _activeWidgets;
	f32 _mapMin[3];
	f32 _mapMax[3];
	Rectangle _mapRect;
	Rectangle _uiRect;
	Rectangle _progressRect;
	Platform& _platform;
	PlatformReadOnly* _sharedReadOnly = nullptr;
	PlatformReadWrite* _sharedReadWrite = nullptr;
	Snapshot* _snapshot = nullptr;
	int _map = InvalidTextureId;
	u32 _mapWidth = 0;
	u32 _mapHeight = 0;
	u32 _snapshotIndex = 0; // Index of the currently displayed snapshot.
	f32 _demoLoadProgress = 0.0f;
	bool _appPaused = false;
	bool _wasTimerRunningBeforePause = false;
	bool _wasPlayingBeforeProgressDrag = false;
	bool _reversePlayback = false;
	bool _mapCoordsLoaded = false;
	bool _drawDemoLoadProgress = false;
	bool _timerShowsServerTime = false;
};
