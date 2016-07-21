#pragma once


#include "uberdemotools.h"
#include "shared.hpp"
#include "platform.hpp"
#include "array.hpp"
#include "timer.hpp"
#include "demo.hpp"
#include "sprites.hpp"
#include "string.hpp"
#include "gui.hpp"
#include "shared.hpp"
#include "math_2d.hpp"
#include "config.hpp"
#include "nanovg/nanovg.h"


struct Platform;
struct PlatformReadOnly;
struct PlatformReadWrite;


extern const f32 ViewerClearColor[4];

#define TAB_LIST(N) \
	N(Options, "Options") \
	N(Chat, "Chat") \
	N(HeatMaps, "Heat Maps") \
	N(Log, "Log")

#define ITEM(Enum, String) Enum,
struct Tab
{
	enum Id
	{
		TAB_LIST(ITEM)
		Count
	};
};
#undef ITEM

struct Viewer
{
	static void DemoProgressCallback(f32 progress, void* userData);
	static void DemoLoadThreadEntryPoint(void* userData);
	static void HeatMapsGenThreadEntryPoint(void* userData);

	Viewer(Platform& platform);
	~Viewer();

	bool Init(int argc, char** argv);
	void ProcessEvent(const Event& event);
	void Render(const RenderParams& renderParams);
	void ShutDown();

private:
	UDT_NO_COPY_SEMANTICS(Viewer);

	struct SpriteDrawParams
	{
		f32 X;
		f32 Y;
		f32 ImageScale;
		f32 CoordsScale;
	};

	struct AppState
	{
		enum Id
		{
			Normal,
			LoadingDemo,
			FinishDemoLoading,
			GeneratingHeatMaps,
			FinishHeatMapGeneration,
			Count
		};
	};

	bool LoadMapAliases();
	bool LoadSprites();
	void LoadMap(const udtString& mapName);
	bool CreateTextureRGBA(int& textureId, u32 width, u32 height, const u8* pixels);
	void StartLoadingDemo(const char* filePath);
	void LoadDemo(const char* filePath);
	void StartGeneratingHeatMaps();
	void GenerateHeatMaps();
	void RenderNormal(const RenderParams& renderParams);
	void RenderDemo(const RenderParams& renderParams);
	void RenderNoDemo(const RenderParams& renderParams);
	void RenderThreadedJobProgress(const RenderParams& renderParams);
	void RenderDemoOverlays(const RenderParams& renderParams);
	void RenderDemoScore(const RenderParams& renderParams);
	void RenderDemoTimer(const RenderParams& renderParams);
	void RenderDemoFollowedPlayer(const RenderParams& renderParams);
	void DrawProgressSliderToolTip(const RenderParams& renderParams);
	void DrawHelp(const RenderParams& renderParams);
	void DrawChat(const RenderParams& renderParams, s32 displayTimeMs);
	void DrawLog(const RenderParams& renderParams);
	void DrawMapSpriteAt(const SpriteDrawParams& params, u32 spriteId, const f32* pos, f32 size, f32 zScale, f32 a = 0.0f);
	void DrawSpriteAt(u32 spriteId, f32 x, f32 y, f32 size, f32 a = 0.0f);
	void GetIndexAndTimes(u32& snapshotIndex, s32& displayTimeMs, s32& serverTimeMs);
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
	void FinishLoadingDemo();
	void FinishGeneratingHeatMaps();

	struct MapAlias
	{
		udtString NameFound;
		udtString NameToUse;
	};

	struct DemoLoadThreadData
	{
		char FilePath[512];
		Viewer* ViewerPtr;
	};

	struct HeatMapsGenThreadData
	{
		Viewer* ViewerPtr;
	};

	struct HeatMapData
	{
		void* Image = nullptr;
		u32 Width = 0;
		u32 Height = 0;
		int TextureId = InvalidTextureId;
	};

	HeatMapData _heatMaps[64];
	int _sprites[Sprite::Count];
	u8 _heatMapBtnIdxToPlayerIdx[64];
	udtVMLinearAllocator _tempAllocator { "Viewer::Temp" };
	udtVMLinearAllocator _persistAllocator { "Viewer::Persist" };
	udtVMArray<MapAlias> _mapAliases { "Viewer::AliasArray" };
	Demo _demo;
	Config _config;
	udtTimer _demoPlaybackTimer;
	udtTimer _globalTimer;
	udtTimer _genericTimer;
	DemoProgressSlider _demoProgressBar;
	PlayPauseButton _playPauseButton;
	StopButton _stopButton;
	ReverseButton _reversePlaybackButton;
	CheckBox _onlyFirstMatchCheckBox;
	CheckBox _removeTimeOutsCheckBox;
	CheckBox _showServerTimeCheckBox;
	CheckBox _drawMapOverlaysCheckBox;
	CheckBox _drawMapScoresCheckBox;
	CheckBox _drawMapClockCheckBox;
	CheckBox _drawMapFollowMsgCheckBox;
	CheckBox _drawMapHealthCheckBox;
	Slider _staticZScaleSlider;
	Slider _dynamicZScaleSlider;
	Slider _globalScaleSlider;
	TextButton _reloadDemoButton;
	WidgetGroup _activeWidgets;
	WidgetGroup _tabWidgets[Tab::Count];
	RadioButton _tabButtons[Tab::Count];
	RadioGroup _tabButtonGroup;
	RadioButton _heatMapPlayers[64];
	RadioGroup _heatMapGroup;
	TextButton _genHeatMapsButton;
	Slider _heatMapOpacitySlider;
	CheckBox _heatMapSquaredRampCheckBox;
	CheckBox _drawHeatMapCheckBox;
	CheckBox _drawHeatMapOnlyCheckBox;
	f32 _mapMin[3];
	f32 _mapMax[3];
	Rectangle _mapRect;
	Rectangle _uiRect;
	Rectangle _tabBarRect;
	Rectangle _progressRect;
	Platform& _platform;
	PlatformReadOnly* _sharedReadOnly = nullptr;
	PlatformReadWrite* _sharedReadWrite = nullptr;
	Snapshot* _snapshot = nullptr;
	AppState::Id _appState = AppState::Normal;
	CriticalSectionId _appStateLock = InvalidCriticalSectionId;
	void* _mapImage = nullptr;
	void* _heatMapImages = nullptr;
	WidgetGroup* _activeTabWidgets = nullptr;
	const char* _threadedJobText = nullptr;
	int _map = InvalidTextureId;
	u32 _mapWidth = 0;
	u32 _mapHeight = 0;
	u32 _snapshotIndex = 0; // Index of the currently displayed snapshot.
	f32 _threadedJobProgress = 0.0f;
	s32 _clockTimeMs = 0;
	bool _appPaused = false;
	bool _wasTimerRunningBeforePause = false;
	bool _wasPlayingBeforeProgressDrag = false;
	bool _reversePlayback = false;
	bool _mapCoordsLoaded = false;
	bool _appLoaded = false;
	bool _displayHelp = false;
	bool _profileMode = false;
};
