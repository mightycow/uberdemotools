#pragma once

#include <QWidget>
#include <QPaintEvent>
#include "demo.hpp"
#include "demo_player.h"

class PaintWidget : public QWidget
{
	Q_OBJECT

public:
	PaintWidget(QWidget *parent);
	~PaintWidget();

	void releaseImage();
	bool loadImage(QString imagePath);
	void loadIcons(QString dirPath, QStringList iconsPath);
	void loadWeapons(QString dirPath, QStringList weaponsPath);

	void resetScaling();
	void setScaling(int* origin, int* end);

	Demo* demo;
	std::vector<DemoPlayer::Player>* players;
	std::vector<DemoPlayer::Player>* spectators;
	std::vector<DemoPlayer::Entity>* entities;	
	std::vector<DemoPlayer::Beam>* beams;
	std::vector<DemoPlayer::ScoreEntry>* scoreTable;
	QTime* clock;
	int* warmupTime;
	
	bool displayDemo;
	bool showClock;
	bool showScore;
	bool showHud;
	float iconScale;
	QString bgMessage;

protected:
	
	void paintEvent(QPaintEvent* event);
	void paintDemo(QPainter& painter);
	void drawClock(QPainter& painter);
	void drawHud(QPainter& painter);
	void drawScores(QPainter& painter);

	struct PlayerData
	{
		Demo::PlayerInfo* player;
		DemoPlayer::Player* dPPlayer;
		float alpha;
		bool dead;

		PlayerData() : player(NULL), dPPlayer(NULL), alpha(1.0f), dead(false){}

		bool operator < (const PlayerData& rhs) const
		{
			return this->player->Position[2] < rhs.player->Position[2];
		}
	};

	void drawPlayer(QPainter& painter, PlayerData data);

	void drawAlivePlayer(QPainter &painter, int a, int b, int c, QColor &color, float alpha);
	void drawDeadPlayer(QPainter &painter, int a, int b, QColor &color, float alpha);
	void drawPlayerPowerup(QPainter &painter, int a, int b, int c, DemoPlayer::Player* player);
	void drawWeapon(QPainter &painter, int a, int b, int c, float angle, float alpha, int weapon, bool firing);

	void drawItem(QPainter& painter, Demo::EntityInfo* info, float alpha);
	void drawProjectile(QPainter& painter, Demo::EntityInfo* info);
	void drawGeneric(QPainter& painter, Demo::EntityInfo* info);
	void drawBeams(QPainter& painter, float* startPosition, float* endPosition, DemoPlayer::Beam::Type::Id type, float alpha = 1.0f);

	void setAlphaOnTransparentImage(QImage* image, float alpha);

	void drawViewAngle(QPainter& painter, QPoint center, QColor color, float orientation, float angle, float radius);
	QImage* getIcon(int type);
	QImage* getWeapon(int type, bool firing);


private:
	QImage* bgImage;	
	int mapOrigin[3];
	int mapEnd[3];
	float coordsScaling;
	float heightScaling;

	std::vector<QImage*> icons;
	std::vector<QImage*> weapons;
	QImage* iconProxy;

public slots:	
};
