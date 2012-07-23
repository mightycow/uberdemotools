#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QColor>
#include <QTime>
#include "demo.hpp"


#define UDT_DV_SYNC_BOOST	 50
#define UDT_DV_SYNC_LOSS	 15
#define UDT_DV_SYNC_MAX		100


struct Entity
{
	Entity() : Index(-1), SyncCoolDown(0) {}

	int SyncCoolDown; // Bumped up when back in sync. When <= 0, it means out of sync.
	int Index;
};

struct Player
{
	QString Name;
	QColor Color;
	int Health;
	int Armor;
	int JustDied;
	int Team;
	int Weapon;
	int Ammo;
	int Score;
	int ClientIndex;
	bool DemoTaker;
	bool Powerups[MAX_POWERUPS];
};

struct Beam
{
	struct Type
	{
		enum Id
		{
			LG,
			RG
		};
	};

	int PlayerIndex;
	int TimeToLive;
	Type::Id TypeId;
	float StartPositions[2];
	float EndPositions[2];
};

struct ScoreEntry
{
	QString Name;
	int Score;

	// For std::sort.
	bool operator <(const ScoreEntry& rhs) const
	{
		return Score < rhs.Score;
	}
};

struct SharedDemoData
{
	SharedDemoData()
	{
		Demo = NULL;
		Entities.resize(576);
	}

	~SharedDemoData()
	{
		if(Demo == NULL)
		{
			return;
		}

		delete Demo; 
		Demo = NULL;
	}

	std::vector<Player> Players;
	std::vector<Beam> Beams;
	std::vector<Entity> Entities;
	std::vector<ScoreEntry> Scores;
	QTime Clock;
	Demo* Demo;
	int WarmupTime;
};


class DemoPlayer : public QObject
{
	Q_OBJECT

public:
	DemoPlayer(QObject *parent);
	~DemoPlayer();

	bool	LoadDemo(const QString& path);
	void	PlayDemo();
	void	PauseDemo();
	void	StopDemo();
	void	Update();
	void	GetDemoBoundingBox(int* origin, int* end);

public slots:
	void	TimerTick();
	void	SearchPlayers();
	void	MakeBeams();
	int		SearchNextEntity(int serverTime, int oldIndex, int entityNumber, bool& sync);
	void	UpdateEntityList(int startIndex, int time);
	void	Reset();
	void	DeSyncEntities();

signals:
	void	EntitiesUpdated();
	void	Progress(float progress);
	void	DemoFinished();

public:
	// Shared with PaintWidget.
	SharedDemoData DemoData;

	// Shared with Gui.
	int _elapsedTime;
	int _gameLength;
	int _gameStartElapsed;
	float _timeScale;
	QTimer _timer;

private:	
	int _demoStartTime;
	int _demoEndTime;
	int _demoLength;
	int _gameStartTime;
	int _lastIndex;
	int _lastObituaryIndex;
	std::vector<bool> _obituariesChecked;
};
