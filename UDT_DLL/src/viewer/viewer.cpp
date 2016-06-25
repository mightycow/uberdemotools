#include "viewer.hpp"
#include "platform.hpp"
#include "file_stream.hpp"
#include "math.hpp"
#include "utils.hpp"
#include "scoped_stack_allocator.hpp"
#include "path.hpp"
#include "nanovg_drawing.hpp"
#include "nanovg/fontstash.h"
#include "nanovg/stb_image.h"

#include <stdlib.h>
#include <math.h>


#define DATA_PATH "viewer_data"


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


void Viewer::DemoProgressCallback(f32 progress, void* userData)
{
	Viewer* const viewer = (Viewer*)userData;
	viewer->_drawDemoLoadProgress = true;
	viewer->_demoLoadProgress = progress;
	Platform_Draw(viewer->_platform);
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
	free(_snapshot);
}

bool Viewer::Init(int argc, char** argv)
{
	Snapshot* const snapshot = (Snapshot*)malloc(sizeof(Snapshot));
	if(snapshot == nullptr ||
	   !_demo.Init(&DemoProgressCallback, this) ||
	   !LoadMapAliases() ||
	   !LoadSprites() ||
	   nvgCreateFont(_sharedReadOnly->NVGContext, "sans", DATA_PATH"/Roboto-Regular.ttf") == FONS_INVALID)
	{
		return false;
	}
	_snapshot = snapshot;

	if(argc >= 2 && udtFileStream::Exists(argv[1]))
	{
		LoadDemo(argv[1]);
	}

	_activeWidgets.AddWidget(&_demoProgressBar);
	_activeWidgets.AddWidget(&_playbackButtonBar);
	_playbackButtonBar.AddButton(&_playPauseButton);
	_playbackButtonBar.AddButton(&_stopButton);
	_playbackButtonBar.AddButton(&_reversePlaybackButton);
	
	_demoPlaybackTimer.Start();

	return true;
}

bool Viewer::LoadMapAliases()
{
	// Line endings: \r\n
	udtVMScopedStackAllocator allocScope(_tempAllocator);

	udtString filePath;
	udtPath::Combine(filePath, _tempAllocator, udtString::NewConstRef(DATA_PATH), "mapaliases.txt");
	udtFileStream file;
	if(!file.Open(filePath.GetPtr(), udtFileOpenMode::Read))
	{
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
	udtVMScopedStackAllocator allocScope(_tempAllocator);

	udtString filePath;
	udtPath::Combine(filePath, _tempAllocator, udtString::NewConstRef(DATA_PATH), "sprites.texturepack");
	udtFileStream file;
	if(!file.Open(filePath.GetPtr(), udtFileOpenMode::Read))
	{
		return false;
	}

	u32 version = 0;
	u32 spriteCount = 0;
	file.Read(&version, 4, 1);
	file.Read(&spriteCount, 4, 1);
	if(spriteCount != Sprite::Count)
	{
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
			return false;
		}
	}

	return true;
}

void Viewer::LoadMap(const udtString& mapName)
{
	if(_map != InvalidTextureId)
	{
		nvgDeleteImage(_sharedReadOnly->NVGContext, _map);
	}
	_map = InvalidTextureId;
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

	int mapTextureId = 0;
	const bool validTexture = CreateTextureRGBA(mapTextureId, (u32)w, (u32)h, pixels);
	stbi_image_free(pixels);
	if(!validTexture)
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
	_map = mapTextureId;
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

void Viewer::ProcessEvent(const Event& event)
{
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
	}
	else if(event.Type == EventType::MouseButtonUp)
	{
		_activeWidgets.MouseButtonUp(event.CursorPos[0], event.CursorPos[1], event.MouseButtonId);
	}
	else if(event.Type == EventType::MouseMove)
	{
		_activeWidgets.MouseMove(event.CursorPos[0], event.CursorPos[1]);
	}
	else if(event.Type == EventType::MouseMoveNC)
	{
		_activeWidgets.MouseMoveNC(event.CursorPos[0], event.CursorPos[1]);
	}
	else if(event.Type == EventType::MouseScroll)
	{
		_activeWidgets.MouseScroll(event.CursorPos[0], event.CursorPos[1], event.Scroll);
	}
	else if(event.Type == EventType::FilesDropped)
	{
		for(u32 i = 0; i < event.DroppedFileCount; ++i)
		{
			if(udtIsValidProtocol(udtGetProtocolByFilePath(event.DroppedFilePaths[i])))
			{
				LoadDemo(event.DroppedFilePaths[i]);
				break;
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
	Button* button = nullptr;
	if(_demoProgressBar.HasProgressChanged(demoProgress) && _demo.IsValid())
	{
		SetPlaybackProgress(demoProgress);
	}
	else if(_playbackButtonBar.WasClicked(button))
	{
		if(button == &_playPauseButton)
		{
			TogglePlayback();
		}
		else if(button == &_reversePlaybackButton)
		{
			ReversePlayback();
		}
		else if(button == &_stopButton)
		{
			StopPlayback();
		}
	}
}

void Viewer::Update()
{
	if(_demo.GetSnapshotCount() > 0 && GetCurrentSnapshotIndex() != _snapshotIndex)
	{
		Platform_RequestDraw(_platform);
	}
}

void Viewer::RenderDemo(RenderParams& renderParams)
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

	RenderDemoScore(renderParams);
	RenderDemoTimer(renderParams);
	RenderDemoFollowedPlayer(renderParams);
}

void Viewer::RenderDemoScore(RenderParams& renderParams)
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

void Viewer::RenderDemoTimer(RenderParams& renderParams)
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

void Viewer::RenderDemoFollowedPlayer(RenderParams& renderParams)
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

	float bounds[4];
	nvgTextBounds(ctx, 0.0f, 0.0f, "000", nullptr, bounds);
	const f32 textw = ceilf(bounds[2] - bounds[0]);
	const f32 iconw = fontSize;

	// Top left corner.
	const f32 delta = 4.0f;
	const f32 w = 3.0f * (iconw + textw) + 5.0f * delta;
	const f32 x = floorf(_mapRect.CenterX() - w / 2.0f);
	const f32 y = _mapRect.Bottom() - fontSize - space;

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

void Viewer::RenderNoDemo(RenderParams& renderParams)
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

void Viewer::RenderProgress(RenderParams& renderParams)
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
}

void Viewer::Render(RenderParams& renderParams)
{
	if(_drawDemoLoadProgress)
	{
		RenderProgress(renderParams);
		_drawDemoLoadProgress = false;
		return;
	}

	if(_demo.GetSnapshotCount() == 0)
	{
		RenderNoDemo(renderParams);
		return;
	}

	const u32 snapshotIndex = GetCurrentSnapshotIndex();
	if(!_demo.GetSnapshotData(*_snapshot, snapshotIndex))
	{
		RenderNoDemo(renderParams);
		return;
	}
	_snapshotIndex = snapshotIndex;

	const f32 progressBarMargin = 2.0f;
	const f32 progressBarHeight = ceilf(16.0f * _config.UIScaleY);
	const f32 progressY = (f32)renderParams.ClientHeight - progressBarHeight - 2.0f * progressBarMargin;
	_progressRect.Set(progressBarMargin, progressY, (f32)renderParams.ClientWidth - 2.0f * progressBarMargin, progressBarHeight);
	const f32 mapAspectRatio = (_mapWidth > 0 && _mapHeight > 0) ? ((f32)_mapWidth / (f32)_mapHeight) : 1.0f;
	const f32 mapHeight = progressY - progressBarMargin;
	const f32 mapWidth = ceilf(mapHeight * mapAspectRatio);
	_mapRect.Set(0.0f, 0.0f, mapWidth, mapHeight);
	_uiRect.Set(mapWidth, 0.0f, (f32)renderParams.ClientWidth - mapWidth, mapHeight);

	RenderDemo(renderParams);

	const f32 r2 = floorf(progressBarHeight / 3.0f);
	_playbackButtonBar.DoLayout(_progressRect.X(), _progressRect.Y(), progressBarHeight, r2);
	f32 a, b, c, d;
	_playbackButtonBar.GetRect(a, b, c, d);

	const f32 x = a + c;
	const f32 r = floorf(progressBarHeight / 2.0f);
	const f32 y = _progressRect.Y();
	_demoProgressBar.SetRect(x + 2.0f * r, y, _progressRect.Width() - x - 4.0f * r, 2.0f * r);
	_demoProgressBar.SetRadius(r);
	_demoProgressBar.SetProgress(GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs()));

	_activeWidgets.Draw(renderParams.NVGContext);
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

u32 Viewer::GetCurrentSnapshotIndex()
{
	return GetSapshotIndexFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
}

u32 Viewer::GetSapshotIndexFromTime(u32 elapsedMs)
{
	const f32 progress = GetProgressFromTime(elapsedMs);
	const s32 serverTimeMs = _demo.GetFirstSnapshotTimeMs() + (s32)(progress * (f32)_demo.GetDurationMs());

	return _demo.GetSnapshotIndexFromServerTime(serverTimeMs);
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
	_playPauseButton.SetPlaying(false);
	Platform_RequestDraw(_platform);
}

void Viewer::ResumePlayback()
{
	_demoPlaybackTimer.Start();
	_playPauseButton.SetPlaying(true);
	Platform_RequestDraw(_platform);
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
	_playPauseButton.SetPlaying(false);
	_reversePlaybackButton.SetReversed(false);
	Platform_RequestDraw(_platform);
}

void Viewer::ReversePlayback()
{
	const f32 progress = GetProgressFromTime((u32)_demoPlaybackTimer.GetElapsedMs());
	_reversePlayback = !_reversePlayback;
	_reversePlaybackButton.SetReversed(_reversePlayback);
	SetPlaybackProgress(progress);
	Platform_RequestDraw(_platform);
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
	Platform_RequestDraw(_platform);
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
