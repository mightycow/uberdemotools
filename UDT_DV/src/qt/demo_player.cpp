#include "demo_player.h"
#include "api.h"
#include "demo73.hpp"
#include <set>
#include <qDebug>
#include <math.h>
#include <limits>


DemoPlayer::DemoPlayer(QObject *parent)
	: QObject(parent)
{
	demo = NULL;
	timer.setInterval(25);
	connect(&timer, SIGNAL(timeout()), this, SLOT(timerTick()));
	timescale = 1.2f;
	lastIndex = -1;
	entities.resize(576);
}

DemoPlayer::~DemoPlayer()
{
	if(demo != NULL)
	{
		delete demo; 
		demo = NULL;
	}
}

bool DemoPlayer::loadDemo( QString path )
{
	if(demo != NULL)
	{
		delete demo; 
		demo = NULL;
	}
	demo = new Demo73;
	demo->_inFilePath = path.toStdString().c_str();
	demo->_protocol = Protocol::Dm73;

	if(demo->Do())
	{
		reset();

		clock = QTime(0, 0, 0, 0);
		demoStartTime = demo->_entityPlaybackInfos[0].Time;
		demoEndTime = demo->_entityPlaybackInfos.back().Time;
		demoLength = demoEndTime - demoStartTime;

		gameStartTime = demo->_gameStartTime;
		if(gameStartTime == -1)
			gameStartTime = demoStartTime;
		gameStartElapsed = gameStartTime - demoStartTime;
		gameLength = demoLength - gameStartElapsed;

		// Start directly with the game
		warmupTime = 0;
		elapsedTime = gameStartElapsed;

		lastObituaryIndex = -1;
		obituariesChecked.resize(demo->_obituaries.size(), false);

		searchPlayers();
		makeBeams();
		return true;
	}
	else
		return false;
}

void DemoPlayer::timerTick()
{
	// Update elapsed time
	elapsedTime += timer.interval() * timescale;

	if(elapsedTime > gameLength + gameStartElapsed)
	{
		elapsedTime = gameLength + gameStartElapsed;
		emit(demoFinished());
	}
	
	if(elapsedTime < gameStartElapsed)
		elapsedTime = gameStartElapsed;

	update();

}


void DemoPlayer::update()
{
	deSyncEntities();

	updateEntityList(lastIndex, demoStartTime + elapsedTime);

	emit(entitiesUpdated());

	//updateObituaries(demoStartTime + elapsedTime);

	QTime time(0, 0);
	clock = time.addMSecs(elapsedTime - (gameStartTime - demoStartTime));


	if(elapsedTime >= gameStartElapsed)
		emit(progress((elapsedTime - gameStartElapsed) / (float)gameLength));
}


void DemoPlayer::playDemo()
{
	if(demo != NULL)
	{
		timer.start();
	}
}

void DemoPlayer::pauseDemo()
{
	timer.stop();
}

void DemoPlayer::stopDemo()
{
	timer.stop();
	clock = QTime(0, 0, 0, 0);
	elapsedTime = 0;
	warmupTime = demoStartTime - gameStartTime;
	lastIndex = -1;
	for(size_t i = 0; i < obituariesChecked.size(); i++)
		obituariesChecked[i] = false;
	emit(progress(0));
}

int DemoPlayer::searchNextEntity(int serverTime, int oldIndex, int entityNumber, bool& sync)
{
	int best = oldIndex;
	sync = false;
	for(size_t i = oldIndex + 1; i < demo->_entityPlaybackInfos.size(); i++)
	{
		
		if(demo->_entityPlaybackInfos[i].Number == entityNumber)
		{
			int time = demo->_entityPlaybackInfos[i].Time;
			if(time <= serverTime)
			{
				best = i;
				sync = true;
			}
			else
				break;
		}
	}
	return best;

}

void DemoPlayer::searchPlayers()
{
	playerList.clear();
	if(!demo)
	{
		return;
	}

	for(size_t i = 0; i < 64; i++)
	{
		if(demo->_players[i].Valid)
		{
			Player p;
			Demo::PlayerInfo& info = demo->_players[i].Info;
			
			p.demoTaker = false;
			p.health = -1;
			p.armor = -1;
			p.clientNum = info.Player;
			p.team = info.Team;
			p.score = 0;
			p.color = QColor(0, 0, 0, 0);
			for(int q = 0; q < MAX_POWERUPS; q++)
				p.powerups[q] = false;
			p.justDied = -1;

			playerList.push_back(p);
		}
	}
}

void DemoPlayer::makeBeams()
{
	int playerCount = playerList.size();
	beams.clear();
	beams.resize(2 * playerCount);

	for(int a = 0; a < playerCount; a++)
	{
		Beam b;

		// LG
		b.playerIndex = a;
		b.TTL = -1;
		b.type = Beam::Type::LG;		
		beams.push_back(b);

		// RG
		b.type = Beam::Type::RG;
		beams.push_back(b);
	}
}


void DemoPlayer::updateEntityList(int startIndex, int time)
{
	// Get closest server time
	int serverTime = -1;
	for(size_t i = std::max(0, startIndex); i < demo->_playerPlaybackInfos.size(); i++)
	{
		if(serverTime > time)
		{
			break;
		}
		serverTime = demo->_playerPlaybackInfos[i].Time;
		//lastIndex = i;
	}

	// Update Players
	for(size_t i = 0; i < demo->_playerPlaybackInfos.size(); i++)
	{
		const Demo::PlayerInfo& info = demo->_playerPlaybackInfos[i];
		if(info.Time == serverTime)
		{
			// Boost player entity sync level
			int number = 2 * info.Player + (info.CurrentWeapon == 0 ? 1 : 0);
			entities[number].index = i;
			entities[number].syncCooldown += SYNCBOOST;
			if(entities[number].syncCooldown > SYNCMAX)
				entities[number].syncCooldown = SYNCMAX;
			
			// Update player stats
			int demoTakerIndex = -1;
			int playerIndex = -1;
			for(size_t p = 0; p < playerList.size(); p++)
			{
				// @TODO: Validate change.
				//if(playerList[p].clientNum == info.Player && playerList[p].name == QString::fromStdString(info.Name))
				if(playerList[p].clientNum == info.Player && playerList[p].team != TEAM_SPECTATOR)
				{
					playerIndex = p;
				}

				if(playerList[p].demoTaker)
				{
					demoTakerIndex = p;
				}
			}
			
			if(playerIndex == -1)
				continue;

			DemoPlayer::Player& player = playerList[playerIndex];

			// Update Stats
			player.health = info.Health;
			player.armor = info.Armor;
			player.ammo = info.CurrentAmmo;
			player.demoTaker = info.demoTaker;
			player.weapon = info.CurrentWeapon;
			player.score = info.Score;

			// Player Powerup
			for(size_t p = 0; p < MAX_POWERUPS; p++)
			{
				player.powerups[p] = (info.Powerups >> p) & 0x1;
			}
			
			// Update Color
			switch(player.team)
			{
			case TEAM_FREE:
				if(info.demoTaker)
					player.color = QColor(255, 255, 255, 255);
				else
					player.color = QColor(50, 255, 50, 255);
				break;
			case TEAM_RED:
				if(info.demoTaker)
					player.color = QColor(255, 150, 150, 255);
				else
					player.color = QColor(255, 50, 50, 255);
				break;
			case TEAM_BLUE:
				if(info.demoTaker)
					player.color = QColor(150, 150, 255, 255);
				else
					player.color = QColor(50, 50, 255, 255);
				break;
			case TEAM_SPECTATOR:
				player.color = QColor(0, 0, 0, 0);
				break;
			default:
				if(playerList[demoTakerIndex].team == TEAM_RED)
					player.color = QColor(50, 50, 255, 255);
				else if (playerList[demoTakerIndex].team == TEAM_BLUE)
					player.color = QColor(255, 50, 50, 255);
				break;
			}

			
			// Update Railtrails 
			if(info.BeamType == Demo::BeamType::RailTrail)
			{
				Beam& beam = beams[2 * playerIndex + 1];
				if(beam.TTL < -1000)
				{
					beam.type = Beam::Type::RG;
					beam.TTL = 500;
					beam.startPosition[0] = info.Position[0];
					beam.startPosition[1] = info.Position[1];
					beam.endPosition[0] = info.Position[0] + cos(info.Angles[1] / 180 * 3.1415) * 3000;
					beam.endPosition[1] = info.Position[1] + sin(info.Angles[1] / 180 * 3.1415) * 3000;
				}
			}

			
			// Player name
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
			player.name = nameInfo != NULL ? nameInfo->Name : "?";
		}
	}

	
	// Update Scores
	for(size_t i = 0; i < demo->_scorePlaybackInfos.size(); i++)
	{
		const Demo::ScoreInfo& scoreInfo = demo->_scorePlaybackInfos[i];
		if(serverTime == scoreInfo.Time)
		{
			int demoTakerScore = -9999;
			for(size_t j = 0; j < playerList.size(); j++)
			{
				if(playerList[j].demoTaker)
				{
					demoTakerScore = playerList[j].score;
					break;
				}
			}
			
			int otherScore = scoreInfo.Score1 == demoTakerScore ? scoreInfo.Score2 : scoreInfo.Score1;

			// Fix score for the other player
			for(size_t j = 0; j < playerList.size(); j++)
			{
				if(!playerList[j].demoTaker)
					playerList[j].score = otherScore;
			}
		}
	}
	
	// Update lg Beams
	for(size_t i = 0; i < demo->_beamPlaybackInfos.size(); i++)
	{
		const Demo::BeamInfo& info = demo->_beamPlaybackInfos[i];
		if(info.Time == serverTime)
		{
			int number = info.ClientNum;

			// Update player stats
			int playerIndex = -1;
			for(size_t p = 0; p < playerList.size(); p++)
			{
				if(playerList[p].clientNum == number && playerList[p].team != TEAM_SPECTATOR)
				{
					playerIndex = p;
					break;
				}
			}

			if(playerIndex == -1)
				continue;

			Beam& beam = beams[2 * playerIndex + 0];
			beam.type = Beam::Type::LG;
			beam.TTL = 50;
			beam.startPosition[0] = info.StartPosition[0];
			beam.startPosition[1] = info.StartPosition[1];
			beam.endPosition[0] = info.EndPosition[0];
			beam.endPosition[1] = info.EndPosition[1];
		}
	}

	
	// Update Entities
	for(size_t i = 0; i < demo->_entityPlaybackInfos.size(); i++)
	{
		const Demo::EntityInfo& info = demo->_entityPlaybackInfos[i];
		if(info.Time == serverTime)
		{
			const int number = info.Number + 128;
			if(number >= (int)entities.size())
			{
				continue;
			}

			DemoPlayer::Entity& entity = entities[number];
			entity.index = i;
			entity.syncCooldown += SYNCBOOST;
			entity.syncCooldown = std::min(entity.syncCooldown, SYNCMAX);
		}
	}

	//
	// Update scores
	//
	scoreTable.clear();
	if(Demo::GameType::IsTeamMode((Demo::GameType::Id)demo->_gameType))
	{
		const int currentTime = demoStartTime + elapsedTime;

		Demo::ScoreInfo* scoreInfo = NULL;
		for(size_t i = 0; i < demo->_scorePlaybackInfos.size(); ++i)
		{
			Demo::ScoreInfo* info = &demo->_scorePlaybackInfos[i];
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

		DemoPlayer::ScoreEntry e;
		e.name = "RED";
		e.score = scoreRed;
		scoreTable.push_back(e);

		e.name = "BLUE";
		e.score = scoreBlue;
		scoreTable.push_back(e);
	}
	else
	{
		for(size_t i = 0; i < playerList.size(); i++)
		{
			if(playerList.at(i).team == TEAM_SPECTATOR)
			{
				continue;
			}

			DemoPlayer::ScoreEntry e;
			e.name = playerList[i].name;
			e.score = playerList[i].score;
			scoreTable.push_back(e);
		}
	}

	if(scoreTable.size() > 2)
	{
		std::sort(scoreTable.begin(), scoreTable.end());
		std::reverse(scoreTable.begin(), scoreTable.end());
	}
}

void DemoPlayer::deSyncEntities()
{
	// Reduce sync level of the items
	for(size_t i = 0; i < entities.size(); i++)
	{
		entities[i].syncCooldown -= SYNCLOSS;
		if(entities[i].syncCooldown < 0)
			entities[i].syncCooldown = 0;
	}

	// Reduce time to live of the beams
	for(size_t i = 0; i < beams.size(); i++)
	{
		beams[i].TTL-= abs(timer.interval() * timescale);
	}
}

void DemoPlayer::msToTime(int _ms, int& h, int& m, int& s, int& ms )
{
	ms = _ms % 1000;
	_ms -= ms;
	_ms /= 1000;

	s = _ms % 60;
	_ms -= s;
	_ms /= 60;

	m = _ms % 60;
	_ms -= m;
	_ms /= 60;

	h = _ms % 24;
	_ms -= h;
	_ms /= 24;
}

void DemoPlayer::reset()
{
	for(size_t i = 0; i < entities.size(); i++)
	{
		entities[i].syncCooldown = 0;
	}
}

void DemoPlayer::getDemoBoundingBox( int* origin, int* end )
{
	if(demo)
	{

		origin[0] = 9999;
		origin[1] = -9999;
		origin[2] = 9999;

		end[0] = -9999;
		end[1] = 9999;
		end[2] = -9999;

		for(size_t i = 0; i < demo->_playerPlaybackInfos.size(); i++)
		{
			float x = demo->_playerPlaybackInfos[i].Position[0];
			float y = demo->_playerPlaybackInfos[i].Position[1];
			float z = demo->_playerPlaybackInfos[i].Position[2];

			if(x < origin[0]) origin[0] = x - 400; if(x > end[0]   ) end[0]    = x + 400;
			if(y < end[1]   ) end[1]    = y - 400; if(y > origin[1]) origin[1] = x + 400;
			if(z < origin[2]) origin[2] = z - 400; if(z > end[2]   ) end[2]    = x + 400;
		}

		for(size_t i = 0; i < demo->_entityPlaybackInfos.size(); i++)
		{
			float x = demo->_entityPlaybackInfos[i].Position[0];
			float y = demo->_entityPlaybackInfos[i].Position[1];
			float z = demo->_entityPlaybackInfos[i].Position[2];

			if(x < origin[0]) origin[0] = x - 400; if(x > end[0]   ) end[0]    = x + 400;
			if(y < end[1]   ) end[1]    = y - 400; if(y > origin[1]) origin[1] = x + 400;
			if(z < origin[2]) origin[2] = z - 400; if(z > end[2]   ) end[2]    = x + 400;
		}
	}
}
