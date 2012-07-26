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

	for(size_t i = 0; i < _icons.size(); ++i)
	{
		if(_icons[i] != NULL)
		{
			delete _icons[i];
			_icons[i] = NULL;
		}
	}

	for(size_t i = 0; i < _weapons.size(); ++i)
	{
		if(_weapons[i] != NULL)
		{
			delete _weapons[i];
			_weapons[i] = NULL;
		}
	}
}

void PaintWidget::ResetScaling()
{
	_mapOrigin[0] = _mapOrigin[1] = _mapOrigin[2] = 0;
	_mapEnd[0] = _mapEnd[1] = _mapEnd[2] = 100;

	resize(800, 800);
	
	const float scaleX =  width()  / (float)(_mapEnd[0] - _mapOrigin[0]);
	const float scaleY = -height() / (float)(_mapEnd[1] - _mapOrigin[1]);
	_coordsScale = std::min(scaleX, scaleY);
	_heightScale = 20.0f / (float)(_mapEnd[2] - _mapOrigin[2]);
}

void PaintWidget::SetScaling(int* origin, int* end)
{
	_mapOrigin[0] = origin[0];
	_mapOrigin[1] = origin[1];
	_mapOrigin[2] = origin[2];

	_mapEnd[0] = end[0];
	_mapEnd[1] = end[1];
	_mapEnd[2] = end[2];

	const int w = (_bgImage == NULL) ? width()  : _bgImage->width();
	const int h = (_bgImage == NULL) ? height() : _bgImage->height();
	const float scaleX =  w / (float)(_mapEnd[0] - _mapOrigin[0]);
	const float scaleY = -h / (float)(_mapEnd[1] - _mapOrigin[1]);
	_coordsScale = std::min(scaleX, scaleY);
	_heightScale = 20.0f / (float)(end[2] - origin[2]);
}

void PaintWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing, true);
	painter.fillRect(rect(), Qt::gray);

	QFont font;
	font.setPixelSize(30);
	painter.setFont(font);
	
	const QFontMetrics fontMetrics(font);
	const int posX = fontMetrics.width(BackgroundMessage) / 2;
	painter.drawText(width() / 2 - posX, height() / 2, BackgroundMessage);
	
	if(DisplayDemo)
	{
		PaintDemo(painter);
	}
}

void PaintWidget::PaintDemo(QPainter& painter)
{
	painter.setBrushOrigin(geometry().x(), geometry().y());
	if(_bgImage != NULL)
	{
		painter.drawImage(0, 0, *_bgImage);
	}

	if(DemoData->Demo == NULL)
	{
		return;
	}

	Demo* const demo = DemoData->Demo;
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
		if(livingAlpha >= deadAlpha && living)
		{
			PlayerData playerData;
			playerData.DemoPlayer = living;
			playerData.Player = &player;
			playerData.Alpha = livingAlpha;
			playerData.Dead = false;
			sortedPlayers.push_back(playerData);
		}
		
		if(dead)
		{
			PlayerData playerData;
			playerData.DemoPlayer = dead;
			playerData.Player = &player;
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

bool PaintWidget::LoadImage(const QString& path)
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
	setMaximumSize(_bgImage->width(), _bgImage->height());
	setMinimumSize(_bgImage->width(), _bgImage->height());

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
		DrawDeadPlayer(painter, x0, y0, data.Player->Color, data.Alpha);
		return;
	}

	DrawViewAngle(painter, QPoint(x0,y0), data.Player->Color, orientation, fovAngle, 50);
	DrawWeapon(painter, x0, y0, z0, orientation, data.Alpha, data.DemoPlayer->CurrentWeapon, data.DemoPlayer->Firing);
	DrawLivingPlayer(painter, x0, y0, z0, data.Player->Color, data.Alpha);
	DrawPlayerPowerup(painter, x0, y0, z0, data.Player);

	if(data.Player->Name.isEmpty())
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
	const int deltaX = fm.width(data.Player->Name) / 2;
	painter.drawText(x0 - deltaX, y0 - 30, data.Player->Name);
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
		
	const int y = 30;

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
	painter.drawText(this->width() / 2 - leftPlayerNameWidth - leftScoreWidth - pad, y, leftPlayerName);
	painter.drawText(this->width() / 2 + rightScoreWidth + pad, y, rightPlayerName);
	painter.drawText(this->width() / 2 - scoreFontMetric.width("-") / 2, y, "-");

	pen.setColor(QColor(255, 100, 100, 255));
	painter.setPen(pen);
	painter.setFont(scoreFont);
	painter.drawText(this->width() / 2 - leftScoreWidth - pad, y, leftScore);
	painter.drawText(this->width() / 2 + pad, y, rightScore);
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
		int x = xStart;
		int pusDrawn = 0;
		const Player& player = players[i];
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

			const QImage* const icon = GetIcon(iconIdx);
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
	if(DemoData == NULL || !ShowHud)
	{
		return;
	}

	std::vector<Player>& players = DemoData->Players;
	for(size_t i = 0; i < players.size(); i++)
	{
		const Player& player = players[i];
		if(!player.DemoTaker)
		{
			continue;
		}

		QFont font;
		font.setPixelSize(20);
		painter.setFont(font);

		const QString text = "Following " + player.Name;
		const int textLength = text.length();
		painter.drawText(width() / 2 - (text.length() / 2) * 10, height() - FollowingTextDeltaY - 30, text);

		DrawHudElement(painter, GetIcon(ITEM_HEALTH), -120, QString::number(player.Health));
		DrawHudElement(painter, GetIcon(ITEM_ARMOR_COMBAT), 80, QString::number(player.Armor));

		if(player.Weapon != 0 || _lastValidWeapon != 0)
		{
			const int weapon = player.Weapon != 0 ? player.Weapon : _lastValidWeapon;
			QImage* const weaponIcon = GetIcon(weapon);
			const int w = (int)((float)weaponIcon->width() *_iconScale * 1.5f);
			DrawHudElement(painter, weaponIcon, -w / 2, QString::number(player.Ammo));
		}

		if(player.Weapon != 0)
		{
			_lastValidWeapon = player.Weapon;
		}
	}
}

void PaintWidget::DrawHudElement(QPainter& painter, QImage* icon, int offsetX, const QString& text)
{
	const int w = (int)((float)icon->width() *_iconScale * 1.5f);
	const int h = (int)((float)icon->height()*_iconScale * 1.5f);
	const int x = width() / 2 + offsetX;
	const int y = height() - FollowingTextDeltaY;
	QRect source(0, 0, icon->width(), icon->height());
	QRect target(x-w/2, y-h/2, w, h);
	painter.drawImage(target, *icon, source);
	painter.drawText(x + w/2 + 5 , y + 10, text);
}

void PaintWidget::LoadIcons(const QString& dirPath, const QStringList& iconsPath)
{
	for(size_t i = 0; i < _icons.size(); i++)
	{
		if(_icons[i] != NULL)
		{
			delete _icons[i];
			_icons[i] = NULL;
		}
	}

	_icons.clear();
	for(int i = 0; i < iconsPath.size(); i++)
	{
		QImage* image = new QImage(dirPath + iconsPath[i]);
		_icons.push_back(image);
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

QImage* PaintWidget::GetIcon(int type)
{
	if(_icons.size() < 35)
	{
		return _proxyImage;
	}

	switch(type)
	{
		case ITEM_INVALID:				return _icons[0];
		case ITEM_ARMOR_SHARD:			return _icons[1];
		case ITEM_ARMOR_COMBAT:			return _icons[2];
		case ITEM_ARMOR_BODY:			return _icons[3];
		case ITEM_ARMOR_GREEN:			return _icons[4];
		case ITEM_HEALTH_SMALL:			return _icons[5];
		case ITEM_HEALTH:				return _icons[6];
		case ITEM_HEALTH_LARGE:			return _icons[7];
		case ITEM_HEALTH_MEGA :			return _icons[8];
		case WEAPON_SHOTGUN:			return _icons[9];
		case WEAPON_GRENADELAUNCHER:	return _icons[10];
		case WEAPON_ROCKETLAUNCHER:		return _icons[11];
		case WEAPON_LIGHTNING:			return _icons[12];
		case WEAPON_RAILGUN:			return _icons[13];
		case WEAPON_PLASMAGUN:			return _icons[14];
		case AMMO_SHELLS:				return _icons[15];
		case AMMO_BULLETS:				return _icons[16];
		case AMMO_GRENADES:				return _icons[17];
		case AMMO_CELLS:				return _icons[18];
		case AMMO_LIGHTNING:			return _icons[19];
		case AMMO_ROCKETS:				return _icons[20];
		case AMMO_SLUGS:				return _icons[21];
		case WEAPON_GAUNTLET:			return _icons[25];
		case WEAPON_MACHINEGUN:			return _icons[26];
		case ITEM_QUAD:					return _icons[27];
		case ITEM_ENVIRO:				return _icons[28];
		case ITEM_HASTE:				return _icons[29];
		case ITEM_INVIS:				return _icons[30];
		case ITEM_REGEN:				return _icons[31];
		case ITEM_FLIGHT:				return _icons[32];
		case TEAM_CTF_REDFLAG:			return _icons[33];
		case TEAM_CTF_BLUEFLAG:			return _icons[34];
		default:						break;
	}

	return _proxyImage;
}

QImage* PaintWidget::GetWeapon( int type, bool firing )
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

void PaintWidget::DrawViewAngle(QPainter& painter, const QPoint& center, const QColor& color, float orientation, float angle, float radius)
{
	QPen pen;
	QColor newColor = color;
	newColor.setAlpha(32);
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

	const int radius = (int)(20.0f + (float)z * _heightScale);
	brush.setColor(color);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(x-radius/2, y-radius/2, radius, radius);
}

void PaintWidget::DrawPlayerPowerup(QPainter& painter, int x, int y, int z, const Player* player)
{
	if(player->Powerups[PW_QUAD])
	{
		DrawPlayerPowerupDisk(painter, x, y, z, 24, QColor(0, 128, 255, 255), QColor(0, 128, 255, 128));
	}

	if(player->Powerups[PW_BATTLESUIT])
	{
		DrawPlayerPowerupDisk(painter, x, y, z, 26, QColor(255, 230, 0, 255), QColor(255, 230, 0, 128));
	}

	if(player->Powerups[PW_REDFLAG])
	{
		DrawPlayerPowerupImage(painter, x, y, z, GetIcon(TEAM_CTF_REDFLAG));
	}

	if(player->Powerups[PW_BLUEFLAG])
	{
		DrawPlayerPowerupImage(painter, x, y, z, GetIcon(TEAM_CTF_BLUEFLAG));
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
	if(_icons.size() < 22)
	{
		return;
	}

	QImage icon = QImage(*_icons[22]);
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

	const QImage* iconImage = GetIcon(info->ItemType);
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
			QImage* icon = _icons[23];
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
			QImage* icon = _icons[24];
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
			QImage* icon = _icons[24];
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

		QFont font;
		font.setPixelSize(15);			
		painter.setFont(font);
		painter.drawText(x0 + 15, y0, QString("Hit!"));
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






