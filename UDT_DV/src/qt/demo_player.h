#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QColor>
#include <QTime>
#include "demo.hpp"

class DemoPlayer : public QObject
{
	Q_OBJECT

public:

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

	int serverTime;
	int elapsedTime;

	int demoStartTime;
	int demoEndTime;
	int demoLength;

	int gameStartTime;
	int gameLength;
	int gameStartElapsed;

#define SYNCBOOST 50
#define SYNCLOSS 15
#define SYNCMAX 100

	struct Entity
	{
		int syncCooldown;
		int index;

		Entity() : index(-1), syncCooldown(0){}
	};

	struct Player
	{
		int clientNum;
		bool demoTaker;
		QString name;
		QColor color;
		int health;
		int armor;
		int justDied;
		int team;
		int weapon;
		int ammo;
		int score;
		bool powerups[MAX_POWERUPS];
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

		int playerIndex;
		int TTL;
		Type::Id type;
		float startPosition[2];
		float endPosition[2];
	};

	Demo* demo;
	std::vector<Player> playerList;
	std::vector<Beam> beams;
	std::vector<Entity> entities;
	int warmupTime;
	float timescale;

	QTimer timer;
	QTime clock;

	DemoPlayer(QObject *parent);
	~DemoPlayer();

	bool loadDemo(QString path);
	void playDemo();
	void pauseDemo();
	void stopDemo();
	void update();
	void getDemoBoundingBox(int* origin, int* end);
	
private:	
	

	int lastIndex;
	int lastObituaryIndex;
	std::vector<bool> obituariesChecked;

	void searchPlayers();
	void makeBeams();

	int searchNextEntity(int serverTime, int oldIndex, int entityNumber, bool& sync);
	void updateEntityList(int startIndex, int time);
	void reset();
	void deSyncEntities();

	void msToTime(int _ms, int& h, int& m, int& s, int&ms);


public slots:
	void timerTick();
	
signals:
	void entitiesUpdated();
	void progress(float p);
	void demoFinished();
	
};
