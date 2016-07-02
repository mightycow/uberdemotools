#include "viewer.hpp"
#include "file_stream.hpp"
#include "math.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "nanovg_drawing.hpp"
#include "nanovg/fontstash.h"
#include "nanovg/stb_image.h"
#include "blendish/blendish.h"

#include <stdlib.h>
#include <math.h>


#define  DATA_PATH          "viewer_data"
#define  DATA_PATH_ICONS    DATA_PATH"/blender_icons.png"
#define  DATA_PATH_FONT     DATA_PATH"/DejaVuSans.ttf"
#define  DATA_PATH_ALIASES  DATA_PATH"/mapaliases.txt"
#define  DATA_PATH_SPRITES  DATA_PATH"/sprites.texturepack"


static const char* const HelpBindStrings[] =
{
	"KEY", "ACTION",
	"F1", "toggle display of this help overlay",
	"escape", "quit",
	"space", "toggle play/pause",
	"P", "toggle play/pause",
	"R", "toggle forward/reverse playback",
	"F", "toggle maximized/restored",
	"left", "jump to the previous snapshot and pause",
	"right", "jump to the next snapshot and pause",
	"up", "jump forward by 1 second",
	"down", "jump backward by 1 second",
	"page up", "jump forward by 10 seconds",
	"page down", "jump backward by 10 seconds"
};


#define ITEM(Enum, String) String,
static const char* TabNames[Tab::Count + 1] =
{
	TAB_LIST(ITEM)
	""
};
#undef ITEM

static const NVGcolor PlayerColors[6] =
{
	// free, free followed, red, red followed, blue, blue followed
	nvgRGB(255, 0, 0),
	nvgRGB(255, 127, 127),
	nvgRGB(255, 0, 0),
	nvgRGB(255, 127, 127),
	nvgRGB(80, 80, 255),
	nvgRGB(160, 160, 255)
};

static const NVGcolor RailColors[3] =
{
	// free, red, blue
	nvgRGB(255, 0, 0),
	nvgRGB(255, 0, 0),
	nvgRGB(0, 0, 255)
};

static s32 GetSpriteIdFromItemId(s32 itemId)
{
	switch((udtItem::Id)itemId)
	{
		case udtItem::AmmoGrenades: return (s32)Sprite::ammo_gl;
		case udtItem::AmmoLightning: return (s32)Sprite::ammo_lg;
		case udtItem::AmmoBullets: return (s32)Sprite::ammo_mg;
		case udtItem::AmmoCells: return (s32)Sprite::ammo_pg;
		case udtItem::AmmoSlugs: return (s32)Sprite::ammo_rg;
		case udtItem::AmmoRockets: return (s32)Sprite::ammo_rl;
		case udtItem::AmmoShells: return (s32)Sprite::ammo_sg;
		case udtItem::ItemArmorJacket: return (s32)Sprite::armor_green;
		case udtItem::ItemArmorBody: return (s32)Sprite::armor_red;
		case udtItem::ItemArmorShard: return (s32)Sprite::armor_shard;
		case udtItem::ItemArmorCombat: return (s32)Sprite::armor_yellow;
		case udtItem::FlagBlue: return (s32)Sprite::flag_blue;
		case udtItem::FlagRed: return (s32)Sprite::flag_red;
		case udtItem::ItemHealthLarge: return (s32)Sprite::health_large;
		case udtItem::ItemHealth: return (s32)Sprite::health_medium;
		case udtItem::ItemHealthMega: return (s32)Sprite::health_mega;
		case udtItem::ItemHealthSmall: return (s32)Sprite::health_small;
		case udtItem::ItemEnviro: return (s32)Sprite::powerup_bs;
		case udtItem::ItemFlight: return (s32)Sprite::powerup_flight;
		case udtItem::ItemHaste: return (s32)Sprite::powerup_haste;
		case udtItem::ItemInvis: return (s32)Sprite::powerup_invis;
		case udtItem::ItemQuad: return (s32)Sprite::powerup_quad;
		case udtItem::ItemRegen: return (s32)Sprite::powerup_regen;
		case udtItem::WeaponGauntlet: return (s32)Sprite::weapon_gauntlet;
		case udtItem::WeaponGrenadeLauncher: return (s32)Sprite::weapon_gl;
		case udtItem::WeaponLightningGun: return (s32)Sprite::weapon_lg;
		case udtItem::WeaponMachinegun: return (s32)Sprite::weapon_mg;
		case udtItem::WeaponPlasmaGun: return (s32)Sprite::weapon_pg;
		case udtItem::WeaponRailgun: return (s32)Sprite::weapon_rg;
		case udtItem::WeaponRocketLauncher: return (s32)Sprite::weapon_rl;
		case udtItem::WeaponShotgun: return (s32)Sprite::weapon_sg;
		default: return -1;
	}
}

static s32 GetSpriteIdFromDynamicItemId(s32 itemId)
{
	switch((DynamicItemType::Id)itemId)
	{
		case DynamicItemType::AmmoGrenades: return (s32)Sprite::ammo_gl;
		case DynamicItemType::AmmoLightning: return (s32)Sprite::ammo_lg;
		case DynamicItemType::AmmoBullets: return (s32)Sprite::ammo_mg;
		case DynamicItemType::AmmoCells: return (s32)Sprite::ammo_pg;
		case DynamicItemType::AmmoSlugs: return (s32)Sprite::ammo_rg;
		case DynamicItemType::AmmoRockets: return (s32)Sprite::ammo_rl;
		case DynamicItemType::AmmoShells: return (s32)Sprite::ammo_sg;
		case DynamicItemType::FlagBlue: return (s32)Sprite::flag_blue;
		case DynamicItemType::FlagRed: return (s32)Sprite::flag_red;
		case DynamicItemType::ItemEnviro: return (s32)Sprite::powerup_bs;
		case DynamicItemType::ItemFlight: return (s32)Sprite::powerup_flight;
		case DynamicItemType::ItemHaste: return (s32)Sprite::powerup_haste;
		case DynamicItemType::ItemInvis: return (s32)Sprite::powerup_invis;
		case DynamicItemType::ItemQuad: return (s32)Sprite::powerup_quad;
		case DynamicItemType::ItemRegen: return (s32)Sprite::powerup_regen;
		case DynamicItemType::WeaponGauntlet: return (s32)Sprite::weapon_gauntlet;
		case DynamicItemType::WeaponGrenadeLauncher: return (s32)Sprite::weapon_gl;
		case DynamicItemType::WeaponLightningGun: return (s32)Sprite::weapon_lg;
		case DynamicItemType::WeaponMachinegun: return (s32)Sprite::weapon_mg;
		case DynamicItemType::WeaponPlasmaGun: return (s32)Sprite::weapon_pg;
		case DynamicItemType::WeaponRailgun: return (s32)Sprite::weapon_rg;
		case DynamicItemType::WeaponRocketLauncher: return (s32)Sprite::weapon_rl;
		case DynamicItemType::WeaponShotgun: return (s32)Sprite::weapon_sg;
		default: return -1;
	}
}

static s32 GetSpriteIdFromWeaponId(u8 weaponId)
{
	switch((udtWeapon::Id)weaponId)
	{
		case udtWeapon::Gauntlet: return Sprite::gauntlet;
		case udtWeapon::MachineGun: return Sprite::mg;
		case udtWeapon::Shotgun: return Sprite::sg;
		case udtWeapon::GrenadeLauncher: return Sprite::gl;
		case udtWeapon::RocketLauncher: return Sprite::rl;
		case udtWeapon::PlasmaGun: return Sprite::pg;
		case udtWeapon::Railgun: return Sprite::rg;
		case udtWeapon::LightningGun: return Sprite::lg;
		default: return -1;
	}
}

static s32 GetIconSpriteIdFromWeaponId(u8 weaponId)
{
	switch((udtWeapon::Id)weaponId)
	{
		case udtWeapon::Gauntlet: return Sprite::weapon_gauntlet;
		case udtWeapon::MachineGun: return Sprite::weapon_mg;
		case udtWeapon::Shotgun: return Sprite::weapon_sg;
		case udtWeapon::GrenadeLauncher: return Sprite::weapon_gl;
		case udtWeapon::RocketLauncher: return Sprite::weapon_rl;
		case udtWeapon::PlasmaGun: return Sprite::weapon_pg;
		case udtWeapon::Railgun: return Sprite::weapon_rg;
		case udtWeapon::LightningGun: return Sprite::weapon_lg;
		default: return -1;
	}
}

static void DrawTexturedRect(NVGcontext* ctx, f32 x, f32 y, f32 w, f32 h, int textureId)
{
	nvgBeginPath(ctx);
	nvgRect(ctx, x, y, w, h);
	nvgFillPaint(ctx, nvgImagePattern(ctx, x, y, w, h, 0.0f, textureId, 1.0f));
	nvgFill(ctx);
	nvgClosePath(ctx);
}


const f32 ViewerClearColor[4] = { 0.447f, 0.447f, 0.447f, 1.0f };


struct CriticalSectionLock
{
	CriticalSectionLock(CriticalSectionId lock)
		: _lock(lock)
	{
		Platform_EnterCriticalSection(lock);
	}

	~CriticalSectionLock()
	{
		Platform_LeaveCriticalSection(_lock);
	}

private:
	CriticalSectionId _lock;
};

void Viewer::DemoProgressCallback(f32 progress, void* userData)
{
	Viewer* const viewer = (Viewer*)userData;
	viewer->_demoLoadProgress = progress;
}

void Viewer::DemoLoadThreadEntryPoint(void* userData)
{
	const DemoLoadThreadData& data = *(const DemoLoadThreadData*)userData;
	Viewer* const viewer = data.ViewerPtr;
	viewer->LoadDemoImpl(data.FilePath);
	free(userData);
	CriticalSectionLock lock(viewer->_appStateLock);
	viewer->_appState = AppState::UploadingMapTexture;
}

Viewer::Viewer(Platform& platform)
	: _platform(platform)
{
	_tempAllocator.Init(1 << 16, "Viewer::Temp");
	_persistAllocator.Init(1 << 16, "Viewer::Persist");
	_mapAliases.Init(1 << 16, "Viewer::AliasArray");

	Platform_GetSharedDataPointers(platform, (const PlatformReadOnly**)&_sharedReadOnly, &_sharedReadWrite);
}

Viewer::~Viewer()
{
	Platform_ReleaseCriticalSection(_appStateLock);
	free(_snapshot);
}

bool Viewer::Init(int argc, char** argv)
{
	Snapshot* const snapshot = (Snapshot*)malloc(sizeof(Snapshot));
	if(snapshot == nullptr)
	{
		Platform_PrintError("Failed to allocate %d bytes for snapshot data", (int)sizeof(Snapshot));
		return false;
	}
	_snapshot = snapshot;
	
	if(!_demo.Init(&DemoProgressCallback, this) ||
	   !LoadMapAliases() ||
	   !LoadSprites())
	{
		return false;
	}

	const int font = nvgCreateFont(_sharedReadOnly->NVGContext, "sans", DATA_PATH_FONT);
	if(font == FONS_INVALID)
	{
		Platform_PrintError("Failed to open/load font file %s", DATA_PATH_FONT);
		return false;
	}

	if(!udtFileStream::Exists(DATA_PATH_ICONS))
	{
		Platform_PrintError("Failed to open icon sheet file %s", DATA_PATH_ICONS);
		return false;
	}

	bndSetFont(font);
	bndSetIconImage(nvgCreateImage(_sharedReadOnly->NVGContext, DATA_PATH_ICONS, 0));

	_playPauseButton.SetTimerPtr(&_demoPlaybackTimer);
	_reversePlaybackButton.SetReversedPtr(&_reversePlayback);
	_showServerTimeCheckBox.SetActivePtr(&_timerShowsServerTime);
	_showServerTimeCheckBox.SetText("Clock shows server time");
	_drawMapOverlaysCheckBox.SetActivePtr(&_drawMapOverlays);
	_drawMapOverlaysCheckBox.SetText("Enable map overlays");
	_drawMapScoresCheckBox.SetActivePtr(&_drawMapScores);
	_drawMapScoresCheckBox.SetText("Draw scores");
	_drawMapClockCheckBox.SetActivePtr(&_drawMapClock);
	_drawMapClockCheckBox.SetText("Draw clock");
	_drawMapFollowMsgCheckBox.SetActivePtr(&_drawMapFollowMsg);
	_drawMapFollowMsgCheckBox.SetText("Draw follow message");
	_drawMapHealthCheckBox.SetActivePtr(&_drawMapHealth);
	_drawMapHealthCheckBox.SetText("Draw followed player status bar");
	
	WidgetGroup& options = _tabWidgets[Tab::Options];
	options.AddWidget(&_showServerTimeCheckBox);
	options.AddWidget(&_drawMapOverlaysCheckBox);
	options.AddWidget(&_drawMapScoresCheckBox);
	options.AddWidget(&_drawMapClockCheckBox);
	options.AddWidget(&_drawMapFollowMsgCheckBox);
	options.AddWidget(&_drawMapHealthCheckBox);

	_activeWidgets.AddWidget(&_playPauseButton);
	_activeWidgets.AddWidget(&_stopButton);
	_activeWidgets.AddWidget(&_reversePlaybackButton);
	_activeWidgets.AddWidget(&_demoProgressBar);
	_activeWidgets.AddWidget(&_tabButtonGroup);

	_activeTabWidgets = &_tabWidgets[0];
	for(u32 i = 0; i < (u32)Tab::Count; ++i)
	{
		int flags = BND_CORNER_TOP;
		if(i > 0)
		{
			flags |= BND_CORNER_LEFT;
		}
		if(i < (u32)Tab::Count - 1)
		{
			flags |= BND_CORNER_RIGHT;
		}
		_tabButtons[i].SetCornerFlags(flags);
		_tabButtons[i].SetActive(i == 0);
		_tabButtons[i].SetText(TabNames[i]);
		_tabButtonGroup.AddRadioButton(&_tabButtons[i]);
	}

	Platform_CreateCriticalSection(_appStateLock);
	if(argc >= 2 && udtFileStream::Exists(argv[1]))
	{
		CriticalSectionLock lock(_appStateLock);
		_appState = AppState::LoadingDemo;
		StartLoadingDemo(argv[1]);
	}
	
	_demoPlaybackTimer.Start();
	_globalTimer.Start();
	_appLoaded = true;

	return true;
}

bool Viewer::LoadMapAliases()
{
	// Line endings: \r\n

	udtFileStream file;
	if(!file.Open(DATA_PATH_ALIASES, udtFileOpenMode::Read))
	{
		Platform_PrintError("Failed to open map aliases file %s", DATA_PATH_ALIASES);
		return false;
	}

	udtString fileData = file.ReadAllAsString(_persistAllocator);
	u32 lineStart = 0;
	for(;;)
	{
		const udtString subString = udtString::NewSubstringRef(fileData, lineStart);
		u32 lineEnd;
		bool hasEnd = udtString::FindFirstCharacterMatch(lineEnd, subString, '\n');
		const u32 finalEnd = hasEnd ? (lineStart + lineEnd - 1) : fileData.GetLength();
		fileData.GetWritePtr()[finalEnd] = '\0';

		const udtString line = udtString::NewSubstringRef(fileData, lineStart, lineEnd);
		u32 space;
		if(!udtString::FindFirstCharacterMatch(space, line, ' '))
		{
			lineStart += lineEnd + 1;
			continue;
		}
		const u32 finalSpace = lineStart + space;
		fileData.GetWritePtr()[finalSpace] = '\0';

		MapAlias alias;
		alias.NameFound = udtString::NewSubstringRef(fileData, lineStart, (u32)strlen(_persistAllocator.GetStringAt(lineStart)));
		alias.NameToUse = udtString::NewSubstringRef(fileData, finalSpace + 1, (u32)strlen(_persistAllocator.GetStringAt(finalSpace + 1)));
		_mapAliases.Add(alias);

		if(!hasEnd)
		{
			break;
		}

		lineStart += lineEnd + 1;
	}

	return true;
}

bool Viewer::LoadSprites()
{
	udtFileStream file;
	if(!file.Open(DATA_PATH_SPRITES, udtFileOpenMode::Read))
	{
		Platform_PrintError("Failed to open the sprite texture pack file %s", DATA_PATH_SPRITES);
		return false;
	}

	u32 version = 0;
	u32 spriteCount = 0;
	file.Read(&version, 4, 1);
	file.Read(&spriteCount, 4, 1);
	if(spriteCount != Sprite::Count)
	{
		Platform_PrintError("Sprite count mismatch in file %s", DATA_PATH_SPRITES);
		return false;
	}

	for(u32 s = 0; s < spriteCount; ++s)
	{
		udtVMScopedStackAllocator allocScope(_tempAllocator);

		u32 w, h;
		file.Read(&w, 4, 1);
		file.Read(&h, 4, 1);
		u8* const pixels = _tempAllocator.AllocateAndGetAddress(w * h * 4);
		file.Read(pixels, w * h * 4, 1);
		if(!CreateTextureRGBA(_sprites[s], w, h, pixels))
		{
			Platform_PrintError("Failed to create a texture for sprite %d in file %s", (int)s, DATA_PATH_SPRITES);
			return false;
		}
	}

	return true;
}

void Viewer::LoadMap(const udtString& mapName)
{
	_mapWidth = 0;
	_mapHeight = 0;
	_mapCoordsLoaded = false;

	udtVMScopedStackAllocator allocScope(_tempAllocator);

	const udtString mapFileName = udtString::NewFromConcatenating(_tempAllocator, mapName, udtString::NewConstRef(".mapinfo"));
	udtString mapFilePath;
	udtPath::Combine(mapFilePath, _tempAllocator, udtString::NewConstRef(DATA_PATH), mapFileName);
	udtFileStream mapFile;
	if(!mapFile.Open(mapFilePath.GetPtr(), udtFileOpenMode::Read))
	{
		return;
	}

	const udtString imageFileName = udtString::NewFromConcatenating(_tempAllocator, mapName, udtString::NewConstRef(".png"));
	udtString imageFilePath;
	udtPath::Combine(imageFilePath, _tempAllocator, udtString::NewConstRef(DATA_PATH), imageFileName);
	if(!udtFileStream::Exists(imageFilePath.GetPtr()))
	{
		return;
	}

	int w, h, c;
	stbi_uc* const pixels = stbi_load(imageFilePath.GetPtr(), &w, &h, &c, 4);
	if(pixels == nullptr)
	{
		return;
	}

	u32 version = 0;
	mapFile.Read(&version, 4, 1);
	mapFile.Read(_mapMin, 12, 1);
	mapFile.Read(_mapMax, 12, 1);
	_mapCoordsLoaded = true;
	_mapWidth = (u32)w;
	_mapHeight = (u32)h;
	_mapImage = pixels;
}

bool Viewer::CreateTextureRGBA(int& textureId, u32 width, u32 height, const u8* pixels)
{
	const int flags = 0 |
		(int)NVGimageFlags::NVG_IMAGE_REPEATX | 
		(int)NVGimageFlags::NVG_IMAGE_REPEATY |
		(int)NVGimageFlags::NVG_IMAGE_PREMULTIPLIED |
		(int)NVGimageFlags::NVG_IMAGE_GENERATE_MIPMAPS;
	const int newTextureId = nvgCreateImageRGBA(_sharedReadOnly->NVGContext, (int)width, (int)height, flags, pixels);
	if(newTextureId < 0)
	{
		return false;
	}

	textureId = newTextureId;

	return true;
}

void Viewer::LoadDemoImpl(const char* filePath)
{
	_demo.Load(filePath);
	
	const udtString originalMapName = _demo.GetMapName();
	udtString mapName = originalMapName;
	for(u32 i = 0, count = _mapAliases.GetSize(); i < count; ++i)
	{
		if(udtString::EqualsNoCase(_mapAliases[i].NameFound, originalMapName))
		{
			mapName = _mapAliases[i].NameToUse;
			break;
		}
	}

	LoadMap(mapName);
	if(!_mapCoordsLoaded)
	{
		for(u32 i = 0; i < 3; ++i)
		{
			_mapMin[i] = _demo.GetMapMin()[i];
			_mapMax[i] = _demo.GetMapMax()[i];
		}
	}

	_reversePlayback = false;
	_demoPlaybackTimer.Stop();
	_demoPlaybackTimer.Reset();
	_demoPlaybackTimer.Start();
}

void Viewer::StartLoadingDemo(const char* filePath)
{
	DemoLoadThreadData* const data = (DemoLoadThreadData*)malloc(sizeof(DemoLoadThreadData));
	if(data == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for thread job data", (int)sizeof(DemoLoadThreadData));
	}
	data->ViewerPtr = this;
	strcpy(data->FilePath, filePath);
	Platform_NewThread(&DemoLoadThreadEntryPoint, data);
}

void Viewer::ProcessEvent(const Event& event)
{
	if(!_appLoaded)
	{
		return;
	}

	if(event.Type == EventType::Paused)
	{
		_wasTimerRunningBeforePause = _demoPlaybackTimer.IsRunning();
		PausePlayback();
		_appPaused = true;
	}
	else if(event.Type == EventType::Unpaused)
	{	
		if(_wasTimerRunningBeforePause)
		{
			ResumePlayback();
		}
		_appPaused = false;
		_wasTimerRunningBeforePause = false;
	}
	else if(event.Type == EventType::MouseButtonDown)
	{
		_activeWidgets.MouseButtonDown(event.CursorPos[0], event.CursorPos[1], event.MouseButtonId);
		_activeTabWidgets->MouseButtonDown(event.CursorPos[0], event.CursorPos[1], event.MouseButtonId);
	}
	else if(event.Type == EventType::MouseButtonUp)
	{
		_activeWidgets.MouseButtonUp(event.CursorPos[0], event.CursorPos[1], event.MouseButtonId);
		_activeTabWidgets->MouseButtonUp(event.CursorPos[0], event.CursorPos[1], event.MouseButtonId);
	}
	else if(event.Type == EventType::MouseMove)
	{
		_activeWidgets.MouseMove(event.CursorPos[0], event.CursorPos[1]);
		_activeTabWidgets->MouseMove(event.CursorPos[0], event.CursorPos[1]);
	}
	else if(event.Type == EventType::MouseMoveNC)
	{
		_activeWidgets.MouseMoveNC(event.CursorPos[0], event.CursorPos[1]);
		_activeTabWidgets->MouseMoveNC(event.CursorPos[0], event.CursorPos[1]);
	}
	else if(event.Type == EventType::MouseScroll)
	{
		_activeWidgets.MouseScroll(event.CursorPos[0], event.CursorPos[1], event.Scroll);
		_activeTabWidgets->MouseScroll(event.CursorPos[0], event.CursorPos[1], event.Scroll);
	}
	else if(event.Type == EventType::FilesDropped)
	{
		// We don't start a threaded job if one is already in progress.
		CriticalSectionLock lock(_appStateLock);
		if(_appState == AppState::Normal)
		{
			for(u32 i = 0; i < event.DroppedFileCount; ++i)
			{
				if(udtIsValidProtocol(udtGetProtocolByFilePath(event.DroppedFilePaths[i])))
				{
					_appState = AppState::LoadingDemo;
					StartLoadingDemo(event.DroppedFilePaths[i]);
					break;
				}
			}
		}
	}
	else if(event.Type == EventType::KeyDown)
	{
		OnKeyPressed(event.VirtualKeyId, false);
	}
	else if(event.Type == EventType::KeyDownRepeat)
	{
		OnKeyPressed(event.VirtualKeyId, true);
	}

	if(_demoProgressBar.HasDragJustStarted())
	{
		_wasPlayingBeforeProgressDrag = _demoPlaybackTimer.IsRunning();
		Platform_SetCursorCapture(_platform, true);
		PausePlayback();
	}
	else if(_demoProgressBar.HasDragJustEnded())
	{
		Platform_SetCursorCapture(_platform, false);
		if(_wasPlayingBeforeProgressDrag)
		{
			ResumePlayback();
		}
	}

	f32 demoProgress = 0.0f;
	if(_demoProgressBar.HasProgressChanged(demoProgress) && _demo.IsValid())
	{
		SetPlaybackProgress(demoProgress);
	}
	else if(_playPauseButton.WasClicked())
	{
		TogglePlayback();
	}
	else if(_reversePlaybackButton.WasClicked())
	{
		ReversePlayback();
	}
	else if(_stopButton.WasClicked())
	{
		StopPlayback();
	}

	if(_tabButtonGroup.HasSelectionChanged())
	{
		_activeTabWidgets = &_tabWidgets[_tabButtonGroup.GetSelectedIndex()];
	}
}

void Viewer::RenderDemo(const RenderParams& renderParams)
{
	f32 mapWidth;
	f32 mapHeight;
	if(_mapWidth > 0 && _mapHeight > 0)
	{
		mapWidth = (f32)_mapWidth;
		mapHeight = (f32)_mapHeight;
	}
	else
	{
		const f32 minSize = (f32)udt_min(_mapRect.Width(), _mapRect.Height());
		mapWidth = (f32)minSize;
		mapHeight = (f32)minSize;
	}

	const f32 bgImageScaleX = (f32)_mapRect.Width() / mapWidth;
	const f32 bgImageScaleY = (f32)_mapRect.Height() / mapHeight;
	const f32 bgImageScale = udt_min(bgImageScaleX, bgImageScaleY);
	const f32 mapDisplayX = _mapRect.Left();
	const f32 mapDisplayY = _mapRect.Top();
	const f32 mapScaleX = mapWidth / (_mapMax[0] - _mapMin[0]);
	const f32 mapScaleY = mapHeight / (_mapMax[1] - _mapMin[1]);
	const f32 mapScale = udt_min(mapScaleX, mapScaleY) * bgImageScale;

	if(_map != InvalidTextureId)
	{
		NVGcontext* const c = renderParams.NVGContext;
		const f32 x = mapDisplayX;
		const f32 y = mapDisplayY;
		const f32 w = mapWidth * bgImageScale;
		const f32 h = mapHeight * bgImageScale;
		nvgBeginPath(c);
		nvgRect(c, x, y, w, h);
		nvgFillPaint(c, nvgImagePattern(c, x, y, w, h, 0.0f, _map, 1.0f));
		nvgFill(c);
		nvgClosePath(c);
	}

	SpriteDrawParams params;
	params.CoordsScale = mapScale;
	params.ImageScale = bgImageScale;
	params.X = mapDisplayX;
	params.Y = mapDisplayY;

	const Snapshot& snapshot = *_snapshot;
	for(u32 i = 0; i < snapshot.StaticItemCount; ++i)
	{
		// @NOTE: DrawSpriteAt will check if the sprite ID is valid.
		const StaticItem& item = snapshot.StaticItems[i];
		const s32 spriteId = GetSpriteIdFromItemId(item.Id);
		DrawMapSpriteAt(params, (u32)spriteId, item.Position, 16.0f, _config.StaticZScale, 0.0f);
	}

	for(u32 i = 0; i < snapshot.DynamicItemCount; ++i)
	{
		const DynamicItem& item = snapshot.DynamicItems[i];
		if(item.Id == DynamicItemType::ProjectileRocket)
		{
			DrawMapSpriteAt(params, (u32)Sprite::rocket, item.Position, 24.0f, _config.DynamicZScale, item.Angle);
		}
		else if(item.Id == DynamicItemType::ProjectileGrenade)
		{
			f32 pos[3];
			ComputeMapPosition(pos, item.Position, mapScale, _config.DynamicZScale);
			DrawGrenade(renderParams.NVGContext, pos[0], pos[1], 4.0f * bgImageScale * pos[2]);
		}
		else
		{
			// @NOTE: DrawSpriteAt will check if the sprite ID is valid.
			const s32 spriteId = GetSpriteIdFromDynamicItemId(item.Id);
			DrawMapSpriteAt(params, (u32)spriteId, item.Position, 16.0f, _config.StaticZScale, item.Angle);
		}
	}
	
	for(u32 b = 0; b < snapshot.RailBeamCount; ++b)
	{
		const RailBeam& beam = snapshot.RailBeams[b];
		if(beam.Team >= udtTeam::Spectators)
		{
			continue;
		}

		f32 p0[3];
		f32 p1[3];
		ComputeMapPosition(p0, beam.StartPosition, mapScale, 4.0f);
		ComputeMapPosition(p1, beam.EndPosition, mapScale, 4.0f);
		DrawRailBeam(renderParams.NVGContext, p0[0], p0[1], p0[2], p1[0], p1[1], p1[2], beam.Alpha, RailColors[beam.Team]);
	}

	for(u32 p = 0; p < snapshot.PlayerCount; ++p)
	{
		const Player& player = snapshot.Players[p];
		if(IsBitSet(&player.Flags, PlayerFlags::Dead) ||
		   !IsBitSet(&player.Flags, PlayerFlags::Firing) ||
		   player.WeaponId != udtWeapon::LightningGun)
		{
			continue;
		}

		f32 p0[3];
		f32 p1[3];
		ComputeMapPosition(p0, player.Position, mapScale, _config.DynamicZScale);
		ComputeMapPosition(p1, player.LGEndPoint, mapScale, _config.DynamicZScale);
		DrawShaftBeam(renderParams.NVGContext, p0[0], p0[1], p0[2], p1[0], p1[1], p1[2]);
	}
	
	for(u32 p = 0; p < snapshot.PlayerCount; ++p)
	{
		const Player& player = snapshot.Players[p];
		if(IsBitSet(&player.Flags, PlayerFlags::Dead) ||
		   player.Team >= udtTeam::Spectators)
		{
			continue;
		}

		f32 pos[3];
		ComputeMapPosition(pos, player.Position, mapScale, _config.DynamicZScale);
		const bool firing = IsBitSet(&player.Flags, PlayerFlags::Firing);
		const s32 spriteId = GetSpriteIdFromWeaponId(player.WeaponId) + (firing ? 1 : 0);
		if(spriteId >= 0 && spriteId < Sprite::Count)
		{
			DrawPlayerWeapon(renderParams.NVGContext, pos[0], pos[1], 6.0f * bgImageScale * pos[2], -player.Angle + UDT_PI / 2.0f, _sprites[spriteId]);
		}
		const u8 colorIndex = 2 * player.Team + (IsBitSet(&player.Flags, PlayerFlags::Followed) ? 1 : 0);
		DrawPlayer(renderParams.NVGContext, pos[0], pos[1], 6.0f * bgImageScale * pos[2], -player.Angle, PlayerColors[colorIndex]);
		const char* const name = _demo.GetString(player.Name);
		if(name != nullptr)
		{
			DrawPlayerName(renderParams.NVGContext, pos[0], pos[1], 6.0f * bgImageScale * pos[2], name);
		}
	}

	Platform_NVGEndFrame(_platform);
	Platform_NVGBeginFrame(_platform);
	nvgBlendMode(renderParams.NVGContext, NVG_BLEND_MODE_ADDITIVE);
	for(u32 i = 0; i < snapshot.DynamicItemCount; ++i)
	{
		const DynamicItem& item = snapshot.DynamicItems[i];
		if(item.Id == DynamicItemType::ImpactPlasma)
		{
			DrawMapSpriteAt(params, (u32)Sprite::impact_plasma, item.Position, 8.0f, _config.DynamicZScale, 0.0f);
		}
		else if(item.Id == DynamicItemType::ImpactBullet)
		{
			DrawMapSpriteAt(params, (u32)Sprite::impact_bullet_0 + item.SpriteOffset, item.Position, 8.0f, _config.DynamicZScale, 0.0f);
		}
		else if(item.Id == DynamicItemType::ImpactGeneric)
		{
			f32 p[3];
			ComputeMapPosition(p, item.Position, mapScale, _config.DynamicZScale);
			DrawImpact(renderParams.NVGContext, p[0], p[1], 2.0f * bgImageScale * p[2]);
		}
		else if(item.Id == DynamicItemType::Explosion)
		{
			DrawMapSpriteAt(params, (u32)Sprite::explosion_0 + item.SpriteOffset, item.Position, 16.0f, _config.DynamicZScale, 0.0f);
		}
		else if(item.Id == DynamicItemType::ProjectilePlasma)
		{
			DrawMapSpriteAt(params, (u32)Sprite::projectile_plasma, item.Position, 8.0f, _config.DynamicZScale, 0.0f);
		}
	}
	Platform_NVGEndFrame(_platform);
	Platform_NVGBeginFrame(_platform);

	for(u32 p = 0; p < snapshot.PlayerCount; ++p)
	{
		const Player& player = snapshot.Players[p];
		if(!IsBitSet(&player.Flags, PlayerFlags::Dead))
		{
			continue;
		}

		DrawMapSpriteAt(params, (u32)Sprite::dead_player, player.Position, 16.0f, _config.DynamicZScale, 0.0f);
	}

	if(!_drawMapOverlays)
	{
		return;
	}

	if(_drawMapScores)
	{
		RenderDemoScore(renderParams);
	}
	
	if(_drawMapClock)
	{
		RenderDemoTimer(renderParams);
	}
	
	if(_drawMapFollowMsg || _drawMapHealth)
	{
		RenderDemoFollowedPlayer(renderParams);
	}
}

void Viewer::RenderDemoScore(const RenderParams& renderParams)
{
	const Snapshot& snapshot = *_snapshot;
	int score1 = (int)snapshot.Score.Score1;
	int score2 = (int)snapshot.Score.Score2;
	const char* name1;
	const char* name2;
	NVGcolor color1;
	NVGcolor color2;
	if(snapshot.Score.IsScoreTeamBased)
	{
		name1 = "RED";
		name2 = "BLUE";
		color1 = nvgRGB(255, 95, 95);
		color2 = nvgRGB(160, 160, 255);
	}
	else
	{
		name1 = _demo.GetStringSafe(snapshot.Score.Score1Name, "?");
		name2 = _demo.GetStringSafe(snapshot.Score.Score2Name, "?");
		color1 = nvgGrey(0);
		color2 = nvgGrey(0);

		// For display consistency, we always keep the lowest client number first
		// in duel and HoonyMode because they are the only 2 game types 
		// that guarantee you have exactly 2 players.
		const udtGameType::Id gt = _demo.GetGameType();
		if((gt == udtGameType::Duel || gt == udtGameType::HM) &&
		   snapshot.Score.Score1Id > snapshot.Score.Score2Id)
		{
			const char* const nameTemp = name2;
			const int scoreTemp = score2;
			name2 = name1;
			score2 = score1;
			name1 = nameTemp;
			score1 = scoreTemp;
		}
	}

	char scoreString1[16];
	char scoreString2[16];
	sprintf(scoreString1, "%d", score1);
	sprintf(scoreString2, "%d", score2);

	NVGcontext* const ctx = renderParams.NVGContext;
	const f32 space = 4.0f;
	const f32 fontSize = 20.0f;
	const f32 dashw = 5.0f;
	const f32 dashh = 2.0f;
	nvgFontSize(ctx, fontSize);
	float bounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, name1, nullptr, bounds);
	const f32 name1w = bounds[2] - bounds[0];
	nvgTextBounds(ctx, 0.0f, 0.0f, name2, nullptr, bounds);
	const f32 name2w = bounds[2] - bounds[0];
	nvgTextBounds(ctx, 0.0f, 0.0f, scoreString1, nullptr, bounds);
	const f32 score1w = bounds[2] - bounds[0];
	nvgTextBounds(ctx, 0.0f, 0.0f, scoreString2, nullptr, bounds);
	const f32 score2w = bounds[2] - bounds[0];
	
	// Top-left corner.
	const f32 x = _mapRect.Right() - name1w - name2w - score1w - score2w - 6.0f * space;
	const f32 y = _mapRect.Y() + space;

	nvgBeginPath(ctx);
	nvgFillColor(ctx, color1);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
	nvgText(ctx, x, y, name1, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, color2);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
	nvgText(ctx, x + name1w + score1w + dashw + score2w + 4.0f * space, y, name2, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGrey(0));
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
	nvgText(ctx, x + name1w + space, y, scoreString1, nullptr);
	nvgText(ctx, x + name1w + score1w + dashw + 3.0f * space, y, scoreString2, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGrey(127));
	nvgRect(ctx, x + name1w + score1w + 2.0f * space, floorf(y + fontSize / 2.0f), dashw, dashh);
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::RenderDemoTimer(const RenderParams& renderParams)
{
	const int extraSec = _timerShowsServerTime ? ((int)_demo.GetFirstSnapshotTimeMs() / 1000) : 0;
	const int totalSec = (int)_demoPlaybackTimer.GetElapsedSec() + extraSec;
	const int minutes = totalSec / 60;
	const int seconds = totalSec % 60;

	char clock[256];
	sprintf(clock, "%d:%02d", minutes, seconds);

	const f32 space = 4.0f;
	const f32 x = floorf(_mapRect.Left()) + space;
	const f32 y = ceilf(_mapRect.Top()) + space;

	NVGcontext* const ctx = renderParams.NVGContext;
	nvgBeginPath(ctx);
	nvgFontSize(ctx, 20.0f);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
	nvgFillColor(ctx, nvgGrey(0));
	nvgText(ctx, x, y, clock, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::RenderDemoFollowedPlayer(const RenderParams& renderParams)
{
	const Snapshot& snapshot = *_snapshot;
	if(snapshot.Core.FollowedTeam != udtTeam::Free &&
	   snapshot.Core.FollowedTeam != udtTeam::Red && 
	   snapshot.Core.FollowedTeam != udtTeam::Blue)
	{
		return;
	}
	
	const f32 space = 4.0f;
	const f32 fontSize = 20.0f;
	NVGcontext* const ctx = renderParams.NVGContext;
	nvgFontSize(ctx, fontSize);

	if(_drawMapFollowMsg)
	{
		const char* const name = _demo.GetString(snapshot.Core.FollowedName);
		if(name != nullptr)
		{
			char msg[128];
			sprintf(msg, "Following %s", name);

			nvgBeginPath(ctx);
			nvgFillColor(ctx, nvgGrey(0));
			nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_BOTTOM);
			nvgText(ctx, _mapRect.Left() + space, _mapRect.Bottom() - space, msg, nullptr);
			nvgFill(ctx);
			nvgClosePath(ctx);
		}
	}

	if(!_drawMapHealth)
	{
		return;
	}

	float bounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, "-000", nullptr, bounds);
	const f32 textw = ceilf(bounds[2] - bounds[0]);
	const f32 iconw = fontSize;

	// Top left corner.
	const f32 delta = 4.0f;
	const f32 w = 3.0f * (iconw + textw) + 5.0f * delta;
	const f32 h = fontSize + space;
	const f32 x = ceilf(_mapRect.Right() - w);
	const f32 y = _mapRect.Bottom() - h;

	if(_map != InvalidTextureId)
	{
		const f32 r = space;
		nvgBeginPath(ctx);
		nvgFillColor(ctx, nvgRGBf(ViewerClearColor[0], ViewerClearColor[1], ViewerClearColor[2]));
		nvgRoundedRect(ctx, x - r, y - r, w + 2.0f * r, h + 2.0f * r, r);
		nvgFill(ctx);
		nvgClosePath(ctx);
	}

	const s32 ammoSpriteId = GetIconSpriteIdFromWeaponId(snapshot.Core.FollowedWeapon);
	DrawTexturedRect(ctx, x, y, iconw, iconw, _sprites[Sprite::armor_red]);
	DrawTexturedRect(ctx, x + iconw + textw + 2.0f * delta, y, iconw, iconw, _sprites[Sprite::health_large]);
	if(ammoSpriteId >= 0)
	{
		DrawTexturedRect(ctx, x + 2.0f * (iconw + textw) + 4.0f * delta, y, iconw, iconw, _sprites[ammoSpriteId]);
	}

	char armor[16];
	char health[16];
	char ammo[16];
	sprintf(armor, "%d", snapshot.Core.FollowedArmor);
	sprintf(health, "%d", snapshot.Core.FollowedHealth);
	sprintf(ammo, "%d", snapshot.Core.FollowedAmmo);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGrey(0));
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_RIGHT | NVGalign::NVG_ALIGN_TOP);
	nvgText(ctx, x + iconw + textw + delta, y, armor, nullptr);
	nvgText(ctx, x + 2.0f * (iconw + textw) + 3.0f * delta, y, health, nullptr);
	if(snapshot.Core.FollowedWeapon != udtWeapon::Gauntlet &&
	   snapshot.Core.FollowedWeapon != udtWeapon::GrapplingHook)
	{
		nvgText(ctx, x + 3.0f * (iconw + textw) + 5.0f * delta, y, ammo, nullptr);
	}
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::RenderNoDemo(const RenderParams& renderParams)
{
	const char* const message = "drag'n'drop a demo file anywhere in the window to load it";
	NVGcontext* const ctx = renderParams.NVGContext;
	nvgBeginPath(ctx);
	nvgFontSize(ctx, 24.0f);
	f32 bounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, message, nullptr, bounds);
	const f32 x = floorf(((f32)renderParams.ClientWidth - bounds[2]) / 2.0f);
	const f32 y = floorf(((f32)renderParams.ClientHeight - bounds[3]) / 2.0f);
	nvgText(ctx, x, y, message, nullptr);
	nvgFillColor(ctx, nvgGrey(255));
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::RenderDemoLoadProgress(const RenderParams& renderParams)
{
	const f32 cx = f32(renderParams.ClientWidth / 2);
	const f32 cy = f32(renderParams.ClientHeight / 2);
	const f32 w = 200.0f;
	const f32 h = 10.0f;
	const f32 r = 5.0f;
	const f32 x = cx - floorf(w / 2.0f);
	const f32 y = cy - floorf(h / 2.0f) + 10.0f;
	NVGcontext* const ctx = renderParams.NVGContext;

	nvgBeginPath(ctx);
	nvgFontSize(ctx, 24.0f);
	nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_BOTTOM);
	nvgText(ctx, cx, cy - 10.0f, "loading demo...", nullptr);
	nvgFillColor(ctx, nvgGrey(255));
	nvgFill(ctx);
	nvgClosePath(ctx);

	DrawProgressBar(ctx, x, y, w, h, r, _demoLoadProgress);

	const f32 icono = 8.0f;
	const f32 iconx = 20.0f;
	const f32 icony = 20.0f;
	const f32 icona = (f32)_globalTimer.GetElapsedMs() / 1000.0f;
	nvgResetTransform(ctx);
	nvgTranslate(ctx, iconx, icony);
	nvgScale(ctx, 1.5f, 1.5f);
	nvgRotate(ctx, icona);
	bndIcon(ctx, -icono, -icono, BND_ICONID(2, 15));
	nvgResetTransform(ctx);
}

void Viewer::Render(const RenderParams& renderParams)
{
	if(!_appLoaded)
	{
		return;
	}

	CriticalSectionLock appStateLock(_appStateLock);

	if(_appState == AppState::UploadingMapTexture)
	{
		UploadMapTexture();
		_appState = AppState::Normal;
		RenderDemoLoadProgress(renderParams);
		return;
	}

	if(_appState == AppState::LoadingDemo)
	{
		RenderDemoLoadProgress(renderParams);
		return;
	}

	if(_demo.GetSnapshotCount() == 0)
	{
		RenderNoDemo(renderParams);
		return;
	}

	u32 snapshotIndex;
	s32 serverTimeMs;
	GetCurrentSnapshotIndexAndServerTime(snapshotIndex, serverTimeMs);
	if(!_demo.GetSnapshotData(*_snapshot, snapshotIndex))
	{
		RenderNoDemo(renderParams);
		return;
	}
	_snapshotIndex = snapshotIndex;

	const f32 progressBarMargin = 2.0f;
	const f32 progressBarHeight = (f32)BND_WIDGET_HEIGHT;
	const f32 progressY = (f32)renderParams.ClientHeight - progressBarHeight - 2.0f * progressBarMargin;
	_progressRect.Set(progressBarMargin, progressY, (f32)renderParams.ClientWidth - 2.0f * progressBarMargin, progressBarHeight);
	const f32 mapAspectRatio = (_mapWidth > 0 && _mapHeight > 0) ? ((f32)_mapWidth / (f32)_mapHeight) : 1.0f;
	const f32 mapHeight = progressY - progressBarMargin;
	const f32 mapWidth = ceilf(mapHeight * mapAspectRatio);
	const f32 uiOffsetX = 20.0f;
	const f32 uiOffsetY = 20.0f;
	_mapRect.Set(0.0f, 0.0f, mapWidth, mapHeight);
	_tabBarRect.Set(mapWidth + uiOffsetX, 0.0f, (f32)renderParams.ClientWidth - mapWidth - uiOffsetX, (f32)BND_WIDGET_HEIGHT);
	_uiRect.Set(mapWidth + uiOffsetX, _tabBarRect.Bottom() + uiOffsetY, (f32)renderParams.ClientWidth - mapWidth - uiOffsetX, mapHeight - _tabBarRect.Height() - uiOffsetY);

	RenderDemo(renderParams);

	const f32 bw = (f32)BND_TOOL_WIDTH;
	const f32 bh = (f32)BND_WIDGET_HEIGHT;
	f32 x0 = _progressRect.Left();
	f32 y0 = _progressRect.Top() + floorf((_progressRect.Height() - bh) / 2.0f);
	_playPauseButton.SetRect(x0, y0, bw, bh); x0 += bw - 1.0f;
	_stopButton.SetRect(x0, y0, bw, bh); x0 += bw - 1.0f;
	_reversePlaybackButton.SetRect(x0, y0, bw, bh); x0 += bw;

	const f32 x = x0;
	const f32 r = floorf(progressBarHeight / 2.0f);
	const f32 ph = (f32)BND_SCROLLBAR_HEIGHT;
	const f32 y = _progressRect.Top() + floorf((_progressRect.Height() - ph) / 2.0f);
	_demoProgressBar.SetRect(x + 2.0f * r, y, _progressRect.Width() - x - 4.0f * r, 2.0f * r);
	_demoProgressBar.SetRadius(r);
	_demoProgressBar.SetProgress(GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs()));

	const f32 tw = 80.0f;
	const f32 th = (f32)BND_WIDGET_HEIGHT;
	const f32 ty = _tabBarRect.Y();
	f32 tx = _tabBarRect.X();
	for(u32 i = 0; i < (u32)Tab::Count; ++i)
	{
		_tabButtons[i].SetRect(tx, ty, tw, th);
		tx += tw - 1.0f;
	}

	NVGcontext* const ctx = renderParams.NVGContext;
	const f32 oo = (f32)BND_WIDGET_HEIGHT + 4.0f;
	const f32 ox = _uiRect.X();
	f32 oy = _uiRect.Y();
	_drawMapOverlaysCheckBox.SetRect(ctx, ox, oy); oy += oo;
	_drawMapScoresCheckBox.SetRect(ctx, ox, oy); oy += oo;
	_drawMapClockCheckBox.SetRect(ctx, ox, oy); oy += oo;
	_drawMapFollowMsgCheckBox.SetRect(ctx, ox, oy); oy += oo;
	_drawMapHealthCheckBox.SetRect(ctx, ox, oy); oy += oo;
	_showServerTimeCheckBox.SetRect(ctx, ox, oy); oy += oo;

	_activeWidgets.Draw(renderParams.NVGContext);
	_activeTabWidgets->Draw(renderParams.NVGContext);

	if(_tabButtonGroup.GetSelectedIndex() == Tab::Chat)
	{
		DrawChat(renderParams, serverTimeMs);
	}

	if(_demoProgressBar.IsHovered())
	{
		DrawProgressSliderToolTip(renderParams);
	}

	if(_displayHelp)
	{
		DrawHelp(renderParams);
	}
}

void Viewer::DrawProgressSliderToolTip(const RenderParams& renderParams)
{
	s32 cx, cy;
	Platform_GetCursorPosition(_platform, cx, cy);

	f32 x, y, w, h;
	_demoProgressBar.GetRect(x, y, w, h);

	const f32 progress = udt_clamp(((f32)cx - x) / w, 0.0f, 1.0f);
	const int extraSec = _timerShowsServerTime ? ((int)_demo.GetFirstSnapshotTimeMs() / 1000) : 0;
	const int totalSec = (int)(GetTimeFromProgress(progress) / 1000) + extraSec;
	const int minutes = totalSec / 60;
	const int seconds = totalSec % 60;

	char timeStamp[256];
	sprintf(timeStamp, "%d:%02d", minutes, seconds);

	NVGcontext* const ctx = renderParams.NVGContext;
	nvgFontSize(ctx, 12.0f);

	float bounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, timeStamp, nullptr, bounds);

	const f32 textm = 2.0f;
	const f32 textw = bounds[2] - bounds[0];
	const f32 texth = bounds[3] - bounds[1];
	const f32 textx = floorf((f32)cx - textw / 2.0f);
	const f32 texty = y - 4.0f - textm - texth;
	const f32 bgx = textx - textm;
	const f32 bgy = texty - textm;
	const f32 bgw = textw + 2.0f * textm;
	const f32 bgh = texth + 2.0f * textm;

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgRGB(255, 255, 191));
	nvgRect(ctx, bgx, bgy, bgw, bgh);
	nvgFill(ctx);
	nvgClosePath(ctx);

	nvgBeginPath(ctx);
	nvgStrokeColor(ctx, nvgGrey(0));
	nvgRect(ctx, bgx + 0.375f, bgy + 0.375f, bgw - 1.0f, bgh - 1.0f);
	nvgStrokeWidth(ctx, 1.0f);
	nvgStroke(ctx);
	nvgClosePath(ctx);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGrey(0));
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
	nvgText(ctx, textx, texty, timeStamp, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::DrawHelp(const RenderParams& renderParams)
{
	NVGcontext* const ctx = renderParams.NVGContext;

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGreyA(0, 160));
	nvgRect(ctx, 0.0f, 0.0f, (f32)renderParams.ClientWidth, (f32)renderParams.ClientHeight);
	nvgFill(ctx);
	nvgClosePath(ctx);

	const u32 stringCount = (u32)UDT_COUNT_OF(HelpBindStrings);
	const u32 lineCount = stringCount / 2;
	const f32 w1 = 90.0f;
	const f32 w2 = 280.0f;
	const f32 w = w1 + w2 - 1.0f;
	const f32 h = (f32)(lineCount * BND_WIDGET_HEIGHT);

	f32 x = floorf(((f32)renderParams.ClientWidth - w) / 2.0f);
	f32 y = floorf(((f32)renderParams.ClientHeight - h) / 2.0f);
	for(u32 i = 0; i < stringCount; i += 2)
	{
		int flags = BND_CORNER_DOWN | BND_CORNER_TOP;
		if(i == 0)
		{
			flags = BND_CORNER_DOWN;
		}
		else if(i == stringCount - 2)
		{
			flags = BND_CORNER_TOP;
		}
		const BNDwidgetState state = i == 0 ? BND_ACTIVE : BND_DEFAULT;
		bndTextField(ctx, x, y, w1, BND_WIDGET_HEIGHT, BND_CORNER_RIGHT | flags, state, -1, HelpBindStrings[i], 1, 0);
		bndTextField(ctx, x + w1 - 1.0f, y, w2, BND_WIDGET_HEIGHT, BND_CORNER_LEFT | flags, state, -1, HelpBindStrings[i + 1], 1, 0);
		y += BND_WIDGET_HEIGHT - 2.0f;
	}
}

void Viewer::DrawChat(const RenderParams& renderParams, s32 serverTimeMs)
{
	const u32 index = _demo.GetChatMessageIndexFromServerTime(serverTimeMs);
	if(index == UDT_U32_MAX)
	{
		return;
	}

	NVGcontext* const ctx = renderParams.NVGContext;
	const f32 fontSize = 16.0f;
	const f32 lineHeight = 20.0f;
	nvgFontSize(ctx, fontSize);

	const f32 top = _uiRect.Top();
	const f32 x = _uiRect.Left();
	f32 y = _uiRect.Bottom();

	u32 i = index;
	ChatMessage message;
	while(_demo.GetChatMessage(message, i--))
	{
		if(y <= top)
		{
			break;
		}

		const int totalSec = (int)message.ServerTimeMs / 1000;
		const int minutes = totalSec / 60;
		const int seconds = totalSec % 60;
		const char* const name = _demo.GetString(message.PlayerName);
		const char* const msg = _demo.GetString(message.Message);
		const char* const loc = _demo.GetString(message.Location);

		char line[512];
		if(message.TeamMessage)
		{
			if(!udtString::IsNullOrEmpty(loc))
			{
				sprintf(line, "[%d:%02d] (%s) (%s): %s", minutes, seconds, name, loc, msg);
			}
			else
			{
				sprintf(line, "[%d:%02d] (%s): %s", minutes, seconds, name, msg);
			}
		}
		else
		{
			sprintf(line, "[%d:%02d] %s: %s", minutes, seconds, name, msg);
		}

		// @TODO: display with line breaks?
		nvgBeginPath(ctx);
		nvgFillColor(ctx, nvgGrey(0));
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_BOTTOM);
		nvgText(ctx, x, y, line, nullptr);
		nvgFill(ctx);
		nvgClosePath(ctx);

		y -= lineHeight;
	}
}

void Viewer::DrawMapSpriteAt(const SpriteDrawParams& params, u32 spriteId, const f32* pos, f32 size, f32 zScale, f32 a)
{
	if(spriteId >= (u32)Sprite::Count)
	{
		return;
	}

	f32 p[3];
	ComputeMapPosition(p, pos, params.CoordsScale, zScale);
	const f32 w = size * params.ImageScale * p[2];
	const f32 h = size * params.ImageScale * p[2];
	NVGcontext* const c = _sharedReadOnly->NVGContext;

	nvgBeginPath(c);
	nvgResetTransform(c);
	nvgTranslate(c, p[0], p[1]);
	nvgScale(c, w, h);
	nvgRotate(c, a);
	nvgRect(c, -0.5f, -0.5f, 1.0f, 1.0f);
	nvgFillPaint(c, nvgImagePattern(c, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, _sprites[spriteId], 1.0f));
	nvgFill(c);
	nvgClosePath(c);
	nvgResetTransform(c);
}

void Viewer::GetCurrentSnapshotIndexAndServerTime(u32& snapshotIndex, s32& serverTimeMs)
{
	const f32 progress = GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
	serverTimeMs = _demo.GetFirstSnapshotTimeMs() + (s32)(progress * (f32)_demo.GetDurationMs());
	snapshotIndex = _demo.GetSnapshotIndexFromServerTime(serverTimeMs);
}

f32 Viewer::GetProgressFromTime(u32 elapsedMs)
{
	const f32 rawProgress = udt_min((f32)elapsedMs / (f32)_demo.GetDurationMs(), 1.0f);
	const f32 progress = _reversePlayback ? (1.0f - rawProgress) : rawProgress;

	return progress;
}

u32 Viewer::GetTimeFromProgress(f32 rawProgress)
{
	const f32 progress = _reversePlayback ? (1.0f - rawProgress) : rawProgress;
	const u32 timeElapsedMs = (u32)(progress * (f32)_demo.GetDurationMs());

	return timeElapsedMs;
}

void Viewer::PausePlayback()
{
	if(_demoPlaybackTimer.IsRunning())
	{
		_demoPlaybackTimer.Stop();
	}
}

void Viewer::ResumePlayback()
{
	_demoPlaybackTimer.Start();
}

void Viewer::TogglePlayback()
{
	if(_demoPlaybackTimer.IsRunning())
	{
		PausePlayback();
	}
	else
	{
		ResumePlayback();
	}
}

void Viewer::StopPlayback()
{
	_demoPlaybackTimer.Reset();
	_reversePlayback = false;
}

void Viewer::ReversePlayback()
{
	const f32 progress = GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
	_reversePlayback = !_reversePlayback;
	SetPlaybackProgress(progress);
}

void Viewer::SetPlaybackProgress(f32 progress)
{
	progress = udt_clamp(progress, 0.0f, 1.0f);
	const bool wasRunning = _demoPlaybackTimer.IsRunning();
	_demoPlaybackTimer.Reset();
	_demoPlaybackTimer.SetElapsedMs(GetTimeFromProgress(progress));
	if(wasRunning)
	{
		_demoPlaybackTimer.Start();
	}
}

void Viewer::OnKeyPressed(VirtualKey::Id virtualKeyId, bool repeat)
{
	// Actions with repeat allowed.
	switch(virtualKeyId)
	{
		case VirtualKey::LeftArrow:
			OffsetSnapshot(-1);
			break;

		case VirtualKey::RightArrow:
			OffsetSnapshot(1);
			break;

		case VirtualKey::UpArrow:
			OffsetTimeMs(1000);
			break;

		case VirtualKey::DownArrow:
			OffsetTimeMs(-1000);
			break;

		case VirtualKey::PageUp:
			OffsetTimeMs(10000);
			break;

		case VirtualKey::PageDown:
			OffsetTimeMs(-10000);
			break;

		default:
			break;
	}

	if(repeat)
	{
		return;
	}

	// Actions with repeat not allowed.
	switch(virtualKeyId)
	{
		case VirtualKey::Escape:
			Platform_RequestQuit(_platform);
			break;

		case VirtualKey::Space:
		case VirtualKey::P:
			TogglePlayback();
			break;

		case VirtualKey::R:
			ReversePlayback();
			break;

		case VirtualKey::S:
			StopPlayback();
			break;

		case VirtualKey::F:
			Platform_ToggleMaximized(_platform);
			break;

		case VirtualKey::F1:
			_displayHelp = !_displayHelp;
			break;

		default:
			break;
	}
}

void Viewer::OffsetSnapshot(s32 snapshotCount)
{
	if(snapshotCount == 0)
	{
		return;
	}

	PausePlayback();
	const s32 newSnapOffset = (snapshotCount > 0) ? (2 * snapshotCount) : 0;
	const u32 newSnapIndex1 = (u32)udt_clamp((s32)_snapshotIndex + snapshotCount, 0, (s32)_demo.GetSnapshotCount() - 1);
	const u32 newSnapIndex2 = (u32)udt_clamp((s32)_snapshotIndex + newSnapOffset, 0, (s32)_demo.GetSnapshotCount() - 1);
	const s32 newServerTimeMs1 = _demo.GetSnapshotServerTimeMs(newSnapIndex1);
	const s32 newServerTimeMs2 = _demo.GetSnapshotServerTimeMs(newSnapIndex2);
	const s32 newServerTimeMs = (newServerTimeMs1 + newServerTimeMs2) / 2;
	const f32 newProgress = (f32)(newServerTimeMs - _demo.GetFirstSnapshotTimeMs()) / (f32)_demo.GetDurationMs();
	SetPlaybackProgress(newProgress);
}

void Viewer::OffsetTimeMs(s32 durationMs)
{
	if(durationMs == 0)
	{
		return;
	}

	const f32 progress = GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
	const f32 progressDeltaAbs = GetProgressFromTime((u32)abs((int)durationMs));
	const f32 newProgress = durationMs >= 0 ? (progress + progressDeltaAbs) : (progress - progressDeltaAbs);
	SetPlaybackProgress(newProgress);
}

void Viewer::ComputeMapPosition(f32* result, const f32* input, f32 mapScale, f32 zScale)
{
	result[0] = _mapRect.Left() + (input[0] - _mapMin[0]) * mapScale;
	result[1] = _mapRect.Bottom() - (input[1] - _mapMin[1]) * mapScale;
	result[2] = 1.0f + zScale * ((input[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
}

void Viewer::UploadMapTexture()
{
	if(_map != InvalidTextureId)
	{
		nvgDeleteImage(_sharedReadOnly->NVGContext, _map);
		_map = InvalidTextureId;
	}

	if(_mapImage == nullptr)
	{
		return;
	}
	
	if(!_mapCoordsLoaded || 
	   _mapWidth == 0 || 
	   _mapHeight == 0)
	{
		stbi_image_free(_mapImage);
		_mapImage = nullptr;
		return;
	}

	int mapTextureId = 0;
	const bool validTexture = CreateTextureRGBA(mapTextureId, _mapWidth, _mapHeight, (const u8*)_mapImage);
	stbi_image_free(_mapImage);
	_mapImage = nullptr;
	if(validTexture)
	{
		_map = mapTextureId;
	}
	else
	{
		_mapCoordsLoaded = false;
		_mapWidth = 0;
		_mapHeight = 0;
	}
}
