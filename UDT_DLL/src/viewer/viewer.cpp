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


const f32 ViewerClearColor[4] = { 0.447f, 0.447f, 0.447f, 1.0f };


Viewer::Viewer(Platform& platform)
	: _platform(platform)
{
	udtInitLibrary();

	_tempAllocator.Init(1 << 16, "Viewer::Temp");
	_persistAllocator.Init(1 << 16, "Viewer::Persist");
	_mapAliases.Init(1 << 16, "Viewer::AliasArray");

	Platform_GetSharedDataPointers(platform, (const PlatformReadOnly**)&_sharedReadOnly, &_sharedReadWrite);
}

Viewer::~Viewer()
{
	udtShutDownLibrary();
	free(_snapshot);
}

bool Viewer::Init(int argc, char** argv)
{
	Snapshot* const snapshot = (Snapshot*)malloc(sizeof(Snapshot));
	if(snapshot == nullptr ||
	   !_demo.Init() ||
	   !LoadMapAliases() ||
	   !LoadSprites() ||
	   nvgCreateFont(_sharedReadOnly->NVGContext, "sans", DATA_PATH"\\Roboto-Regular.ttf") == FONS_INVALID)
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
	if(!CreateTextureRGBA(mapTextureId, (u32)w, (u32)h, pixels))
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
	if(GetCurrentSnapshotIndex() != _snapshotIndex)
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
			const f32 x = _mapRect.Left() + (item.Position[0] - _mapMin[0]) * mapScale;
			const f32 y = _mapRect.Bottom() - (item.Position[1] - _mapMin[1]) * mapScale;
			const f32 zScale = 1.0f + _config.DynamicZScale * ((item.Position[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
			DrawGrenade(renderParams.NVGContext, x, y, 4.0f * bgImageScale * zScale);
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
		const f32 x0 = _mapRect.Left() + (beam.StartPosition[0] - _mapMin[0]) * mapScale;
		const f32 y0 = _mapRect.Bottom() - (beam.StartPosition[1] - _mapMin[1]) * mapScale;
		const f32 z0 = 1.0f + 4.0f * ((beam.StartPosition[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
		const f32 x1 = _mapRect.Left() + (beam.EndPosition[0] - _mapMin[0]) * mapScale;
		const f32 y1 = _mapRect.Bottom() - (beam.EndPosition[1] - _mapMin[1]) * mapScale;
		const f32 z1 = 1.0f + 4.0f * ((beam.EndPosition[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
		DrawRailBeam(renderParams.NVGContext, x0, y0, z0, x1, y1, z1, beam.Alpha);
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

		const f32 x0 = _mapRect.Left() + (player.Position[0] - _mapMin[0]) * mapScale;
		const f32 y0 = _mapRect.Bottom() - (player.Position[1] - _mapMin[1]) * mapScale;
		const f32 z0 = 1.0f + _config.DynamicZScale * ((player.Position[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
		const f32 x1 = _mapRect.Left() + (player.LGEndPoint[0] - _mapMin[0]) * mapScale;
		const f32 y1 = _mapRect.Bottom() - (player.LGEndPoint[1] - _mapMin[1]) * mapScale;
		const f32 z1 = 1.0f + _config.DynamicZScale * ((player.LGEndPoint[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
		DrawShaftBeam(renderParams.NVGContext, x0, y0, z0, x1, y1, z1);
	}
	
	for(u32 p = 0; p < snapshot.PlayerCount; ++p)
	{
		const Player& player = snapshot.Players[p];
		if(IsBitSet(&player.Flags, PlayerFlags::Dead))
		{
			continue;
		}

		const f32 x = _mapRect.Left() + (player.Position[0] - _mapMin[0]) * mapScale;
		const f32 y = _mapRect.Bottom() - (player.Position[1] - _mapMin[1]) * mapScale;
		const f32 zScale = 1.0f + _config.DynamicZScale * ((player.Position[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
		DrawPlayer(renderParams.NVGContext, x, y, 6.0f * bgImageScale * zScale, -player.Angle, IsBitSet(&player.Flags, PlayerFlags::Firing));
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
			const f32 x = _mapRect.Left() + (item.Position[0] - _mapMin[0]) * mapScale;
			const f32 y = _mapRect.Bottom() - (item.Position[1] - _mapMin[1]) * mapScale;
			const f32 zScale = 1.0f + _config.DynamicZScale * ((item.Position[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
			DrawImpact(renderParams.NVGContext, x, y, 2.0f * bgImageScale * zScale);
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

	// @TODO: follow message
	{
		const char* const name = _demo.GetString(snapshot.Core.FollowedName);
		if(name != nullptr)
		{
			char msg[256];
			sprintf(msg, "Following %s - HP %d - armor %d", name, snapshot.Core.FollowedHealth, snapshot.Core.FollowedArmor);
			NVGcontext* const ctx = renderParams.NVGContext;
			nvgBeginPath(ctx);
			nvgFontSize(ctx, 16.0f);
			nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
			nvgText(ctx, _uiRect.X() + 5.0f, _uiRect.Y() + 5.0f, msg, nullptr);
			nvgFillColor(ctx, nvgGrey(255));
			nvgFill(ctx);
			nvgClosePath(ctx);
		}
	}

	// @TODO: score message
	{
		int score1 = (int)snapshot.Score.Score1;
		int score2 = (int)snapshot.Score.Score2;
		const char* name1 = "?";
		const char* name2 = "?";
		if(snapshot.Score.IsScoreTeamBased)
		{
			name1 = "RED";
			name2 = "BLUE";
		}
		else
		{
			name1 = _demo.GetStringSafe(snapshot.Score.Score1Name, "?");
			name2 = _demo.GetStringSafe(snapshot.Score.Score2Name, "?");

			// For display consistency, we always keep the lowest client number first.
			// @TODO: only in duel? for FFA/RR, show the leaders
			if(snapshot.Score.Score1Id > snapshot.Score.Score2Id)
			{
				const char* const nameTemp = name2;
				const int scoreTemp = score2;
				name2 = name1;
				score2 = score1;
				name1 = nameTemp;
				score1 = scoreTemp;
			}
		}

		char msg[256];
		sprintf(msg, "%s %d - %d %s", name1, score1, score2, name2);
		NVGcontext* const ctx = renderParams.NVGContext;
		nvgBeginPath(ctx);
		nvgFontSize(ctx, 16.0f);
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_LEFT | NVGalign::NVG_ALIGN_TOP);
		nvgText(ctx, _uiRect.X() + 5.0f, _uiRect.Y() + 5.0f + 16.0f + 4.0f, msg, nullptr);
		nvgFillColor(ctx, nvgGrey(255));
		nvgFill(ctx);
		nvgClosePath(ctx);
	}
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

void Viewer::Render(RenderParams& renderParams)
{
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

	// @TODO: timer, showing either match time or true server time
	{
		const int totalSec = (int)_demoPlaybackTimer.GetElapsedSec();
		//const int totalSec = ((int)_demoPlaybackTimer.GetElapsedMs() + (int)_demo.GetFirstSnapshotTimeMs()) / 1000;
		const int minutes = totalSec / 60;
		const int seconds = totalSec % 60;
		char clock[256];
		sprintf(clock, "%d:%02d", minutes, seconds);
		const f32 x = floorf(_mapRect.CenterX());
		const f32 y = ceilf(_mapRect.Top());
		NVGcontext* const ctx = renderParams.NVGContext;
		nvgBeginPath(ctx);
		nvgFontSize(ctx, 16.0f);
		nvgTextAlign(ctx, NVGalign::NVG_ALIGN_CENTER | NVGalign::NVG_ALIGN_TOP);
		nvgText(ctx, x, y, clock, nullptr);
		nvgFillColor(ctx, nvgGrey(0));
		nvgFill(ctx);
		nvgClosePath(ctx);
	}
}

void Viewer::DrawMapSpriteAt(const SpriteDrawParams& params, u32 spriteId, const f32* pos, f32 size, f32 scaleZ, f32 a)
{
	if(spriteId >= (u32)Sprite::Count)
	{
		return;
	}

	const f32 x = params.X + (pos[0] - _mapMin[0]) * params.CoordsScale;
	const f32 y = params.Y + _mapRect.Height() - (pos[1] - _mapMin[1]) * params.CoordsScale;
	const f32 zScale = 1.0f + scaleZ * ((pos[2] - _mapMin[2]) / (_mapMax[2] - _mapMin[2]));
	const f32 w = size * params.ImageScale * zScale;
	const f32 h = size * params.ImageScale * zScale;
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
