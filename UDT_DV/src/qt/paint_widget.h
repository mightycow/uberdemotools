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

	void resetScaling();
	void setScaling(int* origin, int* end);

	Demo* demo;
	std::vector<DemoPlayer::Player>* players;
	std::vector<DemoPlayer::Player>* spectators;
	std::vector<DemoPlayer::Entity>* entities;	
	std::vector<DemoPlayer::Beam>* beams;
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
	void drawPlayer(QPainter& painter, Demo::PlayerInfo* pI2D, QColor color, QString name = "", float alpha = 1.0f, bool dead = false);

	void drawAlivePlayer(QPainter &painter, int a, int b, int c, QColor &color, float alpha);
	void drawDeadPlayer(QPainter &painter, int a, int b, QColor &color, float alpha);

	void drawItem(QPainter& painter, Demo::EntityInfo* info, float alpha);
	void drawProjectile(QPainter& painter, Demo::EntityInfo* info);
	void drawGeneric(QPainter& painter, Demo::EntityInfo* info);
	void drawBeams(QPainter& painter, float* startPosition, float* endPosition, DemoPlayer::Beam::Type::Id type, float alpha = 1.0f);

	void setAlphaOnTransparentImage(QImage* image, float alpha);

	void drawViewAngle(QPainter& painter, QPoint center, QColor color, float orientation, float angle, float radius);
	QImage* getIcon(int type);


private:
	QImage* bgImage;	
	int mapOrigin[3];
	int mapEnd[3];
	float coordsScaling;
	float heightScaling;

	std::vector<QImage*> icons;
	QImage* iconProxy;

	struct ScoreEntry
	{
		QString name;
		int score;

		bool operator < (const ScoreEntry& rhs) const
		{
			return score < rhs.score;
		}
	};

	std::vector<ScoreEntry> scoreTable;

public slots:	
};
