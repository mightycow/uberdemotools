#pragma once


#include "demo.hpp"
#include "demo_player.h"

#include <QWidget>
#include <QPaintEvent>


class PaintWidget : public QWidget
{
	Q_OBJECT

protected:
	struct PlayerData
	{
		Demo::PlayerInfo* DemoPlayer;
		Player* PlayerInfo;
		float Alpha;
		bool Dead;

		PlayerData() : DemoPlayer(NULL), PlayerInfo(NULL), Alpha(1.0f), Dead(false) {}

		bool operator <(const PlayerData& rhs) const
		{
			return DemoPlayer->Position[2] < rhs.DemoPlayer->Position[2];
		}
	};

public:
	PaintWidget(QWidget *parent);
	~PaintWidget();

	void	ReleaseImage();
	bool	LoadMapImage(const QString& imagePath);
	void	LoadIcons(const QString& dirPath, const QStringList& iconPaths);
	void	LoadItems(const QString& dirPath, const QStringList& iconPaths);
	void	LoadWeapons(const QString& dirPath, const QStringList& weaponsPath);
	void	ResetScaling();
	void	SetScaling(int* origin, int* end);
	void	ComputeRenderScale();

protected:
	// Qt overrides.
	void	paintEvent(QPaintEvent* event);
	void	resizeEvent(QResizeEvent* event);


	void	PaintDemo(QPainter& painter);
	void	DrawClock(QPainter& painter);
	void	DrawHud(QPainter& painter);
	void	DrawHudElement(QPainter& painter, QImage* icon, int offsetX, const QString& text);
	void	DrawScores(QPainter& painter);
	void	DrawPowerUps(QPainter& painter);
	void	DrawTeamMateInfo(QPainter& painter);
	void	DrawPlayer(QPainter& painter, const PlayerData& data);
	void	DrawLivingPlayer(QPainter& painter, int x, int y, int z, const QColor& color, float alpha);
	void	DrawDeadPlayer(QPainter& painter, int x, int y, const QColor& color, float alpha);
	void	DrawPlayerPowerup(QPainter& painter, int x, int y, int z, const Player* player);
	void	DrawPlayerPowerupDisk(QPainter& painter, int x, int y, int z, int r, const QColor& color1, const QColor& color2);
	void	DrawPlayerPowerupImage(QPainter& painter, int x, int y, int z, QImage* image);
	void	DrawWeapon(QPainter& painter, int x, int y, int z, float angle, float alpha, int weapon, bool firing);
	void	DrawItem(QPainter& painter, const Demo::EntityInfo* info, float alpha);
	void	DrawProjectile(QPainter& painter, const Demo::EntityInfo* info);
	void	DrawGeneric(QPainter& painter, const Demo::EntityInfo* info);
	void	DrawBeams(QPainter& painter, const float* startPositions, const float* endPositions, Beam::Type::Id type, float alpha = 1.0f);
	void	DrawViewAngle(QPainter& painter, const QPoint& center, const QColor& color, float orientation, float angle, float radius, float alpha);
	void	SetImageAlpha(QImage* image, float alpha);
	QImage* GetIcon(const QString& fileName);
	QImage* GetItem(int type, bool respectProtocol = true);
	QImage* GetWeapon(int type, bool firing);
	void	GetUnscaledRect(QRect& rect);
	void	GetScaledRect(QRect& rect);

public:
	SharedDemoData* DemoData;
	float RenderScale;
	bool AdaptRenderScaleToWindowSize;
	bool DisplayDemo;
	bool ShowClock;
	bool ShowScore;
	bool ShowHud;
	bool ShowPowerUps;
	bool ShowTeamMateInfo;
	QString BackgroundMessage;

private:
	struct ItemInfo
	{
		QImage* Image;
		int Index;
	};

	struct IconInfo
	{
		QImage* Image;
		QString FileName;
	};

	QImage* _bgImage;
	int _mapOrigin[3];
	int _mapEnd[3];
	float _coordsScale;
	float _heightScale;
	float _iconScale;
	int _lastValidWeapon;
	std::vector<IconInfo> _icons;
	std::vector<ItemInfo> _items;
	std::vector<QImage*> _weapons;
	QImage* _proxyImage;
};
