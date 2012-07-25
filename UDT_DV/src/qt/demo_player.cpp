#include "demo_player.h"
#include "api.h"
#include "demo73.hpp"

#include <set>
#include <qDebug>
#include <math.h>
#include <limits>


static void GetTimeFromMs(int totalMs, int& hours, int& minutes, int& seconds, int& milliSeconds)
{
	milliSeconds = totalMs % 1000;
	totalMs -= milliSeconds;
	totalMs /= 1000;

	seconds = totalMs % 60;
	totalMs -= seconds;
	totalMs /= 60;

	minutes = totalMs % 60;
	totalMs -= minutes;
	totalMs /= 60;

	hours = totalMs % 24;
	totalMs -= hours;
	totalMs /= 24;
}


DemoPlayer::DemoPlayer(QObject *parent)
	: QObject(parent)
{
	_timer.setInterval(25);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(TimerTick()));
	_timeScale = 1.2f;
	_lastIndex = -1;
}

DemoPlayer::~DemoPlayer()
{
}

bool DemoPlayer::LoadDemo(const QString& path)
{
	if(DemoData.Demo != NULL)
	{
		delete DemoData.Demo; 
		DemoData.Demo = NULL;
	}

	Demo* const demo = new Demo73;
	demo->_inFilePath = path.toStdString().c_str();
	demo->_protocol = Protocol::Dm73;
	if(!demo->Do())
	{
		return false;
	}

	DemoData.Demo = demo;
	
	Reset();

	DemoData.Clock = QTime(0, 0, 0, 0);
	_demoStartTime = demo->_entityPlaybackInfos[0].Time;
	_demoEndTime = demo->_entityPlaybackInfos.back().Time;
	_demoLength = _demoEndTime - _demoStartTime;

	_gameStartTime = demo->_gameStartTime;
	if(_gameStartTime == -1)
	{
		_gameStartTime = _demoStartTime;
	}
	_gameStartElapsed = _gameStartTime - _demoStartTime;
	_gameLength = _demoLength - _gameStartElapsed;

	// Start directly with the game.
	DemoData.WarmupTime = 0;
	_elapsedTime = _gameStartElapsed;

	_lastObituaryIndex = -1;
	_obituariesChecked.resize(demo->_obituaries.size(), false);

	SearchPlayers();
	MakeBeams();

	return true;
}

void DemoPlayer::TimerTick()
{

	if(_timeScale < 0)
		int asd = 0;
	// Update elapsed time.
	_elapsedTime += _timer.interval() * _timeScale;

	if(_elapsedTime > _gameLength + _gameStartElapsed)
	{
		_elapsedTime = _gameLength + _gameStartElapsed;
		emit(DemoFinished());
	}
	
	if(_elapsedTime < _gameStartElapsed)
	{
		_elapsedTime = _gameStartElapsed;
	}

	Update();
}

void DemoPlayer::Update()
{
	DeSyncEntities();

	UpdateEntityList(_lastIndex, _demoStartTime + _elapsedTime);

	emit(EntitiesUpdated());

	const QTime time(0, 0);
	DemoData.Clock = time.addMSecs(_elapsedTime - (_gameStartTime - _demoStartTime));

	if(_elapsedTime >= _gameStartElapsed)
	{
		emit(Progress((_elapsedTime - _gameStartElapsed) / (float)_gameLength));
	}
}

void DemoPlayer::PlayDemo()
{
	if(DemoData.Demo != NULL)
	{
		_timer.start();
	}
}

void DemoPlayer::PauseDemo()
{
	_timer.stop();
}

void DemoPlayer::StopDemo()
{
	_timer.stop();
	DemoData.Clock = QTime(0, 0, 0, 0);
	_elapsedTime = 0;
	DemoData.WarmupTime = _demoStartTime - _gameStartTime;
	_lastIndex = -1;
	for(size_t i = 0; i < _obituariesChecked.size(); ++i)
	{
		_obituariesChecked[i] = false;
	}
	emit(Progress(0));
}

int DemoPlayer::SearchNextEntity(int serverTime, int oldIndex, int entityNumber, bool& sync)
{
	Demo* const demo = DemoData.Demo;
	int latestIndex = oldIndex;

	sync = false;
	for(size_t i = oldIndex + 1; i < demo->_entityPlaybackInfos.size(); ++i)
	{
		if(demo->_entityPlaybackInfos[i].Number != entityNumber)
		{
			continue;
		}

		const int time = demo->_entityPlaybackInfos[i].Time;
		if(time > serverTime)
		{
			break;
		}

		latestIndex = i;
		sync = true;
	}

	return latestIndex;
}

void DemoPlayer::SearchPlayers()
{
	std::vector<Player>& players = DemoData.Players;
	players.clear();

	Demo* const demo = DemoData.Demo;
	if(demo == NULL)
	{
		return;
	}

	for(size_t i = 0; i < 64; ++i)
	{
		if(!demo->_players[i].Valid)
		{
			continue;
		}

		Player p;
		Demo::PlayerInfo& info = demo->_players[i].Info;

		p.DemoTaker = false;
		p.Health = -1;
		p.Armor = -1;
		p.ClientIndex = info.Player;
		p.Team = info.Team;
		p.Score = 0;
		p.Color = QColor(0, 0, 0, 0);
		p.JustDied = -1;
		for(int q = 0; q < MAX_POWERUPS; ++q)
		{
			p.Powerups[q] = false;
		}

		players.push_back(p);
	}
}

void DemoPlayer::MakeBeams()
{
	const std::vector<Player>& players = DemoData.Players;
	std::vector<Beam>& beams = DemoData.Beams;

	const size_t playerCount = players.size();
	beams.clear();
	beams.resize(2 * playerCount);

	for(size_t i = 0; i < playerCount; ++i)
	{
		Beam beam;
		beam.PlayerIndex = i;
		beam.TimeToLive = -1;

		// LG
		beam.TypeId = Beam::Type::LG;		
		beams.push_back(beam);

		// RG
		beam.TypeId = Beam::Type::RG;
		beams.push_back(beam);
	}
}


void DemoPlayer::UpdateEntityList(int startIndex, int time)
{
	Demo* const demo = DemoData.Demo;

	// Get the closest server time.
	int serverTime = -1;
	for(size_t i = std::max(0, startIndex); i < demo->_playerPlaybackInfos.size(); ++i)
	{
		if(serverTime > time)
		{
			break;
		}

		serverTime = demo->_playerPlaybackInfos[i].Time;
	}

	std::vector<Entity>& entities = DemoData.Entities;
	std::vector<Player>& players = DemoData.Players;
	std::vector<Beam>& beams = DemoData.Beams;

	// Update players.
	for(size_t i = 0; i < demo->_playerPlaybackInfos.size(); ++i)
	{
		const Demo::PlayerInfo& info = demo->_playerPlaybackInfos[i];
		if(info.Time != serverTime)
		{
			continue;
		}

		// Boost player entity sync level.
		const int number = 2 * info.Player + (info.CurrentWeapon == 0 ? 1 : 0);
		Entity& entity = entities[number];
		entity.Index = i;
		entity.SyncCoolDown += UDT_DV_SYNC_BOOST;
		entity.SyncCoolDown = std::min(entity.SyncCoolDown, UDT_DV_SYNC_MAX);
			
		// Update player stats.
		int demoTakerIndex = -1;
		int playerIndex = -1;
		for(size_t playerIdx = 0; playerIdx < players.size(); ++playerIdx)
		{
			const Player& player = players[playerIdx];
			if(player.ClientIndex == info.Player && player.Team != TEAM_SPECTATOR)
			{
				playerIndex = playerIdx;
			}
			if(player.DemoTaker)
			{
				demoTakerIndex = playerIdx;
			}
		}
			
		if(playerIndex == -1)
		{
			continue;
		}

		Player& player = players[playerIndex];

		// Update stats.
		player.Health = info.Health;
		player.Armor = info.Armor;
		player.Ammo = info.CurrentAmmo;
		player.DemoTaker = info.DemoTaker;
		player.Weapon = info.CurrentWeapon;
		player.Score = info.Score;

		// Player power-ups.
		for(size_t puIdx = 0; puIdx < MAX_POWERUPS; ++puIdx)
		{
			player.Powerups[puIdx] = !!((info.Powerups >> puIdx) & 1);
		}
			
		// Update color.
		switch(player.Team)
		{
		case TEAM_FREE:
			player.Color = info.DemoTaker ? QColor(220, 255, 220, 255) : QColor(50, 255, 50, 255);
			break;
		case TEAM_RED:
			player.Color = info.DemoTaker ? QColor(255, 150, 150, 255) : QColor(255, 50, 50, 255);
			break;
		case TEAM_BLUE:
			player.Color = info.DemoTaker ? QColor(150, 150, 255, 255) : QColor(50, 50, 255, 255);
			break;
		case TEAM_SPECTATOR:
			player.Color = QColor(0, 0, 0, 0);
			break;
		default:
			if(demoTakerIndex == -1) 
				break;
			if(players[demoTakerIndex].Team == TEAM_RED)
				player.Color = QColor(50, 50, 255, 255);
			else if (players[demoTakerIndex].Team == TEAM_BLUE)
				player.Color = QColor(255, 50, 50, 255);
			break;
		}

		// Update rail trails.
		if(info.BeamType == Demo::BeamType::RailTrail)
		{
			Beam& beam = beams[2 * playerIndex + 1];
			if(beam.TimeToLive < -1000)
			{
				beam.TypeId = Beam::Type::RG;
				beam.TimeToLive = 500;
				beam.StartPositions[0] = info.Position[0];
				beam.StartPositions[1] = info.Position[1];
				beam.EndPositions[0] = info.Position[0] + cos(info.Angles[1] / 180 * 3.1415) * 3000;
				beam.EndPositions[1] = info.Position[1] + sin(info.Angles[1] / 180 * 3.1415) * 3000;
			}
		}
			
		// Player name.
		Demo::PlayerNamePlaybackInfoVector& playerNames = demo->_playerNamesPlaybackInfos[info.Player];
		Demo::PlayerNameInfo* nameInfo = NULL;
		if(playerNames.size() > 0)
		{
			nameInfo = &playerNames[0];
		}
			
		// Find the most up-to-date player name.
		for(size_t i = 0; i < playerNames.size(); ++i)
		{
			if(playerNames[i].Time > time)
			{
				break;
			}
			nameInfo = &playerNames[i];
		}
		player.Name = nameInfo != NULL ? nameInfo->Name : "?";
	}
	
	// Update scores.
	for(size_t i = 0; i < demo->_scorePlaybackInfos.size(); ++i)
	{
		const Demo::ScoreInfo& scoreInfo = demo->_scorePlaybackInfos[i];
		if(serverTime != scoreInfo.Time)
		{
			continue;
		}

		int demoTakerScore = -9999;
		for(size_t j = 0; j < players.size(); ++j)
		{
			if(players[j].DemoTaker)
			{
				demoTakerScore = players[j].Score;
				break;
			}
		}

		// Fix the score for all other players.
		const int otherScore = scoreInfo.Score1 == demoTakerScore ? scoreInfo.Score2 : scoreInfo.Score1;
		for(size_t j = 0; j < players.size(); ++j)
		{
			if(!players[j].DemoTaker)
			{
				players[j].Score = otherScore;
			}
		}
	}
	
	// Update lg beams.
	for(size_t i = 0; i < demo->_beamPlaybackInfos.size(); ++i)
	{
		const Demo::BeamInfo& info = demo->_beamPlaybackInfos[i];
		if(info.Time != serverTime)
		{
			continue;
		}

		const int ClientIdx = info.ClientNum;

		// Update player stats.
		int playerIndex = -1;
		for(size_t p = 0; p < players.size(); p++)
		{
			if(players[p].ClientIndex == ClientIdx && players[p].Team != TEAM_SPECTATOR)
			{
				playerIndex = p;
				break;
			}
		}

		if(playerIndex == -1)
		{
			continue;
		}

		Beam& beam = beams[2 * playerIndex + 0];
		beam.TypeId = Beam::Type::LG;
		beam.TimeToLive = 50;
		beam.StartPositions[0] = info.StartPosition[0];
		beam.StartPositions[1] = info.StartPosition[1];
		beam.EndPositions[0] = info.EndPosition[0];
		beam.EndPositions[1] = info.EndPosition[1];
	}

	// Update entities.
	for(size_t i = 0; i < demo->_entityPlaybackInfos.size(); ++i)
	{
		const Demo::EntityInfo& info = demo->_entityPlaybackInfos[i];
		if(info.Time != serverTime)
		{
			continue;
		}

		const int number = info.Number + 128;
		if(number >= (int)entities.size())
		{
			continue;
		}

		Entity& entity = entities[number];
		entity.Index = i;
		entity.SyncCoolDown += UDT_DV_SYNC_BOOST;
		entity.SyncCoolDown = std::min(entity.SyncCoolDown, UDT_DV_SYNC_MAX);
	}

	//
	// Update scores.
	//
	std::vector<ScoreEntry>& scoreTable = DemoData.Scores;
	scoreTable.clear();
	if(Demo::GameType::IsTeamMode((Demo::GameType::Id)demo->_gameType))
	{
		const int currentTime = _demoStartTime + _elapsedTime;
		const Demo::ScoreInfo* scoreInfo = NULL;
		for(size_t i = 0; i < demo->_scorePlaybackInfos.size(); ++i)
		{
			const Demo::ScoreInfo* info = &demo->_scorePlaybackInfos[i];
			if(info->Time > currentTime)
			{
				break;
			}

			scoreInfo = info;
		}

		int scoreRed = 0;
		int scoreBlue = 0;
		if(scoreInfo != NULL)
		{
			scoreRed = scoreInfo->Score1;
			scoreBlue = scoreInfo->Score2;
		}

		ScoreEntry e;
		e.Name = "RED";
		e.Score = scoreRed;
		scoreTable.push_back(e);

		e.Name = "BLUE";
		e.Score = scoreBlue;
		scoreTable.push_back(e);
	}
	else
	{
		for(size_t i = 0; i < players.size(); ++i)
		{
			if(players.at(i).Team == TEAM_SPECTATOR)
			{
				continue;
			}

			ScoreEntry e;
			e.Name = players[i].Name;
			e.Score = players[i].Score;
			scoreTable.push_back(e);
		}
	}

	if(scoreTable.size() > 2)
	{
		std::sort(scoreTable.begin(), scoreTable.end());
		std::reverse(scoreTable.begin(), scoreTable.end());
	}
}

void DemoPlayer::DeSyncEntities()
{
	std::vector<Entity>& entities = DemoData.Entities;
	std::vector<Beam>& beams = DemoData.Beams;

	// Reduce sync level of the items.
	for(size_t i = 0; i < entities.size(); ++i)
	{
		Entity& entity = entities[i];
		entity.SyncCoolDown -= UDT_DV_SYNC_LOSS;
		entity.SyncCoolDown = std::max(entity.SyncCoolDown, 0);
	}

	// Reduce time to live of the beams.
	const int timeDelta = (int)abs((float)_timer.interval() * _timeScale);
	for(size_t i = 0; i < beams.size(); ++i)
	{
		beams[i].TimeToLive -= timeDelta;
	}
}

void DemoPlayer::Reset()
{
	for(size_t i = 0; i < DemoData.Entities.size(); ++i)
	{
		DemoData.Entities[i].SyncCoolDown = 0;
	}
}

void DemoPlayer::GetDemoBoundingBox(int* origin, int* end)
{
	Demo* const demo = DemoData.Demo;
	if(demo == NULL)
	{
		return;
	}

	origin[0] = 9999;
	origin[1] = -9999;
	origin[2] = 9999;

	end[0] = -9999;
	end[1] = 9999;
	end[2] = -9999;

	const int Margin = 400;

	for(size_t i = 0; i < demo->_playerPlaybackInfos.size(); i++)
	{
		float x = demo->_playerPlaybackInfos[i].Position[0];
		float y = demo->_playerPlaybackInfos[i].Position[1];
		float z = demo->_playerPlaybackInfos[i].Position[2];

		if(x < origin[0]) origin[0] = x - Margin; if(x > end[0]   ) end[0]    = x + Margin;
		if(y < end[1]   ) end[1]    = y - Margin; if(y > origin[1]) origin[1] = x + Margin;
		if(z < origin[2]) origin[2] = z - Margin; if(z > end[2]   ) end[2]    = x + Margin;
	}

	for(size_t i = 0; i < demo->_entityPlaybackInfos.size(); i++)
	{
		float x = demo->_entityPlaybackInfos[i].Position[0];
		float y = demo->_entityPlaybackInfos[i].Position[1];
		float z = demo->_entityPlaybackInfos[i].Position[2];

		if(x < origin[0]) origin[0] = x - Margin; if(x > end[0]   ) end[0]    = x + Margin;
		if(y < end[1]   ) end[1]    = y - Margin; if(y > origin[1]) origin[1] = x + Margin;
		if(z < origin[2]) origin[2] = z - Margin; if(z > end[2]   ) end[2]    = x + Margin;
	}
}
