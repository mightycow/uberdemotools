#include "viewer.hpp"
#include "file_stream.hpp"
#include "math.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "nanovg_drawing.hpp"
#include "log.hpp"
#include "thread_local_allocators.hpp"
#include "nanovg/fontstash.h"
#include "nanovg/stb_image.h"
#include "blendish/blendish.h"

#include <stdlib.h>
#include <math.h>


#define  DATA_PATH          "viewer_data"
#define  DATA_PATH_ICONS    DATA_PATH"/blender_icons.png"
#define  DATA_PATH_FONT     DATA_PATH"/deja_vu_sans.ttf"
#define  DATA_PATH_ALIASES  DATA_PATH"/map_aliases.txt"
#define  DATA_PATH_SPRITES  DATA_PATH"/sprites.texturepack"
#define  CONFIG_PATH        "viewerconfig.cfg"


static const char* const ViewerVersionString = "0.1.2";

static const char* const LogSeparator = "^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^    ^";

static const char* HelpBindStrings[] =
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

static const char* HelpCreditStrings[] =
{
	"CODE / ASSETS", "DESCRIPTION", "AUTHORS",
	"UDT 2D Viewer", ViewerVersionString, "myT",
	"UDT Library", udtGetVersionString(), "myT",
	"NanoVG", "Vector graphics library", "Mikko Mononen",
	"NanoVG", "Direct3D 11 port", "Chris Maughan",
	"Blendish", "UI drawing functions", "Leonard Ritter",
	"stb_image", "Image library", "Sean Barrett",
	"stb_truetype", "TrueType library", "Sean Barrett",
	"Quake 3 textures", "", "id Software",
	"Blender icons", "", "The Blender Foundation",
	"DejaVu", "Font", "The DejaVu team",
	"Icons", "", "Memento_Mori",
	"Viz data", "", "Memento_Mori, Akuma, myT"
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

static const NVGcolor LogColors[Log::Level::Count] =
{
	nvgGrey(0),
	nvgRGB(255, 127, 0),
	nvgRGB(255, 0, 0)
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

// The sum of all kernel coefficients should be 256.
static void ConvolveRGBAImage(u8* output, const u8* input, u32 width, u32 height, const u8* kernelCoeffs, u32 kernelSize)
{
	const u32 ks2 = kernelSize / 2;
	const u32 x1 = width - ks2;
	const u32 y1 = height - ks2;

	for(u32 y = 0; y < ks2; ++y)
	{
		for(u32 x = 0; x < width; ++x)
		{
			const u32 idx = y * width + x;
			output[idx * 4 + 0] = input[idx * 4 + 0];
			output[idx * 4 + 1] = input[idx * 4 + 1];
			output[idx * 4 + 2] = input[idx * 4 + 2];
			output[idx * 4 + 3] = input[idx * 4 + 3];
		}
	}

	for(u32 y = ks2; y < y1; ++y)
	{
		for(u32 x = 0; x < ks2; ++x)
		{
			const u32 idx = y * width + x;
			output[idx * 4 + 0] = input[idx * 4 + 0];
			output[idx * 4 + 1] = input[idx * 4 + 1];
			output[idx * 4 + 2] = input[idx * 4 + 2];
			output[idx * 4 + 3] = input[idx * 4 + 3];
		}

		for(u32 x = ks2; x < x1; ++x)
		{
			u16 sumR = 0;
			u16 sumG = 0;
			u16 sumB = 0;
			for(u32 ky = 0; ky < kernelSize; ++ky)
			{
				for(u32 kx = 0; kx < kernelSize; ++kx)
				{
					const u32 inIdx = (y + ky - ks2) * width + (x + kx - ks2);
					const u32 kernIdx = ky * kernelSize + kx;
					const u8 r = input[inIdx * 4 + 0];
					const u8 g = input[inIdx * 4 + 1];
					const u8 b = input[inIdx * 4 + 2];
					const u8 k = kernelCoeffs[kernIdx];
					sumR += (u16)r * (u16)k;
					sumG += (u16)g * (u16)k;
					sumB += (u16)b * (u16)k;
				}
			}

			const u32 idx = y * width + x;
			output[idx * 4 + 0] = sumR >> 8;
			output[idx * 4 + 1] = sumG >> 8;
			output[idx * 4 + 2] = sumB >> 8;
			output[idx * 4 + 3] = input[idx * 4 + 3];
		}

		for(u32 x = x1; x < width; ++x)
		{
			const u32 idx = y * width + x;
			output[idx * 4 + 0] = input[idx * 4 + 0];
			output[idx * 4 + 1] = input[idx * 4 + 1];
			output[idx * 4 + 2] = input[idx * 4 + 2];
			output[idx * 4 + 3] = input[idx * 4 + 3];
		}
	}

	for(u32 y = y1; y < height; ++y)
	{
		for(u32 x = 0; x < width; ++x)
		{
			const u32 idx = y * width + x;
			output[idx * 4 + 0] = input[idx * 4 + 0];
			output[idx * 4 + 1] = input[idx * 4 + 1];
			output[idx * 4 + 2] = input[idx * 4 + 2];
			output[idx * 4 + 3] = input[idx * 4 + 3];
		}
	}
}

static void SliderFormatZScale(char* buffer, f32 value, f32, f32)
{
	sprintf(buffer, "%.2f", 1.0f + value);
}

static void SliderFormatScale(char* buffer, f32 value, f32, f32)
{
	sprintf(buffer, "%.2f", value);
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
	viewer->_threadedJobProgress = progress;
}

void Viewer::DemoLoadThreadEntryPoint(void* userData)
{
	const DemoLoadThreadData& data = *(const DemoLoadThreadData*)userData;
	Viewer* const viewer = data.ViewerPtr;
	viewer->LoadDemo(data.FilePath);
	free(userData);
	CriticalSectionLock lock(viewer->_appStateLock);
	viewer->_appState = AppState::FinishDemoLoading;
}

void Viewer::HeatMapsGenThreadEntryPoint(void* userData)
{
	const HeatMapsGenThreadData& data = *(const HeatMapsGenThreadData*)userData;
	Viewer* const viewer = data.ViewerPtr;
	viewer->GenerateHeatMaps();
	free(userData);
	CriticalSectionLock lock(viewer->_appStateLock);
	viewer->_appState = AppState::FinishHeatMapGeneration;
}

Viewer::Viewer(Platform& platform)
	: _platform(platform)
{
	Platform_GetSharedDataPointers(platform, (const PlatformReadOnly**)&_sharedReadOnly, &_sharedReadWrite);
}

Viewer::~Viewer()
{
	Platform_ReleaseCriticalSection(_appStateLock);
	free(_snapshot);
}

bool Viewer::Init(int argc, char** argv)
{
	if(!LoadConfig(_config, CONFIG_PATH))
	{
		SaveConfig(_config, CONFIG_PATH);
	}

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
	_showServerTimeCheckBox.SetActivePtr(&_config.TimerShowsServerTime);
	_showServerTimeCheckBox.SetText("Clock shows server time");
	_drawMapOverlaysCheckBox.SetActivePtr(&_config.DrawMapOverlays);
	_drawMapOverlaysCheckBox.SetText("Enable map overlays");
	_drawMapScoresCheckBox.SetActivePtr(&_config.DrawMapScores);
	_drawMapScoresCheckBox.SetText("Draw scores");
	_drawMapClockCheckBox.SetActivePtr(&_config.DrawMapClock);
	_drawMapClockCheckBox.SetText("Draw clock");
	_drawMapFollowMsgCheckBox.SetActivePtr(&_config.DrawMapFollowMsg);
	_drawMapFollowMsgCheckBox.SetText("Draw follow message");
	_drawMapHealthCheckBox.SetActivePtr(&_config.DrawMapHealth);
	_drawMapHealthCheckBox.SetText("Draw followed player status bar");
	_genHeatMapsButton.SetText("Generate Heat Maps");
	_heatMapOpacitySlider.SetValuePtr(&_config.HeatMapOpacity);
	_heatMapOpacitySlider.SetText("Opacity");
	_heatMapSquaredRampCheckBox.SetActivePtr(&_config.HeatMapSquaredRamp);
	_heatMapSquaredRampCheckBox.SetText("Bias towards heat (quadratic)");
	_drawHeatMapOnlyCheckBox.SetActivePtr(&_config.DrawHeatMapOnly);
	_drawHeatMapOnlyCheckBox.SetText("Hide entities when the heat map is visible");
	_drawHeatMapCheckBox.SetActivePtr(&_config.DrawHeatMap);
	_drawHeatMapCheckBox.SetText("Draw heat map");
	_onlyFirstMatchCheckBox.SetActivePtr(&_config.OnlyKeepFirstMatchSnapshots);
	_onlyFirstMatchCheckBox.SetText("Only keep snapshots from the first full match (when available)");
	_removeTimeOutsCheckBox.SetActivePtr(&_config.RemoveTimeOutSnapshots);
	_removeTimeOutsCheckBox.SetText("Remove time-out snapshots");
	_staticZScaleSlider.SetText("Static Depth Scale");
	_staticZScaleSlider.SetFormatter(&SliderFormatZScale);
	_staticZScaleSlider.SetValuePtr(&_config.StaticZScale);
	_dynamicZScaleSlider.SetText("Dynamic Depth Scale");
	_dynamicZScaleSlider.SetFormatter(&SliderFormatZScale);
	_dynamicZScaleSlider.SetValuePtr(&_config.DynamicZScale);
	_globalScaleSlider.SetText("Global Scale");
	_globalScaleSlider.SetRange(0.5f, 2.0f);
	_globalScaleSlider.SetFormatter(&SliderFormatScale);
	_globalScaleSlider.SetValuePtr(&_config.GlobalScale);
	_reloadDemoButton.SetText("Reload Demo");
	
	WidgetGroup& options = _tabWidgets[Tab::Options];
	options.AddWidget(&_showServerTimeCheckBox);
	options.AddWidget(&_drawMapOverlaysCheckBox);
	options.AddWidget(&_drawMapScoresCheckBox);
	options.AddWidget(&_drawMapClockCheckBox);
	options.AddWidget(&_drawMapFollowMsgCheckBox);
	options.AddWidget(&_drawMapHealthCheckBox);
	options.AddWidget(&_onlyFirstMatchCheckBox);
	options.AddWidget(&_removeTimeOutsCheckBox);
	options.AddWidget(&_staticZScaleSlider);
	options.AddWidget(&_dynamicZScaleSlider);
	options.AddWidget(&_globalScaleSlider);
	options.AddWidget(&_reloadDemoButton);

	WidgetGroup& heatMaps = _tabWidgets[Tab::HeatMaps];
	heatMaps.AddWidget(&_heatMapGroup);
	heatMaps.AddWidget(&_heatMapOpacitySlider);
	heatMaps.AddWidget(&_genHeatMapsButton);
	heatMaps.AddWidget(&_heatMapSquaredRampCheckBox);
	heatMaps.AddWidget(&_drawHeatMapOnlyCheckBox);
	heatMaps.AddWidget(&_drawHeatMapCheckBox);

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

	for(int i = 2; i < argc; ++i)
	{
		if(udtString::Equals(udtString::NewConstRef(argv[i]), "/ProfileMode"))
		{
			_profileMode = true;
			break;
		}
	}

	Log::LogInfo("UDT 2D Viewer is now operational");
	
	_globalTimer.Start();
	_appLoaded = true;

	return true;
}

bool Viewer::LoadMapAliases()
{
	udtFileStream file;
	if(!file.Open(DATA_PATH_ALIASES, udtFileOpenMode::Read))
	{
		Platform_PrintError("Failed to open map aliases file %s", DATA_PATH_ALIASES);
		return false;
	}

	// Lines can end with "\r\n" or just "\n".
	// The last entry might not be followed by a line return.
	const udtString fileData = file.ReadAllAsString(_persistAllocator);
	char* c = fileData.GetWritePtr();
	char* found = c;
	char* toUse = c;
	for(;;)
	{
		if(*c == '\0')
		{
			if(toUse > found)
			{
				MapAlias alias;
				alias.NameFound = udtString::NewConstRef(found);
				alias.NameToUse = udtString::NewConstRef(toUse);
				_mapAliases.Add(alias);
			}
			break;
		}

		if(*c == ' ' || *c == '\t')
		{
			*c = '\0';
			toUse = c + 1;
		}
		else if(*c == '\r')
		{
			*c = '\0';
		}
		else if(*c == '\n')
		{
			*c = '\0';
			if(toUse > found)
			{
				MapAlias alias;
				alias.NameFound = udtString::NewConstRef(found);
				alias.NameToUse = udtString::NewConstRef(toUse);
				_mapAliases.Add(alias);
			}
			found = c + 1;
		}

		++c;
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

void Viewer::LoadDemo(const char* filePath)
{
	_demo.Load(filePath, _config.OnlyKeepFirstMatchSnapshots, _config.RemoveTimeOutSnapshots);
	
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
		for(u32 i = 0; i < 2; ++i)
		{
			_mapMin[i] -= 256.0f;
			_mapMax[i] += 256.0f;
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

	for(u32 p = 0; p < 64; ++p)
	{
		HeatMapData& heatMap = _heatMaps[p];
		if(heatMap.TextureId != InvalidTextureId)
		{
			nvgDeleteImage(_sharedReadOnly->NVGContext, heatMap.TextureId);
			heatMap.TextureId = InvalidTextureId;
		}
		heatMap.Image = nullptr;
		heatMap.Width = 0;
		heatMap.Height = 0;
		_heatMapBtnIdxToPlayerIdx[p] = 0;
	}

	free(_heatMapImages);
	_heatMapImages = nullptr;

	_threadedJobText = "loading demo...";
	Platform_NewThread(&DemoLoadThreadEntryPoint, data);
}

void Viewer::GenerateHeatMaps()
{
	_threadedJobProgress = 0.0f;
	_genericTimer.Restart();

	const HeatMapPlayer* players;
	u32 playerCount = 0;
	_demo.GetHeatMapPlayers(players);
	for(u32 i = 0; i < 64; ++i)
	{
		if(players[i].Present)
		{
			++playerCount;
		}
	}

	if(playerCount == 0)
	{
		return;
	}

	u32 buttonIndex = 0;
	for(u32 i = 0; i < 64; ++i)
	{
		if(players[i].Present)
		{
			_heatMapBtnIdxToPlayerIdx[buttonIndex++] = (u8)i;
		}
	}

	const u32 rampColorCount = 256;
	const u32 baseColorCount = 5;
	const u8 colors[baseColorCount][3] = { { 0, 0, 0 }, { 0, 255, 255 }, { 0, 255, 0 }, { 255, 255, 0 }, { 255, 0, 0 } };
	u8 colorRamp[rampColorCount][3];
	const u32 div = rampColorCount / (baseColorCount - 1);
	for(u32 i = 0; i < rampColorCount; ++i)
	{
		const u32 c0 = i / div;
		const u32 c1 = c0 + 1;
		const f32 t = (f32)(i % div) / (f32)div;
		for(u32 c = 0; c < 3; ++c)
		{
			colorRamp[i][c] = (u8)((f32)colors[c0][c] * (1.0f - t) + (f32)colors[c1][c] * t);
		}
	}

	const u32 w = _mapWidth > 0 ? _mapWidth : 1024;
	const u32 h = _mapHeight > 0 ? _mapHeight : 1024;
	const u32 pixelCount = w * h;
	const u32 byteCountHistogram = (u32)sizeof(u32) * pixelCount;
	const u32 byteCountHeatMap = 4 * pixelCount;
	const u32 byteCountPersistent = playerCount * byteCountHeatMap;
	const u32 byteCountTemp = byteCountHistogram + byteCountHeatMap;

	u8* const heatMaps = (u8*)malloc((size_t)byteCountPersistent);
	if(heatMaps == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for heat map generation", (int)byteCountPersistent);
	}
	_heatMapImages = heatMaps;

	u8* const temp = (u8*)malloc((size_t)byteCountTemp);
	if(temp == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for heat map generation", (int)byteCountTemp);
	}

	u32* const histogramTemp = (u32*)temp;
	u8* const heatMapTemp = temp + byteCountHistogram;

	u32 playerIndex = 0;
	for(u32 p = 0; p < 64; ++p)
	{
		if(players[p].Present == 0)
		{
			_heatMaps[p].Width = 0;
			_heatMaps[p].Height = 0;
			_heatMaps[p].Image = nullptr;
			_heatMaps[p].TextureId = InvalidTextureId;
			continue;
		}

		_demo.GenerateHeatMap(histogramTemp, w, h, _mapMin, _mapMax, p);

		u32 maxValue = 0;
		for(u32 i = 0; i < pixelCount; ++i)
		{
			maxValue = udt_max(maxValue, histogramTemp[i]);
		}

		u8* const heatMapFinal = heatMaps + playerIndex * byteCountHeatMap;
		const u8 gaussian3x3[9] = { 16, 32, 16, 32, 64, 32, 16, 32, 16 };
		const u32 divider = (maxValue + rampColorCount - 1) / rampColorCount;
		if(divider == 0)
		{
			memset(heatMapFinal, 0, (size_t)pixelCount * 4);
			goto finalize_image;
		}

		if(_config.HeatMapSquaredRamp)
		{
			for(u32 i = 0; i < pixelCount; ++i)
			{
				const u32 col2 = rampColorCount - 1 - (histogramTemp[i] / divider);
				const u32 col = rampColorCount - 1 - ((col2 * col2) / rampColorCount);
				const u8 op = (u8)((col * 256) / rampColorCount);
				heatMapTemp[4 * i + 0] = colorRamp[col][0];
				heatMapTemp[4 * i + 1] = colorRamp[col][1];
				heatMapTemp[4 * i + 2] = colorRamp[col][2];
				heatMapTemp[4 * i + 3] = op;
			}
		}
		else
		{
			for(u32 i = 0; i < pixelCount; ++i)
			{
				const u32 col = histogramTemp[i] / divider;
				const u8 op = (u8)((col * 256) / rampColorCount);
				heatMapTemp[4 * i + 0] = colorRamp[col][0];
				heatMapTemp[4 * i + 1] = colorRamp[col][1];
				heatMapTemp[4 * i + 2] = colorRamp[col][2];
				heatMapTemp[4 * i + 3] = op;
			}
		}

		// We smooth the heat map to make it look less blocky.
		ConvolveRGBAImage(heatMapFinal, heatMapTemp, w, h, gaussian3x3, 3);
		ConvolveRGBAImage(heatMapTemp, heatMapFinal, w, h, gaussian3x3, 3);
		ConvolveRGBAImage(heatMapFinal, heatMapTemp, w, h, gaussian3x3, 3);

	finalize_image:
		_heatMaps[p].Width = w;
		_heatMaps[p].Height = h;
		_heatMaps[p].Image = heatMapFinal;
		_heatMaps[p].TextureId = InvalidTextureId;

		++playerIndex;
		_threadedJobProgress = (f32)playerIndex / (f32)playerCount;
	}

	free(temp);

	_threadedJobProgress = 1.0f;

	udtVMLinearAllocator& tempAlloc = udtThreadLocalAllocators::GetTempAllocator();
	udtVMScopedStackAllocator allocScope(tempAlloc);
	const udtString genTime = FormatTime(tempAlloc, _genericTimer.GetElapsedMs());
	Log::LogInfo("Heat maps generated in %s", genTime.GetPtr());
}

void Viewer::StartGeneratingHeatMaps()
{
	HeatMapsGenThreadData* const data = (HeatMapsGenThreadData*)malloc(sizeof(HeatMapsGenThreadData));
	if(data == nullptr)
	{
		Platform_FatalError("Failed to allocate %d bytes for thread job data", (int)sizeof(HeatMapsGenThreadData));
	}
	data->ViewerPtr = this;
	_threadedJobText = "generating heat maps...";
	Platform_NewThread(&HeatMapsGenThreadEntryPoint, data);
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

		if(event.Scroll != 0 &&
		   _tabButtonGroup.GetSelectedIndex() == Tab::Log)
		{
			const f32 x = (f32)event.CursorPos[0];
			const f32 y = (f32)event.CursorPos[1];
			if(x >= _uiRect.Left() && 
			   x <= _uiRect.Right() &&
			   y >= _uiRect.Top() &&
			   y <= _uiRect.Bottom())
			{
				Log::Lock();
				Log::ShiftOffset(event.Scroll > 0 ? 2 : -2);
				Log::Unlock();
			}
		}
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
	else if(_genHeatMapsButton.WasClicked())
	{
		CriticalSectionLock lock(_appStateLock);
		_appState = AppState::GeneratingHeatMaps;
		StartGeneratingHeatMaps();
	}
	else if(_reloadDemoButton.WasClicked())
	{
		CriticalSectionLock lock(_appStateLock);
		_appState = AppState::LoadingDemo;
		StartLoadingDemo(_demo.GetFilePath());
	}

	if(_tabButtonGroup.HasSelectionChanged())
	{
		_activeTabWidgets = &_tabWidgets[_tabButtonGroup.GetSelectedIndex()];
	}
}

void Viewer::RenderNormal(const RenderParams& renderParams)
{
	if(!_demo.IsValid())
	{
		RenderNoDemo(renderParams);
		return;
	}

	u32 snapshotIndex;
	s32 displayTimeMs;
	s32 serverTimeMs;
	GetIndexAndTimes(snapshotIndex, displayTimeMs, serverTimeMs);
	if(!_demo.GetSnapshotData(*_snapshot, snapshotIndex))
	{
		RenderNoDemo(renderParams);
		return;
	}
	_snapshotIndex = snapshotIndex;
	_clockTimeMs = _config.TimerShowsServerTime ? serverTimeMs : (displayTimeMs - _demo.GetFirstSnapshotTimeMs());

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
	const u32 tabIndex = _tabButtonGroup.GetSelectedIndex();
	if(tabIndex == Tab::Options)
	{
		const f32 oo = (f32)BND_WIDGET_HEIGHT + 4.0f;
		const f32 ox = _uiRect.X();
		f32 oy = _uiRect.Y();
		_onlyFirstMatchCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_removeTimeOutsCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_drawMapOverlaysCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_drawMapScoresCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_drawMapClockCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_drawMapFollowMsgCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_drawMapHealthCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_showServerTimeCheckBox.SetRect(ctx, ox, oy); oy += oo;
		_globalScaleSlider.SetRect(ox, oy, 200.0f, (f32)BND_WIDGET_HEIGHT); oy += oo;
		_staticZScaleSlider.SetRect(ox, oy, 200.0f, (f32)BND_WIDGET_HEIGHT); oy += oo;
		_dynamicZScaleSlider.SetRect(ox, oy, 200.0f, (f32)BND_WIDGET_HEIGHT); oy += oo;
		_reloadDemoButton.SetRect(ctx, ox, oy);
	}
	else if(tabIndex == Tab::HeatMaps)
	{
		const f32 hmx = _uiRect.X();
		f32 hmy = _uiRect.Y();

		_heatMapSquaredRampCheckBox.SetRect(ctx, hmx, hmy);
		hmy += (f32)BND_WIDGET_HEIGHT;

		_genHeatMapsButton.SetRect(ctx, hmx, hmy);
		hmy += 2.0f * (f32)BND_WIDGET_HEIGHT;

		_drawHeatMapCheckBox.SetRect(ctx, hmx, hmy);
		hmy += (f32)BND_WIDGET_HEIGHT;

		_drawHeatMapOnlyCheckBox.SetRect(ctx, hmx, hmy);
		hmy += (f32)BND_WIDGET_HEIGHT + 8.0f;

		_heatMapOpacitySlider.SetRect(hmx, hmy, 100.0f, (f32)BND_WIDGET_HEIGHT);
		hmy += (f32)BND_WIDGET_HEIGHT + 8.0f;

		const HeatMapPlayer* players;
		_demo.GetHeatMapPlayers(players);

		float bounds[4];
		nvgFontSize(ctx, 13.0f);
		f32 maxNameWidth = 60.0f; // Minimum size to look good.
		for(u32 i = 0; i < 64; ++i)
		{
			if(players[i].Present)
			{
				nvgTextBounds(ctx, 0.0f, 0.0f, _demo.GetStringSafe(players[i].Name, "?"), nullptr, bounds);
				const f32 width = bounds[2] - bounds[0];
				maxNameWidth = udt_max(maxNameWidth, width);
			}
		}

		for(u32 i = 0; i < 64; ++i)
		{
			if(players[i].Present)
			{
				_heatMapPlayers[i].SetRect(hmx, hmy, maxNameWidth + 16.0f, (f32)BND_WIDGET_HEIGHT);
				hmy += (f32)BND_WIDGET_HEIGHT - 2.0f;
			}
		};
	}

	_activeWidgets.Draw(renderParams.NVGContext);
	_activeTabWidgets->Draw(renderParams.NVGContext);

	if(tabIndex == Tab::Chat)
	{
		DrawChat(renderParams, displayTimeMs);
	}
	else if(tabIndex == Tab::Log)
	{
		DrawLog(renderParams);
	}

	if(_demoProgressBar.IsHovered())
	{
		DrawProgressSliderToolTip(renderParams);
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
	const f32 mapScaleX = mapWidth / (_mapMax[0] - _mapMin[0]);
	const f32 mapScaleY = mapHeight / (_mapMax[1] - _mapMin[1]);
	const f32 mapScale = udt_min(mapScaleX, mapScaleY) * bgImageScale;
	f32 mapDisplayX = _mapRect.Left();
	f32 mapDisplayY = _mapRect.Top();
	
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
	
	const u32 heatMapButtonIndex = _heatMapGroup.GetSelectedIndex();
	const u32 heatMapIndex = _heatMapBtnIdxToPlayerIdx[heatMapButtonIndex];
	if(_config.DrawHeatMap && 
	   _config.HeatMapOpacity > 0.0f &&
	   heatMapIndex < 64 &&
	   _heatMaps[heatMapIndex].TextureId != InvalidTextureId)
	{
		NVGcontext* const c = renderParams.NVGContext;
		f32 w = mapWidth * bgImageScale;
		f32 h = mapHeight * bgImageScale;
		if(_map == InvalidTextureId)
		{
			const f32 tw = (f32)_heatMaps[heatMapIndex].Width;
			const f32 th = (f32)_heatMaps[heatMapIndex].Height;
			const f32 rw = (f32)_mapRect.Width();
			const f32 rh = (f32)_mapRect.Height();
			const f32 r = udt_min(tw / rw, th / rh);
			const f32 cw = _mapMax[0] - _mapMin[0];
			const f32 ch = _mapMax[1] - _mapMin[1];
			const f32 ar = cw / ch;
			w = tw / r;
			h = th / r;
			if(cw > ch)
			{
				mapDisplayY += (h - (h / ar));
				h /= ar;
			}
			else
			{
				w *= ar;
			}
		}
		const f32 x = mapDisplayX;
		const f32 y = mapDisplayY;
		nvgBeginPath(c);
		nvgRect(c, x, y, w, h);
		nvgFillPaint(c, nvgImagePattern(c, x, y, w, h, 0.0f, _heatMaps[heatMapIndex].TextureId, _config.HeatMapOpacity));
		nvgFill(c);
		nvgClosePath(c);

		if(_config.DrawHeatMapOnly)
		{
			RenderDemoOverlays(renderParams);
			return;
		}
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
			f32 size = 16.0f;
			if(item.Id == DynamicItemType::FlagRed ||
			   item.Id == DynamicItemType::FlagBlue)
			{
				size = 24.0f;
			}

			// @NOTE: DrawSpriteAt will check if the sprite ID is valid.
			const s32 spriteId = GetSpriteIdFromDynamicItemId(item.Id);
			DrawMapSpriteAt(params, (u32)spriteId, item.Position, size, _config.StaticZScale, item.Angle);
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
	
	// @TODO: sort players by Z position?
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
		const f32 radius = 6.0f * bgImageScale * pos[2];

		const bool firing = IsBitSet(&player.Flags, PlayerFlags::Firing);
		const s32 weaponSpriteId = GetSpriteIdFromWeaponId(player.WeaponId) + (firing ? 1 : 0);
		if(weaponSpriteId >= 0 && weaponSpriteId < Sprite::Count)
		{
			DrawPlayerWeapon(renderParams.NVGContext, pos[0], pos[1], radius, -player.Angle + UDT_PI / 2.0f, _sprites[weaponSpriteId]);
		}

		const u8 colorIndex = 2 * player.Team + (IsBitSet(&player.Flags, PlayerFlags::Followed) ? 1 : 0);
		DrawPlayer(renderParams.NVGContext, pos[0], pos[1], radius, -player.Angle, PlayerColors[colorIndex]);

		if(IsBitSet(&player.Flags, PlayerFlags::HasFlag))
		{
			const u32 flagSpriteId = player.Team == 1 ? (u32)Sprite::flag_blue : (u32)Sprite::flag_red;
			const f32 flagSize = 24.0f * pos[2];
			DrawSpriteAt(flagSpriteId, pos[0], pos[1] - flagSize / 1.5f, flagSize);
		}

		const char* const name = _demo.GetString(player.Name);
		if(name != nullptr)
		{
			DrawPlayerName(renderParams.NVGContext, pos[0], pos[1], radius, name);
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

	RenderDemoOverlays(renderParams);
}

void Viewer::RenderDemoOverlays(const RenderParams& renderParams)
{
	if(!_config.DrawMapOverlays)
	{
		return;
	}

	if(_config.DrawMapScores)
	{
		RenderDemoScore(renderParams);
	}
	
	if(_config.DrawMapClock)
	{
		RenderDemoTimer(renderParams);
	}
	
	if(_config.DrawMapFollowMsg || _config.DrawMapHealth)
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
	const int totalSec = (int)_clockTimeMs / 1000;
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
	
	if(_config.DrawMapFollowMsg &&
	   snapshot.Core.FollowedName != UDT_U32_MAX)
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

	if(!_config.DrawMapHealth)
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
	sprintf(armor, "%d", (int)snapshot.Core.FollowedArmor);
	sprintf(health, "%d", (int)snapshot.Core.FollowedHealth);
	sprintf(ammo, "%d", (int)snapshot.Core.FollowedAmmo);

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
	const char* const message0 = "Drag'n'drop a demo file anywhere in the window to load it.";
	const char* const message1 = "Press F1 to display the help (key binds, credits).";
	NVGcontext* const ctx = renderParams.NVGContext;
	nvgFontSize(ctx, 24.0f);
	f32 bounds0[4];
	f32 bounds1[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, message0, nullptr, bounds0);
	nvgTextBounds(ctx, 0.0f, 0.0f, message1, nullptr, bounds1);
	const f32 x0 = floorf(((f32)renderParams.ClientWidth - bounds0[2]) / 2.0f);
	const f32 x1 = floorf(((f32)renderParams.ClientWidth - bounds1[2]) / 2.0f);
	const f32 y = floorf((f32)renderParams.ClientHeight / 2.0f);

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGrey(255));
	nvgText(ctx, x0, y - 12.0f, message0, nullptr);
	nvgText(ctx, x1, y + 12.0f, message1, nullptr);
	nvgFill(ctx);
	nvgClosePath(ctx);
}

void Viewer::RenderThreadedJobProgress(const RenderParams& renderParams)
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
	nvgText(ctx, cx, cy - 10.0f, _threadedJobText, nullptr);
	nvgFillColor(ctx, nvgGrey(255));
	nvgFill(ctx);
	nvgClosePath(ctx);

	DrawProgressBar(ctx, x, y, w, h, r, _threadedJobProgress);

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

	switch(_appState)
	{
		case AppState::FinishDemoLoading:
			if(_demo.IsValid())
			{
				FinishLoadingDemo();
			}
			if(_demo.IsValid() && _profileMode)
			{
				_appState = AppState::GeneratingHeatMaps;
				StartGeneratingHeatMaps();
			}
			else
			{
				_appState = AppState::Normal;
				RenderThreadedJobProgress(renderParams);
			}
			break;

		case AppState::FinishHeatMapGeneration:
			if(_profileMode)
			{
				Platform_RequestQuit(_platform);
			}
			else
			{
				FinishGeneratingHeatMaps();
				_appState = AppState::Normal;
				RenderThreadedJobProgress(renderParams);
			}
			break;

		case AppState::LoadingDemo:
		case AppState::GeneratingHeatMaps:
			RenderThreadedJobProgress(renderParams);
			break;

		case AppState::Normal:
			RenderNormal(renderParams);
			break;

		default:
			break;
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
	int totalSec = 0;
	if(_config.TimerShowsServerTime)
	{
		const s32 displayTimeMs = (s32)GetTimeFromProgress(progress) + _demo.GetFirstSnapshotTimeMs();
		const u32 snapIndex = _demo.GetSnapshotIndexFromDisplayTime(displayTimeMs);
		const s32 serverTimeMs = _demo.GetSnapshotServerTimeMs(snapIndex);
		totalSec = (int)serverTimeMs / 1000;
	}
	else
	{
		totalSec = (int)GetTimeFromProgress(progress) / 1000;
	}
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

static void DrawHelpBox(f32& width, NVGcontext* ctx, f32 x, f32 y, const char** strings, u32 stringCount, u32 columnCount)
{
	f32 widths[16];
	assert(columnCount <= (u32)UDT_COUNT_OF(widths));

	nvgFontSize(ctx, 13.0f);
	width = 0.0f;
	for(u32 c = 0; c < columnCount; ++c)
	{
		widths[c] = 10.0f;
		for(u32 s = c; s < stringCount; s += columnCount)
		{
			float bounds[4];
			nvgTextBounds(ctx, 0.0f, 0.0f, strings[s], nullptr, bounds);
			widths[c] = udt_max(widths[c], bounds[2] - bounds[0] + 10.0f);
		}
		width += widths[c];
	}

	width -= (f32)(columnCount - 1);

	for(u32 i = 0; i < stringCount; i += columnCount)
	{
		int flags = BND_CORNER_DOWN | BND_CORNER_TOP;
		if(i == 0)
		{
			flags = BND_CORNER_DOWN;
		}
		else if(i == stringCount - columnCount)
		{
			flags = BND_CORNER_TOP;
		}
		const BNDwidgetState state = i == 0 ? BND_ACTIVE : BND_DEFAULT;
		if(columnCount == 2)
		{
			bndTextField(ctx, x, y, widths[0], BND_WIDGET_HEIGHT, BND_CORNER_RIGHT | flags, state, -1, strings[i], 1, 0);
			bndTextField(ctx, x + widths[0] - 1.0f, y, widths[1], BND_WIDGET_HEIGHT, BND_CORNER_LEFT | flags, state, -1, strings[i + 1], 1, 0);
		}
		else if(columnCount == 3)
		{
			bndTextField(ctx, x, y, widths[0], BND_WIDGET_HEIGHT, BND_CORNER_RIGHT | flags, state, -1, strings[i], 1, 0);
			bndTextField(ctx, x + widths[0] - 1.0f, y, widths[1], BND_WIDGET_HEIGHT, BND_CORNER_RIGHT | BND_CORNER_LEFT | flags, state, -1, strings[i + 1], 1, 0);
			bndTextField(ctx, x + widths[0] + widths[1] - 2.0f, y, widths[2], BND_WIDGET_HEIGHT, BND_CORNER_LEFT | flags, state, -1, strings[i + 2], 1, 0);
		}
		
		y += BND_WIDGET_HEIGHT - 2.0f;
	}
}

void Viewer::DrawHelp(const RenderParams& renderParams)
{
	NVGcontext* const ctx = renderParams.NVGContext;

	nvgBeginPath(ctx);
	nvgFillColor(ctx, nvgGreyA(0, 160));
	nvgRect(ctx, 0.0f, 0.0f, (f32)renderParams.ClientWidth, (f32)renderParams.ClientHeight);
	nvgFill(ctx);
	nvgClosePath(ctx);

	f32 w;
	DrawHelpBox(w, ctx, 10.0f, 10.0f, HelpBindStrings, (u32)UDT_COUNT_OF(HelpBindStrings), 2);
	DrawHelpBox(w, ctx, w + 20.0f, 10.0f, HelpCreditStrings, (u32)UDT_COUNT_OF(HelpCreditStrings), 3);
}

void Viewer::DrawChat(const RenderParams& renderParams, s32 displayTimeMs)
{
	const u32 index = _demo.GetChatMessageIndexFromDisplayTime(displayTimeMs);
	if(index == UDT_U32_MAX)
	{
		return;
	}

	NVGcontext* const ctx = renderParams.NVGContext;
	const f32 fontSize = 16.0f;
	nvgFontSize(ctx, fontSize);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);

	const f32 top = _uiRect.Top() + fontSize;
	const f32 x = _uiRect.Left();
	const f32 w = _uiRect.Width();
	const f32 lineW = udt_max(w, 100.0f);
	f32 y = _uiRect.Bottom();

	u32 i = index;
	ChatMessage message;
	while(_demo.GetChatMessage(message, i--))
	{
		if(y <= top)
		{
			break;
		}

		const int offsetMs = _config.TimerShowsServerTime ? 0 : (int)_demo.GetFirstSnapshotTimeMs();
		const int totalSec = ((int)message.DisplayTimeMs - offsetMs) / 1000;
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

		float bounds[4];
		nvgTextBoxBounds(ctx, 0.0f, 0.0f, lineW, line, nullptr, bounds);
		y -= bounds[3] - bounds[1];

		nvgBeginPath(ctx);
		nvgFillColor(ctx, nvgGrey(0));
		nvgTextBox(ctx, x, y, lineW, line, nullptr);
		nvgFill(ctx);
		nvgClosePath(ctx);
	}
}

void Viewer::DrawLog(const RenderParams& renderParams)
{
	NVGcontext* const ctx = renderParams.NVGContext;
	const f32 fontSize = 16.0f;
	nvgFontSize(ctx, fontSize);
	nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);

	const f32 top = _uiRect.Top() + fontSize;
	const f32 x = _uiRect.Left();
	const f32 w = udt_max(_uiRect.Width(), 100.0f);
	f32 y = _uiRect.Bottom();

	Log::Lock();

	const u32 msgCount = Log::GetMessageCount();
	const u32 offset = Log::GetOffset();
	if(offset > 0)
	{
		y -= fontSize;
	}

	for(s32 i = (s32)(msgCount - 1 - offset); i >= 0; i--)
	{
		if(y <= top)
		{
			break;
		}

		const char* const msg = Log::GetMessageString((u32)i);
		const u32 level = Log::GetMessageLevel((u32)i);
		const NVGcolor color = LogColors[level > (u32)Log::Level::Count ? 0 : level];

		float bounds[4];
		nvgTextBoxBounds(ctx, 0.0f, 0.0f, w, msg, nullptr, bounds);
		y -= bounds[3] - bounds[1];

		nvgBeginPath(ctx);
		nvgFillColor(ctx, color);
		nvgTextBox(ctx, x, y, w, msg, nullptr);
		nvgFill(ctx);
		nvgClosePath(ctx);
	}

	Log::Unlock();

	if(offset > 0)
	{
		nvgBeginPath(ctx);
		nvgFillColor(ctx, nvgGrey(0));
		nvgText(ctx, x, _uiRect.Bottom() - fontSize, LogSeparator, nullptr);
		nvgFill(ctx);
		nvgClosePath(ctx);
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

void Viewer::DrawSpriteAt(u32 spriteId, f32 x, f32 y, f32 size, f32 a)
{
	if(spriteId >= (u32)Sprite::Count)
	{
		return;
	}

	const f32 w = size;
	const f32 h = size;
	NVGcontext* const c = _sharedReadOnly->NVGContext;

	nvgBeginPath(c);
	nvgResetTransform(c);
	nvgTranslate(c, x, y);
	nvgScale(c, w, h);
	nvgRotate(c, a);
	nvgRect(c, -0.5f, -0.5f, 1.0f, 1.0f);
	nvgFillPaint(c, nvgImagePattern(c, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, _sprites[spriteId], 1.0f));
	nvgFill(c);
	nvgClosePath(c);
	nvgResetTransform(c);
}

void Viewer::GetIndexAndTimes(u32& snapshotIndex, s32& displayTimeMs, s32& serverTimeMs)
{
	const f32 progress = GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
	displayTimeMs = _demo.GetFirstSnapshotTimeMs() + (s32)(progress * (f32)_demo.GetDurationMs());
	snapshotIndex = _demo.GetSnapshotIndexFromDisplayTime(displayTimeMs);
	serverTimeMs = _demo.GetSnapshotServerTimeMs(snapshotIndex);
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
	const s32 newDisplayTimeMs1 = _demo.GetSnapshotDisplayTimeMs(newSnapIndex1);
	const s32 newDisplayTimeMs2 = _demo.GetSnapshotDisplayTimeMs(newSnapIndex2);
	const s32 newDisplayTimeMs = (newDisplayTimeMs1 + newDisplayTimeMs2) / 2;
	const f32 newProgress = (f32)(newDisplayTimeMs - _demo.GetFirstSnapshotTimeMs()) / (f32)_demo.GetDurationMs();
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
	result[2] = _config.GlobalScale * (1.0f + zScale * ((input[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2])));
}

void Viewer::FinishLoadingDemo()
{
	const HeatMapPlayer* players;
	_demo.GetHeatMapPlayers(players);
	_heatMapGroup.RemoveAllRadioButtons();

	u32 first = 64;
	u32 last = 0;
	for(u32 i = 0; i < 64; ++i)
	{
		if(players[i].Present)
		{
			first = udt_min(first, i);
			last = udt_max(last, i);
		}
	}

	for(u32 i = 0; i < 64; ++i)
	{
		if(!players[i].Present)
		{
			continue;
		}

		int cornerFlags = 0;
		if(i > first)
		{
			cornerFlags |= BND_CORNER_TOP;
		}
		if(i < last)
		{
			cornerFlags |= BND_CORNER_DOWN;
		}
		_heatMapPlayers[i].SetActive(i == first);
		_heatMapPlayers[i].SetCornerFlags(cornerFlags);
		_heatMapPlayers[i].SetText(_demo.GetStringSafe(players[i].Name, "?"));
		_heatMapGroup.AddRadioButton(&_heatMapPlayers[i]);
	}

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

void Viewer::FinishGeneratingHeatMaps()
{
	for(u32 p = 0; p < 64; ++p)
	{
		HeatMapData& heatMap = _heatMaps[p];
		if(heatMap.Image != nullptr)
		{
			CreateTextureRGBA(heatMap.TextureId, heatMap.Width, heatMap.Height, (const u8*)heatMap.Image);
			heatMap.Image = nullptr;
		}
	}

	free(_heatMapImages);
	_heatMapImages = nullptr;
}

void Viewer::ShutDown()
{
	SaveConfig(_config, CONFIG_PATH);
}
