#include "paint_widget.h"

#include <QPainter>
#include <QFileInfo>
#include <QStaticText>
#include <QPainterPathStroker>
#include <QStringList>
#include <QRect>
#include <QString>
#include <qDebug>


static const int FollowingTextDeltaY = 35;
static const float HeightScaleRatio = 50.0f;
static const float PlayerRadiusBase = 15.0f;
static const int PlayerRadiusQuad = 19;
static const int PlayerRadiusBs = 21;

static const int ItemIdxFromPowerUpIdx[16] =
{
	-1, // Invalid.
	ITEM_QUAD,
	ITEM_ENVIRO,
	ITEM_HASTE,
	ITEM_INVIS,
	ITEM_REGEN,
	ITEM_FLIGHT,
	TEAM_CTF_REDFLAG,
	TEAM_CTF_BLUEFLAG,
	TEAM_CTF_NEUTRALFLAG,
	ITEM_SCOUT,
	ITEM_GUARD,
	ITEM_DOUBLER,
	ITEM_AMMOREGEN,
	-1, // PW_INVULNERABILITY
	-1, // Nothing, filler.
};

// @TODO:
static const int QlItemIndexFromQ3ItemIndex[] = 
{
	-1, // ok
	1, // ok
	2, // ok
	3, // ok
	5, // ok
	6, // ok
	7, // ok
	8, // ok
	9, // ??? gauntlet?
	10, // ok
	11, // ok
	12, // ok
	13, // ok
	14, // ok
	15, // ok
	16, // ok
	17, // ???
	18, // ???
	19, // ok
	20, // ok
	21, // ok
	22, // ok
	23, // ok
	24, // ok
	25, // ok
	-1, // ???
	-1, //26,
	-1, //27,
	-1, //28
	29, // ok, quad
	30, // ok, battle suit
	-1, //31,
	-1, //32,
	33, // ok, regeneration
	35, // ok, red flag
	36, // ok, blue flag
	4 // ok, green armor
};


static QImage* CreateProxyImage(int width, int height)
{
	QImage* const image = new QImage(width, height, QImage::Format_RGB888);

	for(int y = 0; y < height / 2; ++y)
	{
		for(int x = 0; x < width / 2; ++x)
		{
			image->setPixel(x, y, -1);
		}
		for(int x = width / 2; x < width; ++x)
		{
			image->setPixel(x, y, 0);
		}
	}

	for(int y = height / 2; y < height; ++y)
	{
		for(int x = 0; x < width / 2; ++x)
		{
			image->setPixel(x, y, 0);
		}
		for(int x = width / 2; x < width; ++x)
		{
			image->setPixel(x, y, -1);
		}
	}

	return image;
}


PaintWidget::PaintWidget(QWidget *parent)
	: QWidget(parent)
{
	DemoData = NULL;
	RenderScale = 1.0f;
	AdaptRenderScaleToWindowSize = true;
	ShowClock = true;
	ShowScore = true;
	ShowHud = true;
	ShowPowerUps = true;
	DisplayDemo = false;
	BackgroundMessage = "Drag and drop a demo here.";

	_bgImage = NULL;
	_proxyImage = NULL;
	_iconScale = 0.4f;
	_lastValidWeapon = 0;

	ResetScaling();
}

PaintWidget::~PaintWidget()
{
	if(_bgImage != NULL)
	{
		delete _bgImage;
	}

	for(size_t i = 0; i < _items.size(); ++i)
	{
		if(_items[i].Image != NULL)
		{
			delete _items[i].Image;
		}
	}

	for(size_t i = 0; i < _weapons.size(); ++i)
	{
		if(_weapons[i] != NULL)
		{
			delete _weapons[i];
		}
	}
}

void PaintWidget::ResetScaling()
{
	_mapOrigin[0] = _mapOrigin[1] = _mapOrigin[2] = 0;
	_mapEnd[0] = _mapEnd[1] = _mapEnd[2] = 100;

	QRect unscaledRect;
	GetUnscaledRect(unscaledRect);
	resize(unscaledRect.width(), unscaledRect.height());

	const float scaleX =  unscaledRect.width()  / (float)(_mapEnd[0] - _mapOrigin[0]);
	const float scaleY = -unscaledRect.height() / (float)(_mapEnd[1] - _mapOrigin[1]);
	_coordsScale = std::min(scaleX, scaleY);
	_heightScale = HeightScaleRatio / (float)(_mapEnd[2] - _mapOrigin[2]);
}

void PaintWidget::SetScaling(int* origin, int* end)
{
	_mapOrigin[0] = origin[0];
	_mapOrigin[1] = origin[1];
	_mapOrigin[2] = origin[2];

	_mapEnd[0] = end[0];
	_mapEnd[1] = end[1];
	_mapEnd[2] = end[2];

	QRect rect;
	GetUnscaledRect(rect);
	const int w = rect.width();
	const int h = rect.height();
	const float scaleX =  w / (float)(_mapEnd[0] - _mapOrigin[0]);
	const float scaleY = -h / (float)(_mapEnd[1] - _mapOrigin[1]);
	_coordsScale = std::min(scaleX, scaleY);
	_heightScale = HeightScaleRatio / (float)(end[2] - origin[2]);
}

void PaintWidget::paintEvent(QPaintEvent* event)
{
	QRect offlineRect;
	GetUnscaledRect(offlineRect);
	const int width = offlineRect.width();
	const int height = offlineRect.height();

	QPixmap offlineBuffer(width, height);
	QPainter offlinePainter(&offlineBuffer);
	offlinePainter.setRenderHints(QPainter::Antialiasing, true);
	offlinePainter.fillRect(offlineRect, Qt::gray);

	QFont font;
	font.setPixelSize(30);
	offlinePainter.setFont(font);

	const QFontMetrics fontMetrics(font);
	const int posX = fontMetrics.width(BackgroundMessage) / 2;
	offlinePainter.drawText(width / 2 - posX, height / 2, BackgroundMessage);

	if(DisplayDemo)
	{
		PaintDemo(offlinePainter);
	}

	QRect windowRect;
	GetScaledRect(windowRect);
	const int scaledWidth = windowRect.width();
	const int scaledHeight = windowRect.height();
	QPainter windowPainter(this);
	windowPainter.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform, true);
	windowPainter.drawPixmap(windowRect, offlineBuffer);
}

void PaintWidget::resizeEvent(QResizeEvent* event)
{
	if(!AdaptRenderScaleToWindowSize)
	{
		return;
	}

	ComputeRenderScale();

}

void PaintWidget::PaintDemo(QPainter& painter)
{
	painter.setBrushOrigin(geometry().x(), geometry().y());
	if(_bgImage != NULL)
	{
		painter.drawImage(0, 0, *_bgImage);
	}

	if(DemoData->DemoParser == NULL)
	{
		return;
	}

	Demo* const demo = DemoData->DemoParser;
	const std::vector<Entity>& entities = DemoData->Entities;
	for(size_t i = 128; i < entities.size(); ++i)
	{
		const Entity& entity = entities[i];
		if(entity.SyncCoolDown <= 0)
		{
			continue;
		}

		const float alpha = entity.SyncCoolDown / (float)(UDT_DV_SYNC_MAX);
		Demo::EntityInfo* entityInfo = &(demo->_entityPlaybackInfos[entity.Index]);

		switch(entityInfo->Type)
		{
		case Demo::EntityType::Item:
			DrawItem(painter, entityInfo, alpha);
			break;
		case Demo::EntityType::Projectile:
			DrawProjectile(painter, entityInfo);
			break;
		case Demo::EntityType::Generic:
			DrawGeneric(painter, entityInfo);
			break;
		}
	}

	const std::vector<Beam>& beams = DemoData->Beams;
	for(size_t i = 0; i < beams.size(); ++i)
	{
		const Beam& beam = beams[i];
		if(beam.TimeToLive <= 0)
		{
			continue;
		}

		const float timeRatio = beam.TypeId == Beam::Type::RG ? 500.0f : 50.0f;
		DrawBeams(painter, beam.StartPositions, beam.EndPositions, beam.TypeId, (float)beam.TimeToLive / timeRatio);
	}

	std::vector<PlayerData> sortedPlayers;
	std::vector<Player>& players = DemoData->Players;

	// Get all players.
	for(size_t i = 0; i < players.size(); ++i)
	{
		Player& player = players[i];
		if(player.Color.alpha() == 0)
		{
			continue;
		}

		// Draw living player.
		int number = player.ClientIndex * 2;
		const Entity* entity = &entities[number];
		int idx = entity->Index;

		// Collect data for both the living and dead player entities.
		Demo::PlayerInfo* living = NULL;
		Demo::PlayerInfo* dead = NULL;

		float livingAlpha = 0.0f;
		float deadAlpha = 0.0f;
		
		if(idx >= 0 && idx < (int)demo->_playerPlaybackInfos.size())
		{
			living = &(demo->_playerPlaybackInfos.at(idx));
			livingAlpha = entity->SyncCoolDown / (float)(UDT_DV_SYNC_MAX);
		}
		number = player.ClientIndex * 2 + 1;
		entity = &entities[number];
		idx = entity->Index;
		if(idx >= 0 && idx < (int)demo->_playerPlaybackInfos.size())
		{
			dead = &(demo->_playerPlaybackInfos.at(idx));
			deadAlpha = entity->SyncCoolDown / (float)(UDT_DV_SYNC_MAX);
		}
		
		// Draw the one closer in time.
		if(livingAlpha >= deadAlpha && living && livingAlpha > 0)
		{
			PlayerData playerData;
			playerData.DemoPlayer = living;
			playerData.PlayerInfo = &player;
			playerData.Alpha = livingAlpha;
			playerData.Dead = false;
			sortedPlayers.push_back(playerData);
		}
		
		if(dead)
		{
			PlayerData playerData;
			playerData.DemoPlayer = dead;
			playerData.PlayerInfo = &player;
			playerData.Alpha = deadAlpha;
			playerData.Dead = true;
			sortedPlayers.push_back(playerData);
		}
	}

	// Draw living players and corpses in bottom-top order.
	std::sort(sortedPlayers.begin(), sortedPlayers.end());
	for(size_t i = 0; i < sortedPlayers.size(); ++i)
	{
		DrawPlayer(painter, sortedPlayers[i]);
	}

	DrawClock(painter);
	DrawHud(painter);
	DrawScores(painter);
	DrawPowerUps(painter);
}

bool PaintWidget::LoadMapImage(const QString& path)
{
	const QFileInfo info(path);
	if(!info.exists())
	{
		return false;	
	}
	
	if(_bgImage != NULL)
	{
		delete _bgImage;
	}

	_bgImage = new QImage(path);

	if(AdaptRenderScaleToWindowSize)
	{
		const float sx = (float)width()  / (float)_bgImage->width();
		const float sy = (float)height() / (float)_bgImage->height();
		float scale = std::min(sx, sy);

		if(scale > 1.0f) scale = 1.0f;

		RenderScale = scale;
	}
	else
	{
		QRect scaledRect;
		GetScaledRect(scaledRect);

		setMaximumSize(scaledRect.width(), scaledRect.height());
		setMinimumSize(scaledRect.width(), scaledRect.height());
	}

	return true;
}

void PaintWidget::ReleaseImage()
{
	if(_bgImage != NULL)
	{
		delete _bgImage;
		_bgImage = NULL;
	}
}

void PaintWidget::DrawPlayer(QPainter& painter, const PlayerData& data)
{
	if(data.DemoPlayer == NULL)
	{
		return;
	}

	const float x = data.DemoPlayer->Position[0];
	const float y = data.DemoPlayer->Position[1];
	const float z = data.DemoPlayer->Position[2];

	const int x0 = (int)( (x - (float)_mapOrigin[0]) * _coordsScale);
	const int y0 = (int)(-(y - (float)_mapOrigin[1]) * _coordsScale);
	const int z0 = (int)( (z - (float)_mapOrigin[2]) * _coordsScale);

	const float orientation = data.DemoPlayer->Angles[1];
	const float fovAngle = 90.0f;

	if(data.Dead)
	{
		DrawDeadPlayer(painter, x0, y0, data.PlayerInfo->Color, data.Alpha);
		return;
	}

	DrawViewAngle(painter, QPoint(x0,y0), data.PlayerInfo->Color, orientation, fovAngle, 50, data.Alpha);
	DrawWeapon(painter, x0, y0, z0, orientation, data.Alpha, data.DemoPlayer->CurrentWeapon, data.DemoPlayer->Firing);
	DrawLivingPlayer(painter, x0, y0, z0, data.PlayerInfo->Color, data.Alpha);
	DrawPlayerPowerup(painter, x0, y0, z0, data.PlayerInfo);

	if(data.PlayerInfo->Name.isEmpty())
	{
		return;
	}

	QFont font;			
	font.setPixelSize(20);
	painter.setFont(font);
	if(data.Alpha < 1.0f)
	{
		QPen pen;
		pen.setColor(QColor(0, 0, 0, 128));
		painter.setPen(pen);
	}

	const QFontMetrics fm(font);
	const int deltaX = fm.width(data.PlayerInfo->Name) / 2;
	painter.drawText(x0 - deltaX, y0 - 30, data.PlayerInfo->Name);
}

void PaintWidget::DrawClock(QPainter& painter)
{
	if(!ShowClock || DemoData == NULL)
	{
		return;
	}

	const QString clockText =  DemoData->Clock.toString("mm:ss");
	QFont font;
	font.setPixelSize(30);
	const QFontMetrics fm(font);

	const int textWidth = fm.width(clockText);
	const int textHeight = fm.height();

	const int v = 15 + textHeight;
	const int h = 20;

	const QBrush brush(QColor(0, 0, 0, 0));
	painter.setBrush(brush);

	QPen pen(QColor(0, 0, 0, 255)); 
	pen.setWidth(1);
	painter.setPen(pen);

	painter.setFont(font);
	painter.drawText(h, v, clockText);
}

void PaintWidget::DrawScores(QPainter& painter)
{
	std::vector<ScoreEntry>& scoreTable = DemoData->Scores;
	if(scoreTable.empty() || !ShowScore)
	{
		return;
	}

	// @TODO: FFA etc support.
	if(scoreTable.size() != 2)
	{
		return;
	}
		
	QFont nameFont;
	nameFont.setPixelSize(20);
	const QFontMetrics nameFontMetric(nameFont);

	const QString leftPlayerName = scoreTable[0].Name;
	const QString rightPlayerName = scoreTable[1].Name;

	int leftPlayerNameWidth = nameFontMetric.width(leftPlayerName);
	int rightPlayerNameWidth = nameFontMetric.width(rightPlayerName);

	QFont scoreFont;
	scoreFont.setPixelSize(25);
	const QFontMetrics scoreFontMetric(scoreFont);

	const QString leftScore = " " + QString::number(scoreTable[0].Score) + " ";
	const QString rightScore = " " + QString::number(scoreTable[1].Score) + " ";

	const int leftScoreWidth = scoreFontMetric.width(leftScore);
	const int rightScoreWidth = scoreFontMetric.width(rightScore);

	const int pad = 0;

	QPen pen; 
	pen.setWidth(1);
	pen.setColor(QColor(127, 50, 50, 255));
	painter.setPen(pen);
	painter.setFont(nameFont);

	QRect renderTargetRect;
	GetUnscaledRect(renderTargetRect);

	const int y = 50;
	const int x = renderTargetRect.width() - (rightScoreWidth + rightPlayerNameWidth) - y;

	painter.drawText(x - leftPlayerNameWidth - leftScoreWidth - pad, y, leftPlayerName);
	painter.drawText(x + rightScoreWidth + pad, y, rightPlayerName);
	painter.drawText(x - scoreFontMetric.width("-") / 2, y, "-");

	pen.setColor(QColor(255, 100, 100, 255));
	painter.setPen(pen);
	painter.setFont(scoreFont);
	painter.drawText(x - leftScoreWidth - pad, y, leftScore);
	painter.drawText(x + pad, y, rightScore);
}

void PaintWidget::DrawPowerUps(QPainter& painter)
{
	if(DemoData == NULL || !ShowPowerUps)
	{
		return;
	}

	QFont fcFont;
	fcFont.setPixelSize(16);

	QPen pen; 
	pen.setWidth(1);
	pen.setColor(QColor(0, 0, 0, 255));
	painter.setPen(pen);
	painter.setFont(fcFont);

	const int w = 32;
	const int h = 32;

	const int xStart = 20;
	int y = 80;

	const int dy = h + 4;
	const int dyText = (h + fcFont.pixelSize()) / 2;

	std::vector<Player>& players = DemoData->Players;
	for(size_t i = 0; i < players.size(); ++i)
	{
		const Player& player = players[i];
		if(player.Team == TEAM_SPECTATOR)
		{
			continue;
		}

		int x = xStart;
		int pusDrawn = 0;
		for(int puIdx = 0; puIdx < 16; ++puIdx)
		{
			if(!player.Powerups[puIdx])
			{
				continue;
			}

			const int iconIdx = ItemIdxFromPowerUpIdx[puIdx];
			if(iconIdx == -1)
			{
				continue;
			}

			const QImage* const icon = GetItem(iconIdx, false);
			if(icon == NULL)
			{
				continue;
			}

			const QRect source(0, 0, icon->width(), icon->height());
			const QRect target(x, y, w, h);
			painter.drawImage(target, *icon, source);
			x += w + 4;
			++pusDrawn;
		}

		if(pusDrawn == 0)
		{
			continue;
		}

		painter.drawText(x, y + dyText, player.Name);
		y += dy;
	}
}

void PaintWidget::DrawHud(QPainter& painter)
{
	if(DemoData == NULL || !ShowHud || DemoData->FollowedPlayer == -1)
	{
		return;
	}

	const Player& player = DemoData->Players[DemoData->FollowedPlayer];

	QFont font;
	font.setPixelSize(20);
	painter.setFont(font);

	QRect renderTargetRect;
	GetUnscaledRect(renderTargetRect);

	const QString text = "Following " + player.Name;
	const int textLength = text.length();
	painter.drawText(renderTargetRect.width() / 2 - (text.length() / 2) * 10, renderTargetRect.height() - FollowingTextDeltaY - 30, text);

	DrawHudElement(painter, GetItem(ITEM_HEALTH, false), -120, QString::number(player.Health));
	DrawHudElement(painter, GetItem(ITEM_ARMOR_COMBAT, false), 80, QString::number(player.Armor));

	if(player.Weapon != 0 || _lastValidWeapon != 0)
	{
		const int weapon = player.Weapon != 0 ? player.Weapon : _lastValidWeapon;
		QImage* const weaponIcon = GetItem(weapon, false);
		const int w = (int)((float)weaponIcon->width() *_iconScale * 1.5f);
		DrawHudElement(painter, weaponIcon, -w / 2, QString::number(player.Ammo));
	}

	if(player.Weapon != 0)
	{
		_lastValidWeapon = player.Weapon;
	}
}

void PaintWidget::DrawHudElement(QPainter& painter, QImage* icon, int offsetX, const QString& text)
{
	QRect renderTargetRect;
	GetUnscaledRect(renderTargetRect);

	const int w = (int)((float)icon->width() *_iconScale * 1.5f);
	const int h = (int)((float)icon->height()*_iconScale * 1.5f);
	const int x = renderTargetRect.width() / 2 + offsetX;
	const int y = renderTargetRect.height() - FollowingTextDeltaY;
	QRect source(0, 0, icon->width(), icon->height());
	QRect target(x-w/2, y-h/2, w, h);
	painter.drawImage(target, *icon, source);
	painter.drawText(x + w/2 + 5 , y + 10, text);
}

void PaintWidget::LoadIcons(const QString& dirPath, const QStringList& iconPaths)
{
	for(size_t i = 0; i < _icons.size(); i++)
	{
		if(_icons[i].Image != NULL)
		{
			delete _icons[i].Image;
		}
	}

	_icons.clear();
	for(int i = 0; i < iconPaths.size(); ++i)
	{
		const QFileInfo fileInfo(iconPaths[i]);
		const QString fileNameNoExt = fileInfo.baseName();

		IconInfo info;
		info.Image = new QImage(dirPath + iconPaths[i]);
		info.FileName = fileNameNoExt.toLower();
		_icons.push_back(info);
	}
}

void PaintWidget::LoadItems(const QString& dirPath, const QStringList& itemPaths)
{
	for(size_t i = 0; i < _items.size(); i++)
	{
		if(_items[i].Image != NULL)
		{
			delete _items[i].Image;
		}
	}

	_items.clear();
	for(int i = 0; i < itemPaths.size(); ++i)
	{
		const QFileInfo fileInfo(itemPaths[i]);
		const QString fileNameNoExt = fileInfo.baseName();

		bool isInteger = false;
		const int index = fileNameNoExt.toInt(&isInteger);
		if(!isInteger)
		{
			continue;
		}

		ItemInfo info;
		info.Image = new QImage(dirPath + itemPaths[i]);
		info.Index = index;
		_items.push_back(info);
	}

	if(_proxyImage == NULL)
	{
		_proxyImage = CreateProxyImage(64, 64);
	}
}

void PaintWidget::LoadWeapons(const QString& dirPath, const QStringList& weaponsPath)
{
	for(size_t i = 0; i < _weapons.size(); i++)
	{
		if(_weapons[i] != NULL)
		{
			delete _weapons[i];
			_weapons[i] = NULL;
		}
	}

	_weapons.clear();
	for(int i = 0; i < weaponsPath.size(); i++)
	{
		QImage* image = new QImage(dirPath + weaponsPath[i]);
		_weapons.push_back(image);
	}
}

QImage* PaintWidget::GetItem(int type, bool respectProtocol)
{
	int type2 = type;
	if(DemoData->DemoParser->_protocol == Protocol::Dm68 && respectProtocol)
	{
		const int arraySize = (int)(sizeof(QlItemIndexFromQ3ItemIndex) / sizeof(QlItemIndexFromQ3ItemIndex[0]));
		if(type2 < 0 || type2 >= arraySize)
		{
			return _proxyImage;
		}

		type2 = QlItemIndexFromQ3ItemIndex[type2];
	}

	for(size_t i = 0; i < _items.size(); ++i)
	{
		if(type2 == _items[i].Index)
		{
			return _items[i].Image;
		}
	}

	return _proxyImage;
}

QImage* PaintWidget::GetIcon(const QString& fileName)
{
	const QString fileNameLc = fileName.toLower();
	for(size_t i = 0; i < _icons.size(); ++i)
	{
		if(fileNameLc == _icons[i].FileName)
		{
			return _icons[i].Image;
		}
	}

	return _proxyImage;
}

QImage* PaintWidget::GetWeapon(int type, bool firing)
{
	if(_weapons.size() < 8)
	{
		return _proxyImage;
	}

	const int delta = firing ? 1 : 0;
	switch(type)
	{
	case WEAPON_GAUNTLET:			return _weapons[0*2 + delta];
	case WEAPON_MACHINEGUN:			return _weapons[1*2 + delta];
	case WEAPON_SHOTGUN:			return _weapons[2*2 + delta];
	case WEAPON_GRENADELAUNCHER:	return _weapons[3*2 + delta];
	case WEAPON_ROCKETLAUNCHER:		return _weapons[4*2 + delta];
	case WEAPON_LIGHTNING:			return _weapons[5*2 + delta];
	case WEAPON_RAILGUN:			return _weapons[6*2 + delta];
	case WEAPON_PLASMAGUN:			return _weapons[7*2 + delta];
	default:						break;
	}
	
	return _proxyImage;
}

void PaintWidget::DrawViewAngle(QPainter& painter, const QPoint& center, const QColor& color, float orientation, float angle, float radius, float alpha)
{
	QPen pen(QColor(0, 0, 0, 255*alpha));
	QColor newColor = color;
	newColor.setAlpha(64 * alpha);
	const QBrush brush(newColor);
	painter.setBrush(brush);
	painter.setPen(pen);

	const QRectF rectangle(center.x() - radius, center.y() - radius, 2*radius, 2*radius);
	const int startAngle = (orientation - angle/2) * 16;
	const int spanAngle = (angle) * 16;
	painter.drawPie(rectangle, startAngle, spanAngle);
}

void PaintWidget::DrawLivingPlayer(QPainter &painter, int x, int y, int z, const QColor& color, float alpha)
{
	QColor newColor = color;

	QPen pen;
	pen.setWidthF(1.5);

	QBrush brush;
	brush.setStyle(Qt::SolidPattern);

	if(alpha != 1.0f)
	{
		pen.setStyle(Qt::DotLine);
		brush.setStyle(Qt::SolidPattern);
		newColor.setAlpha(alpha * 255);
	}

	const int radius = (int)(PlayerRadiusBase + (float)z * _heightScale);
	QColor c(color);
	c.setAlpha(alpha * 255);
	brush.setColor(c);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(x-radius/2, y-radius/2, radius, radius);
}

void PaintWidget::DrawPlayerPowerup(QPainter& painter, int x, int y, int z, const Player* player)
{
	if(player->Powerups[PW_QUAD])
	{
		DrawPlayerPowerupDisk(painter, x, y, z, PlayerRadiusQuad, QColor(0, 128, 255, 255), QColor(0, 128, 255, 128));
	}

	if(player->Powerups[PW_BATTLESUIT])
	{
		DrawPlayerPowerupDisk(painter, x, y, z, PlayerRadiusBs, QColor(255, 230, 0, 255), QColor(255, 230, 0, 128));
	}

	if(player->Powerups[PW_REDFLAG])
	{
		DrawPlayerPowerupImage(painter, x, y, z, GetItem(TEAM_CTF_REDFLAG, false));
	}

	if(player->Powerups[PW_BLUEFLAG])
	{
		DrawPlayerPowerupImage(painter, x, y, z, GetItem(TEAM_CTF_BLUEFLAG, false));
	}
}

void PaintWidget::DrawPlayerPowerupDisk(QPainter& painter, int x, int y, int z, int r, const QColor& color1, const QColor& color2)
{
	QPen pen(color1);
	pen.setWidth(2);
	painter.setPen(pen);

	QBrush brush(color2);
	brush.setStyle(Qt::SolidPattern);
	painter.setBrush(brush);

	const int radius = r + (int)((float)z * _heightScale);
	painter.drawEllipse(x-radius/2, y-radius/2, radius, radius);
}

void PaintWidget::DrawPlayerPowerupImage(QPainter& painter, int x, int y, int z, QImage* image)
{
	if(image == NULL)
	{
		return;
	}

	const QImage icon = QImage(*image);
	const int w = (int)((float)icon.width() * _iconScale);
	const int h = (int)((float)icon.height()* _iconScale);
	const QRect source(0, 0, icon.width(), icon.height());
	const QRect target(x-w/2, y-h/2, w, h);
	painter.drawImage(target, icon, source);
}

void PaintWidget::DrawWeapon(QPainter &painter, int x, int y, int z, float angle, float alpha, int weapon, bool firing)
{
	QImage* const icon = GetWeapon(weapon, firing);

	angle -= 90.0f;
	if(angle < 0.0f)
	{
		angle += 360.0f;
	}

	z = std::max(z, 0);
	const float scaleZ = (float)(300 + z) / 400.0f;

	painter.save();
	painter.translate(x,y);
	painter.rotate(-angle);
	painter.translate(13 * scaleZ, -10);

	const int w = (int)((float)icon->width() * _iconScale * 0.6f * scaleZ);
	const int h = (int)((float)icon->height()* _iconScale * 0.6f * scaleZ);
	const QRect source(0, 0, icon->width(), icon->height());
	const QRect target(-w/2,-h/2, w, h);					

	painter.drawImage(target, *icon, source);
	painter.restore();
}

void PaintWidget::DrawDeadPlayer(QPainter &painter, int x, int y, const QColor& color, float alpha)
{
	if(_items.size() < 22)
	{
		return;
	}

	QImage icon = *GetIcon("dead_player");
	SetImageAlpha(&icon, alpha);

	const int w = icon.width() * _iconScale;
	const int h = icon.height()* _iconScale;
	const QRect source(0, 0, icon.width(), icon.height());
	const QRect target(x-w/2, y-h/2, w, h);
	painter.drawImage(target, icon, source);
}

void PaintWidget::DrawItem(QPainter& painter, const Demo::EntityInfo* info, float alpha)
{
	if(info == NULL)
	{
		return;
	}

	const float x = info->Position[0];
	const float y = info->Position[1];
	const int x0 = (int)( (x - (float)_mapOrigin[0]) * _coordsScale);
	const int y0 = (int)(-(y - (float)_mapOrigin[1]) * _coordsScale);

	const QImage* iconImage = GetItem(info->ItemType);
	if(iconImage == NULL)
	{
		painter.drawEllipse(x0, y0, 10, 10);
		return;	
	}

	QImage icon = QImage(*iconImage);
	if(alpha < 1.0f)
	{
		SetImageAlpha(&icon, alpha);
	}

	const int w = (int)((float)icon.width() * _iconScale);
	const int h = (int)((float)icon.height()* _iconScale);
	const QRect source(0, 0, icon.width(), icon.height());
	const QRect target(x0-w/2, y0-h/2, w, h);
	painter.drawImage(target, icon, source);
}

void PaintWidget::DrawProjectile(QPainter& painter, const Demo::EntityInfo* info)
{
	if(info == NULL)
	{
		return;
	}

	const float x = info->Position[0];
	const float y = info->Position[1];
	const int x0 = (int)( (x - (float)_mapOrigin[0]) * _coordsScale);
	const int y0 = (int)(-(y - (float)_mapOrigin[1]) * _coordsScale);

	QPen pen;
	QBrush brush;
	QColor color = Qt::white;
	int size = 10;

	switch(info->ProjectileType)
	{
	case Demo::ProjectileType::NotAProjectile:
		{
			return;
		}
	case Demo::ProjectileType::Rocket:
		{
			QImage* icon = GetIcon("rocket");
			if(icon != NULL)
			{
				const float angle = RadToDeg(info->Angle);
				painter.save();
				painter.translate(x0, y0);
				painter.rotate(angle);

				const int w = (int)((float)icon->width() * _iconScale * 1.5f);
				const int h = (int)((float)icon->height()* _iconScale * 1.5f);
				const QRect source(0, 0, icon->width(), icon->height());
				const QRect target(-w/2, -h/2, w, h);					
				painter.drawImage(target, *icon, source);
				painter.restore();
			}
			return;
		}
	case Demo::ProjectileType::PlasmaBall:
		{
			color = QColor(20, 100, 255);
			size = 4;
			pen.setWidth(1);
			brush.setColor(color);
			brush.setStyle(Qt::SolidPattern);
			painter.setPen(pen);
			painter.setBrush(brush);
			painter.drawEllipse(x0 - size, y0 - size, size*2, size*2);
			return;
		}
	case Demo::ProjectileType::Grenade:
		{
			color = QColor(20, 180, 50);
			size = 5;
			pen.setWidth(1);
			brush.setColor(color);
			brush.setStyle(Qt::SolidPattern);
			painter.setPen(pen);
			painter.setBrush(brush);
			painter.drawEllipse(x0 - size, y0 - size, size*2, size*2);
			return;
		}
	}
}

void PaintWidget::DrawGeneric(QPainter& painter, const Demo::EntityInfo* info)
{
	if(info == NULL)
	{
		return;
	}

	const float x = info->Position[0];
	const float y = info->Position[1];
	const int x0 = (int)( (x - (float)_mapOrigin[0]) * _coordsScale);
	const int y0 = (int)(-(y - (float)_mapOrigin[1]) * _coordsScale);

	QPen pen;
	QBrush brush;
	QColor color = Qt::white;
	int size = 10;

	switch(info->GenericType)
	{
	case Demo::GenericType::RocketSplash:
		{
			QImage* icon = GetIcon("explosion");
			if(icon != NULL)
			{
				const float angle = RadToDeg(info->Angle);
				painter.save();
				painter.translate(x0, y0);
				painter.rotate(angle);

				const int w = (int)((float)icon->width() * _iconScale * 1.5f);
				const int h = (int)((float)icon->height()* _iconScale * 1.5f);
				const QRect source(0, 0, icon->width(), icon->height());
				const QRect target(-w/2, -h/2, w, h);
				painter.drawImage(target, *icon, source);
				painter.restore();
			}
			return;
		}
	case Demo::GenericType::GrenadeSplash:
		{
			QImage* icon = GetIcon("explosion");
			if(icon != NULL)
			{
				const float angle = RadToDeg(info->Angle);
				painter.save();
				painter.translate(x0, y0);
				painter.rotate(angle);

				const int w = (int)((float)icon->width() * _iconScale * 1.5f);
				const int h = (int)((float)icon->height()* _iconScale * 1.5f);
				const QRect source(0, 0, icon->width(), icon->height());
				const QRect target(-w/2, -h/2, w, h);
				painter.drawImage(target, *icon, source);
				painter.restore();
			}
			return;
		}
	case Demo::GenericType::Hit:
	case Demo::GenericType::Miss:
		color = QColor(0, 200, 255);
		size = 3;
		break;
	case Demo::GenericType::PlasmaSplash:
		color = QColor(0, 160, 255);
		size = 6;
		break;
	case Demo::GenericType::LgSplash:
		color = QColor(0, 255, 255);
		size = 4;
		break;
	default:
		return;
	}

	pen.setWidth(1);
	brush.setColor(color);
	brush.setStyle(Qt::SolidPattern);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(x0 - size, y0 - size, size*2, size*2);

	if(info->GenericType == Demo::GenericType::Hit)
	{
		const QPen pen(QColor(0, 200, 255));
		painter.setPen(pen);
	}
}

void PaintWidget::DrawBeams(QPainter& painter, const float* startPositions, const float* endPositions, Beam::Type::Id beamType, float alpha)
{
	const float deltaX = endPositions[0] - startPositions[0];
	const float deltaY = endPositions[1] - startPositions[1];

	const float length = sqrt(deltaX*deltaX + deltaY*deltaY);

	const float q =  deltaY / length;
	const float r = -deltaX / length;

	const float x = startPositions[0] + q*30 - r*60;
	const float y = startPositions[1] + r*30 + q*60;

	const int x1 =  (x - _mapOrigin[0]) * _coordsScale;
	const int y1 = -(y - _mapOrigin[1]) * _coordsScale;

	const float z = endPositions[0];
	const float w = endPositions[1];

	const int x2 =  (z - _mapOrigin[0]) * _coordsScale;
	const int y2 = -(w - _mapOrigin[1]) * _coordsScale;

	bool drawLine = true;
	QPen pen;
	switch(beamType)
	{
	case Beam::Type::LG:
		{
			pen.setWidth(5);
			pen.setColor(QColor(0, 200, 255, (int)(80.0f*alpha)));
			painter.setPen(pen);
			break;
		}
	case Beam::Type::RG:
		{
			pen.setWidth(2);
			pen.setColor(QColor(0, 0, 0, 0));
			painter.setPen(pen);

			QBrush brush(QColor(255, 128, 0, (int)(255.0f*alpha)));
			painter.setBrush(brush);

			const int n = 100;
			for(int i = 0; i < n; ++i) 
			{
				const int x = x1 + (x2-x1)*i/(n-1);
				const int y = y1 + (y2-y1)*i/(n-1);
				const qreal distance = 4;
				const qreal circleRadius = 2;
				painter.save();
				painter.translate(x, y);
				painter.rotate(i*(55.6 + 5*alpha));
				painter.drawEllipse(0, distance, circleRadius*2, circleRadius*2);
				painter.restore();
			}
			pen.setColor(QColor(255, 0, 0, (int)(80.0f*alpha)));
			painter.setPen(pen);

			break;
		}
	default:
		drawLine = false;
		break;
	}

	if(drawLine)
	{
		painter.drawLine(x1, y1, x2, y2);
	}
}

void PaintWidget::SetImageAlpha(QImage* image, float alpha)
{
	for(int y = 0; y < image->height(); ++y) 
	{
		for(int x = 0; x < image->height(); ++x) 
		{
			QColor srcColor = QColor::fromRgba(image->pixel(x, y));
			const int dstAlpha = (int)(255.0f * alpha);
			const int srcAlpha = srcColor.alpha();
			if(dstAlpha < srcAlpha)
			{
				srcColor.setAlpha(dstAlpha);
				image->setPixel(x, y, srcColor.rgba());
			}
		}
	}
}

void PaintWidget::GetUnscaledRect(QRect& rect)
{
	rect = QRect(0, 0, 600, 600);
	if(_bgImage != NULL)
	{
		rect.setWidth(_bgImage->width());
		rect.setHeight(_bgImage->height());
	}
}

void PaintWidget::GetScaledRect(QRect& rect)
{
	GetUnscaledRect(rect);
	const int w = (int)(RenderScale * (float)rect.width());
	const int h = (int)(RenderScale * (float)rect.height());
	rect = QRect(0, 0, w, h);
}

void PaintWidget::ComputeRenderScale()
{
	QRect unscaledRect;
	GetUnscaledRect(unscaledRect);

	const float sx = (float)width() / (float)unscaledRect.width();
	const float sy = (float)height() / (float)unscaledRect.height();
	
	float scale = std::min(sx, sy);
	scale = std::min(scale, 1.0f);

	RenderScale = scale;
}






