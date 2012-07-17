#include "paint_widget.h"
#include <QPainter>
#include <QFileInfo>
#include <QStaticText>
#include <QPainterPathStroker>
#include <QStringList>
#include <QRect>
#include <QString>
#include <qDebug>


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
	bgImage = NULL;
	iconProxy = NULL;
	demo = NULL;
	players = NULL;
	entities = NULL;
	beams = NULL;
	clock = NULL;
	showClock = true;
	showScore = true;
	showHud = true;
	float scaling = 1.0;
	iconScale = 0.4f;

	displayDemo = false;
	bgMessage = "Drag and drop a demo here.";

	resetScaling();
}


PaintWidget::~PaintWidget()
{
	if(bgImage != NULL)
		delete bgImage;

	for(size_t i = 0; i < icons.size(); i++)
	{
		if(icons[i] != NULL)
		{
			delete icons[i];
			icons[i] = NULL;

		}
	}

	for(size_t i = 0; i < weapons.size(); i++)
	{
		if(weapons[i] != NULL)
		{
			delete weapons[i];
			weapons[i] = NULL;

		}
	}
}

void PaintWidget::resetScaling()
{
	mapOrigin[0] = mapOrigin[1] = mapOrigin[2] = 0;
	mapEnd[0] = mapEnd[1] = mapEnd[2] = 100;

	this->resize(800, 800);

	coordsScaling = this->width() / (float)(mapEnd[0] - mapOrigin[0]);
	heightScaling = 20 / (float) (mapEnd[2] - mapOrigin[2]);
}


void PaintWidget::setScaling( int* origin, int* end )
{
	mapOrigin[0] = origin[0];
	mapOrigin[1] = origin[1];
	mapOrigin[2] = origin[2];

	mapEnd[0] = end[0];
	mapEnd[1] = end[1];
	mapEnd[2] = end[2];


	int w = (bgImage == NULL) ? this->width() : bgImage->width();

	coordsScaling = w  / (float) (end[0] - origin[0]);
	heightScaling = 20 / (float) (end[2] - origin[2]);
}


void PaintWidget::paintEvent( QPaintEvent* event )
{
	QPainter painter(this);
	painter.setRenderHints(QPainter::Antialiasing, true);

	painter.fillRect(this->rect(), Qt::gray);

	QFont font;
	font.setPixelSize(30);
	painter.setFont(font);
	
	QFontMetrics fm(font);
	int pos = fm.width(bgMessage) / 2;
	
	painter.drawText(this->width() / 2 - pos, this->height() / 2, bgMessage);

	if(displayDemo)
		paintDemo(painter);
}


void PaintWidget::paintDemo( QPainter& painter )
{
	painter.setBrushOrigin(this->geometry().x(), this->geometry().y());
	if(bgImage != NULL)
	{
		QPoint p(0,0);
		painter.drawImage(0, 0, *bgImage);
	}

	if(entities != NULL)
	{
		for(size_t i = 128; i < entities->size(); i++)
		{
			if(entities->at(i).syncCooldown > 0)
			{
				float alpha = entities->at(i).syncCooldown / (float)(SYNCMAX);

				Demo::EntityInfo* p = &(demo->_entityPlaybackInfos.at(entities->at(i).index));

				switch(p->Type)
				{
				case Demo::EntityType::Item:
					drawItem(painter, p, alpha);
					break;
				case Demo::EntityType::Projectile:
					drawProjectile(painter, p);
					break;
				case Demo::EntityType::Generic:
					drawGeneric(painter, p);
					break;
				}
			}
		}
	}

	if(beams != NULL)
	{
		for(size_t i = 0; i < beams->size(); i++)
		{
			if(beams->at(i).TTL > 0)
			{
				if(beams->at(i).type == DemoPlayer::Beam::Type::RG)
					drawBeams(painter, beams->at(i).startPosition, beams->at(i).endPosition, beams->at(i).type, beams->at(i).TTL / 500.0f);
				else
					drawBeams(painter, beams->at(i).startPosition, beams->at(i).endPosition, beams->at(i).type, beams->at(i).TTL / 50.0f);
			}
		}
	}

	if(players != NULL)
	{
		for(size_t i = 0; i < players->size(); i++)
		{
			if(players->at(i).color.alpha() == 0)
				continue;

			// Draw alive player
			int number = players->at(i).clientNum * 2;
			int idx = (entities->at(number)).index;

			// Collect data for both the alive and dead player entities
			Demo::PlayerInfo* alive = NULL;
			Demo::PlayerInfo* dead = NULL;

			float aliveAlpha = 0;
			float deadAlpha = 0;

			if(idx >= 0 && idx < (int)demo->_playerPlaybackInfos.size())
			{
				alive = &(demo->_playerPlaybackInfos.at(idx));
				aliveAlpha = (entities->at(number)).syncCooldown / (float)(SYNCMAX);
			}
			number = players->at(i).clientNum * 2 + 1;
			idx = (entities->at(number)).index;
			if(idx >= 0 && idx < (int)demo->_playerPlaybackInfos.size())
			{
				dead = &(demo->_playerPlaybackInfos.at(idx));
				deadAlpha = (entities->at(number)).syncCooldown / (float)(SYNCMAX);
			}

			// Draw the one closer in time
			if(aliveAlpha >= deadAlpha && alive)
			{
				drawPlayer(
					painter, 
					alive, 
					players->at(i).color, 
					players->at(i).name, 
					aliveAlpha,
					false);
			}

			if(dead)
			{
				drawPlayer(
					painter, 
					dead, 
					players->at(i).color, 
					players->at(i).name, 
					deadAlpha,
					true);
			}
		}

		scoreTable.clear();
		for(size_t i = 0; i < players->size(); i++)
		{
			ScoreEntry e;
			e.name = players->at(i).name;
			e.score = players->at(i).score;
			scoreTable.push_back(e);
		}
		if(scoreTable.size() > 2)
		{
			std::sort(scoreTable.begin(), scoreTable.end());
			std::reverse(scoreTable.begin(), scoreTable.end());
		}
	}
	drawClock(painter);
	drawHud(painter);
	drawScores(painter);
}

bool PaintWidget::loadImage(QString path)
{
	QFileInfo info(path);

	if(info.exists())
	{
		if(bgImage != NULL)
			delete bgImage;

		bgImage = new QImage(path);
		this->setMaximumSize(bgImage->width(), bgImage->height());
		this->setMinimumSize(bgImage->width(), bgImage->height());
		return true;
	}
	else
		return false;
}

void PaintWidget::releaseImage()
{
	if(bgImage != NULL)
	{
		delete bgImage;
		bgImage = NULL;
	}
}

void PaintWidget::drawPlayer(QPainter& painter, Demo::PlayerInfo* pI2D, QColor color, QString name, float alpha, bool dead )
{
	if(pI2D != NULL)
	{
		float x = pI2D->Position[0];
		float y = pI2D->Position[1];
		float z = pI2D->Position[2];

		int a = (x - mapOrigin[0]) * coordsScaling;
		int b = -(y - mapOrigin[1]) * coordsScaling;
		int c = (z - mapOrigin[2]) * coordsScaling;

		float orientation = pI2D->Angles[1];
		float angle = 90; // FOV

		if(!dead)
		{
			drawViewAngle(painter, QPoint(a,b), color, orientation, angle, 50);
			drawWeapon(painter, a, b, orientation, alpha, pI2D->CurrentWeapon);
			drawAlivePlayer(painter, a, b, c, color, alpha);
			
			if(!name.isEmpty())
			{
				QFont font;			
				font.setPixelSize(20);
				painter.setFont(font);
				if(alpha < 1.0f)
				{
					QPen pen;
					pen.setColor(QColor(0, 0, 0, 128));
					painter.setPen(pen);
				}
				QFontMetrics fm(font);
				int shift = fm.width(name) / 2;
				painter.drawText(a - shift, b - 15, name);
			}
		}
		else
		{
			drawDeadPlayer(painter, a, b, color, alpha);
		}
		
	}
}

void PaintWidget::drawClock(QPainter& painter)
{
	if(showClock && clock != NULL)
	{	

		QString text = clock->toString("mm:ss");
		QFont font;
		font.setPixelSize(30);
		QFontMetrics fm(font);

		int textWidth = fm.width(text);
		int textHeight = fm.height();

		int v = 15 + textHeight;
		int h = 20;

		QBrush brush(QColor(0, 0, 0, 0));
		painter.setBrush(brush);
		QPen pen(QColor(0, 0, 0, 255)); pen.setWidth(1);
		painter.setPen(pen);

		/*if(*warmupTime < 0)
			painter.drawText(this->width() / 2 - 45, v + 30, "(Warmup)");*/

		/*QBrush brush(QColor(0,0,0, 16));
		painter.setBrush(brush);
		QPen pen; pen.setWidth(1);
		painter.setPen(pen);

		QPolygon polygon;
		polygon.append(QPoint(30, v - 30));
		polygon.append(QPoint(120, v - 30));
		polygon.append(QPoint(120, v + 10));
		polygon.append(QPoint(30, v + 10));
		painter.drawPolygon(polygon, Qt::FillRule::OddEvenFill);*/

		
		painter.setFont(font);
		painter.drawText(h, v, clock->toString("mm:ss"));
	}
}

void PaintWidget::drawScores( QPainter& painter )
{
	if(scoreTable.empty() || !showScore)
		return;

	/*if(scoreTable.size() > 2)
	{
		int x = 30;
		int y = 30;
		for(size_t i = 0; i < scoreTable.size(); i++)
		{
			QFont font;
			font.setPixelSize(15);
			painter.setFont(font);

			painter.drawText(x, y, scoreTable[i].name + " " + QString::number(scoreTable[i].score));

			y += 20;
		}
	}
	else */if(scoreTable.size() == 2)
	{
		int y = 30;

		QFont nameFont;
		nameFont.setPixelSize(20);
		QFontMetrics nameFontMetric(nameFont);
		
		QString leftPlayer = scoreTable[0].name;
		QString rightPlayer = scoreTable[1].name;

		int leftPlayerWidth = nameFontMetric.width(leftPlayer);
		int rightPlayerWidth = nameFontMetric.width(rightPlayer);

		QFont scoreFont;
		scoreFont.setPixelSize(25);
		QFontMetrics scoreFontMetric(scoreFont);

		QString leftScore = " " + QString::number(scoreTable[0].score) + " ";
		QString rightScore = " " + QString::number(scoreTable[1].score) + " ";

		int leftScoreWidth = scoreFontMetric.width(leftScore);
		int rightScoreWidth = scoreFontMetric.width(rightScore);

		int pad = 0; //scoreFontMetric.width(" - ") / 2;
		

		QPen pen(QColor(100, 50, 50, 255)); pen.setWidth(1);
		painter.setPen(pen);
		painter.setFont(nameFont);
		painter.drawText(this->width() / 2 - leftPlayerWidth - leftScoreWidth - pad, y, leftPlayer);
		painter.drawText(this->width() / 2 + rightScoreWidth + pad, y, rightPlayer);
		painter.drawText(this->width() / 2 - scoreFontMetric.width("-") / 2, y, "-");

		pen.setColor(QColor(255, 100, 100, 255));
		painter.setPen(pen);
		painter.setFont(scoreFont);
		painter.drawText(this->width() / 2 - leftScoreWidth - pad, y, leftScore);
		painter.drawText(this->width() / 2 + pad, y, rightScore);

		/*QString scoreline = leftPlayer + " - " + rightPlayer;
		int boxWidth = fm.width(" " + scoreline + " ");
		int boxHeight = fm.height();
		int boxShift = boxWidth - leftWidth;

		painter.save();
		QBrush brush(QColor(100,50,50, 16));
		painter.setBrush(brush);
		QPen pen(QColor(100, 50, 50, 255)); pen.setWidth(1);
		painter.setPen(pen);

		QPolygon polygon;
		polygon.append(QPoint(this->width() / 2 - leftWidth  - 2, y - boxHeight / 1 ));
		polygon.append(QPoint(this->width() / 2 + rightWidth + 2, y - boxHeight / 1 ));
		polygon.append(QPoint(this->width() / 2 + rightWidth + 2, y + boxHeight / 2 ));
		polygon.append(QPoint(this->width() / 2 - leftWidth  - 2, y + boxHeight / 2 ));
		painter.drawPolygon(polygon, Qt::FillRule::OddEvenFill);
		painter.restore();*/

	}
}


void PaintWidget::drawHud( QPainter& painter )
{
	if(demo == NULL || !showHud)
		return;

	int v = 35;

	for(size_t i = 0; i < players->size(); i++)
	{
		if(players->at(i).demoTaker)
		{
			const DemoPlayer::Player& p = players->at(i);

			QFont font;
			font.setPixelSize(20);
			painter.setFont(font);
			QString text = "Following " + p.name;
			int textLen = text.length();
			painter.drawText(this->width() / 2 - (text.length() / 2) * 10, this->height() - v - 30, text);

			bool displayHealth = true;
			bool displayArmor = true;
			bool displayWeapon = true;

			if(displayHealth)
			{
				QImage* icon = getIcon(ITEM_HEALTH);

				int w = icon->width() * iconScale * 1.5f;
				int h = icon->height()* iconScale * 1.5f;
				int a = this->width() / 2 - 120;
				int b = this->height() - v;
				QRect source(0, 0, icon->width(), icon->height());
				QRect target(a-w/2, b-h/2, w, h);
				painter.drawImage(target, *icon, source);
				painter.drawText(a + w/2 + 5 , b + 10, QString::number(p.health));

			}
			if(displayArmor)
			{
				QImage* icon = getIcon(ITEM_ARMOR_COMBAT);

				int w = icon->width() * iconScale * 1.5f;
				int h = icon->height()* iconScale * 1.5f;
				int a = this->width() / 2 + 80;
				int b = this->height() - v;
				QRect source(0, 0, icon->width(), icon->height());
				QRect target(a-w/2, b-h/2, w, h);
				painter.drawImage(target, *icon, source);
				painter.drawText(a + w/2 + 5 , b + 10, QString::number(p.armor));
			}
			if(displayWeapon && p.weapon != 0)
			{
				QImage* icon = getIcon(p.weapon);

				int w = icon->width() * iconScale * 1.5f;
				int h = icon->height()* iconScale * 1.5f;
				int a = this->width() / 2 - w/2;
				int b = this->height() - v;
				QRect source(0, 0, icon->width(), icon->height());
				QRect target(a-w/2, b-h/2, w, h);
				painter.drawImage(target, *icon, source);

				painter.drawText(a + w/2 + 5 , b + 10, QString::number(p.ammo));
				
			}
		}
	}
}

void PaintWidget::loadIcons(QString dirPath, QStringList iconsPath)
{
	for(size_t i = 0; i < icons.size(); i++)
	{
		if(icons[i] != NULL)
		{
			delete icons[i];
			icons[i] = NULL;
		}
	}

	icons.clear();
	for(int i = 0; i < iconsPath.size(); i++)
	{
		QImage* image = new QImage(dirPath + iconsPath[i]);
		icons.push_back(image);
	}

	if(iconProxy == NULL)
	{
		iconProxy = CreateProxyImage(64, 64);
	}
}


void PaintWidget::loadWeapons( QString dirPath, QStringList weaponsPath )
{
	for(size_t i = 0; i < weapons.size(); i++)
	{
		if(weapons[i] != NULL)
		{
			delete weapons[i];
			weapons[i] = NULL;
		}
	}

	weapons.clear();
	for(int i = 0; i < weaponsPath.size(); i++)
	{
		QImage* image = new QImage(dirPath + weaponsPath[i]);
		weapons.push_back(image);
	}
}


QImage* PaintWidget::getIcon(int type )
{
	if(icons.size() < 21)
	{
		return iconProxy;
	}

	switch(type)
	{
		case ITEM_INVALID:				return icons[0];
		case ITEM_ARMOR_SHARD:			return icons[1];
		case ITEM_ARMOR_COMBAT:			return icons[2];
		case ITEM_ARMOR_BODY:			return icons[3];
		case ITEM_ARMOR_GREEN:			return icons[4];
		case ITEM_HEALTH_SMALL:			return icons[5];
		case ITEM_HEALTH:				return icons[6];
		case ITEM_HEALTH_LARGE:			return icons[7];
		case ITEM_HEALTH_MEGA :			return icons[8];
		case WEAPON_SHOTGUN:			return icons[9];
		case WEAPON_GRENADELAUNCHER:	return icons[10];
		case WEAPON_ROCKETLAUNCHER:		return icons[11];
		case WEAPON_LIGHTNING:			return icons[12];
		case WEAPON_RAILGUN:			return icons[13];
		case WEAPON_PLASMAGUN:			return icons[14];
		case AMMO_SHELLS:				return icons[15];
		case AMMO_BULLETS:				return icons[16];
		case AMMO_GRENADES:				return icons[17];
		case AMMO_CELLS:				return icons[18];
		case AMMO_LIGHTNING:			return icons[19];
		case AMMO_ROCKETS:				return icons[20];
		case AMMO_SLUGS:				return icons[21];
		case WEAPON_GAUNTLET:			return icons[25];
		case WEAPON_MACHINEGUN:			return icons[26];
		default:						break;
	}
	return iconProxy;
}

QImage* PaintWidget::getWeapon( int type )
{
	if(weapons.size() < 8)
	{
		return iconProxy;
	}

	switch(type)
	{
	case WEAPON_GAUNTLET:			return weapons[0];
	case WEAPON_MACHINEGUN:			return weapons[1];
	case WEAPON_SHOTGUN:			return weapons[2];
	case WEAPON_GRENADELAUNCHER:	return weapons[3];
	case WEAPON_ROCKETLAUNCHER:		return weapons[4];
	case WEAPON_LIGHTNING:			return weapons[5];
	case WEAPON_RAILGUN:			return weapons[6];
	case WEAPON_PLASMAGUN:			return weapons[7];
	default:						return iconProxy;
	}
	
}


void PaintWidget::drawViewAngle( QPainter& painter, QPoint center, QColor color, float orientation, float angle, float radius)
{
	color.setAlpha(32);
	QBrush brush(color);
	painter.setBrush(brush);
	QPen pen; pen.setWidth(0.5);
	painter.setPen(pen);

	QRectF rectangle(center.x() - radius, center.y() - radius, 2*radius, 2*radius);
	int startAngle = (orientation - angle/2) * 16;
	int spanAngle = (angle) * 16;
	painter.drawPie(rectangle, startAngle, spanAngle);
}

void PaintWidget::drawAlivePlayer(QPainter &painter, int a, int b, int c, QColor &color, float alpha)
{
	QPen pen;
	pen.setWidth(2);
	QBrush brush;
	brush.setStyle(Qt::SolidPattern);
	if(alpha == 1.0f)
	{
	}
	else
	{
		pen.setStyle(Qt::DotLine);
		brush.setStyle(Qt::SolidPattern);
		color.setAlpha(alpha * 255);
	}

	int radius = 20 + c * heightScaling;

	brush.setColor(color);
	painter.setPen(pen);
	painter.setBrush(brush);
	painter.drawEllipse(a-radius/2, b-radius/2, radius, radius);
}


void PaintWidget::drawWeapon( QPainter &painter, int a, int b, float angle, float alpha, int weapon )
{
	QImage* icon = getWeapon(weapon);

	angle = angle - 90;
	if(angle < 0) angle += 360;

	painter.save();
	painter.translate(a,b);
	painter.rotate(-angle);
	painter.translate(10,-10);
	int w = icon->width() * iconScale * 0.6;
	int h = icon->height()* iconScale * 0.6;

	QRect source(0, 0, icon->width(), icon->height());
	QRect target(-w/2,-h/2, w, h);					

	painter.drawImage(target,*icon, source);
	painter.restore();
}


void PaintWidget::drawDeadPlayer( QPainter &painter, int a, int b, QColor &color, float alpha)
{
	if(icons.size() < 22)
		return;

	QImage icon = QImage(*icons[22]);

	int w = icon.width() * iconScale;
	int h = icon.height()* iconScale;

	setAlphaOnTransparentImage(&icon, alpha);

	QRect source(0, 0, icon.width(), icon.height());
	QRect target(a-w/2, b-h/2, w, h);

	painter.drawImage(target, icon, source);
	
}

void PaintWidget::drawItem( QPainter& painter, Demo::EntityInfo* info, float alpha)
{
	if(info != NULL)
	{
		float x = info->Position[0];
		float y = info->Position[1];

		int a = (x - mapOrigin[0]) * coordsScaling;
		int b = -(y - mapOrigin[1]) * coordsScaling;

		QImage* r = getIcon(info->ItemType);
		
		if(r != NULL)
		{
			QImage icon = QImage(*r);

			int w = icon.width() * iconScale;
			int h = icon.height()* iconScale;

			if(alpha < 1)
				setAlphaOnTransparentImage(&icon, alpha);

			QRect source(0, 0, icon.width(), icon.height());
			QRect target(a-w/2, b-h/2, w, h);

			painter.drawImage(target, icon, source);
		}
		else
			painter.drawEllipse(a, b, 10, 10);

		bool debug = false;

		if(debug)
		{
			QFont font;
			font.setPixelSize(10);		
			painter.setFont(font);
			painter.drawText(a, b/*-10*/, QString::number(info->Number));
			//painter.drawText(a, b+10, QString::number(info->number));
			//return;
		}
	}
}
void PaintWidget::drawProjectile( QPainter& painter, Demo::EntityInfo* info )
{
	if(info != NULL)
	{
		float x = info->Position[0];
		float y = info->Position[1];

		int a = (x - mapOrigin[0]) * coordsScaling;
		int b = -(y - mapOrigin[1]) * coordsScaling;
		
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
				QImage* icon = icons[23];

				if(icon != NULL)
				{
					painter.save();
					painter.translate(a,b);
					int angle = info->Angle / 3.1415 * 180;
					painter.rotate(angle);
					int w = icon->width() * iconScale * 1.5;
					int h = icon->height()* iconScale * 1.5;

					QRect source(0, 0, icon->width(), icon->height());
					QRect target(-w/2,-h/2, w, h);					

					painter.drawImage(target,*icon, source);
					painter.restore();

					/*painter.drawText(a, b, QString::number(info->delta));

					x = info->Base[0];
					y = info->Base[1];

					a = (x - mapOrigin[0]) * coordsScaling;
					b = -(y - mapOrigin[1]) * coordsScaling;
					size = 2;
					painter.drawEllipse(a - size, b - size, size*2, size*2);*/
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
				painter.drawEllipse(a - size, b - size, size*2, size*2);
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
				painter.drawEllipse(a - size, b - size, size*2, size*2);

				/*painter.drawText(a, b, QString::number(info->delta));

				x = info->Base[0];
				y = info->Base[1];

				a = (x - mapOrigin[0]) * coordsScaling;
				b = -(y - mapOrigin[1]) * coordsScaling;
				size = 2;
				painter.drawEllipse(a - size, b - size, size*2, size*2);*/

				return;
			}
		}
	}
}


void PaintWidget::drawGeneric( QPainter& painter, Demo::EntityInfo* info )
{
	if(info != NULL)
	{
		float x = info->Position[0];
		float y = info->Position[1];

		int a = (x - mapOrigin[0]) * coordsScaling;
		int b = -(y - mapOrigin[1]) * coordsScaling;

		QPen pen;
		QBrush brush;
		QColor color = Qt::white;
		int size = 10;

		switch(info->GenericType)
		{
		case Demo::GenericType::RocketSplash:
			{
				QImage* icon = icons[24];
				if(icon != NULL)
				{
					painter.save();
					int w = icon->width() * iconScale * 1.5;
					int h = icon->height()* iconScale * 1.5;

					painter.translate(a,b);
					int angle = info->Angle / 3.1415 * 180;
					painter.rotate(angle);

					QRect source(0, 0, icon->width(), icon->height());
					QRect target(-w/2, -h/2, w, h);

					painter.drawImage(target, *icon, source);
					painter.restore();
				}
				return;
			}
		case Demo::GenericType::GrenadeSplash:
			{
				QImage* icon = icons[24];
				if(icon != NULL)
				{
					painter.save();
					int w = icon->width() * iconScale * 1.5;
					int h = icon->height()* iconScale * 1.5;

					painter.translate(a,b);
					int angle = info->Angle / 3.1415 * 180;
					painter.rotate(angle);

					QRect source(0, 0, icon->width(), icon->height());
					QRect target(-w/2, -h/2, w, h);

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
		painter.drawEllipse(a - size, b - size, size*2, size*2);

		if(info->GenericType == Demo::GenericType::Hit)
		{
			QPen pen(QColor(0, 200, 255));
			painter.setPen(pen);
			QFont font;
			font.setPixelSize(15);			
			painter.setFont(font);
			painter.drawText(a + 15, b, QString("Hit!"));
		}
	}
}
void PaintWidget::drawBeams( QPainter& painter, float* startPosition, float* endPosition, DemoPlayer::Beam::Type::Id type, float alpha)
{
	float x = startPosition[0];
	float y = startPosition[1];

	int a = (x - mapOrigin[0]) * coordsScaling;
	int b = -(y - mapOrigin[1]) * coordsScaling;

	float z = endPosition[0];
	float w = endPosition[1];

	int c = (z - mapOrigin[0]) * coordsScaling;
	int d = -(w - mapOrigin[1]) * coordsScaling;

	bool stop = false;
	QPen pen;
	switch(type)
	{
	case DemoPlayer::Beam::Type::LG:
		{
			pen.setWidth(5);
			pen.setColor(QColor(0, 200, 255, 80*alpha));
			painter.setPen(pen);
			break;
		}
	case DemoPlayer::Beam::Type::RG:
		{
			pen.setWidth(2);
			pen.setColor(QColor(0, 0, 0, 0));
			painter.setPen(pen);

			QBrush brush(QColor(255,128,0,255*alpha));
			painter.setBrush(brush);


			int n = 100;
			for (int i = 0; i < n; ++i) 
			{
				painter.save();
				int x = a + (c-a)*i/(n-1);
				int y = b + (d-b)*i/(n-1);
				painter.translate(x, y);
				painter.rotate(i*(55.6 + 5*alpha));
				qreal distance = 4;
				qreal circleRadius = 2;
				painter.drawEllipse(0, distance, circleRadius*2, circleRadius*2);
				painter.restore();
			}
			pen.setColor(QColor(255, 0, 0, 80*alpha));
			painter.setPen(pen);

			break;
		}
	default:
		stop = true;
		break;
	}
	if(stop)
		return;
	
	painter.drawLine(a,b,c,d);
}

void PaintWidget::setAlphaOnTransparentImage( QImage* image, float alpha )
{
	for (int y = 0; y < image->height(); y++) {
		for (int x = 0; x < image->height(); x++) {
			QColor c = QColor::fromRgba(image->pixel(x, y));
			int des_a = 255*alpha;
			int a = c.alpha();
			if(des_a < a)
			{
				c.setAlpha(des_a);
				image->setPixel(x, y, c.rgba());
			}
		}
	}
}





