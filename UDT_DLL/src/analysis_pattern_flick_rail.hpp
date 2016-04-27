#pragma once


#include "analysis_pattern_base.hpp"


struct udtFlickRailPatternAnalyzer : public udtPatternSearchAnalyzerBase
{
public:
	udtFlickRailPatternAnalyzer();
	~udtFlickRailPatternAnalyzer();

	void StartAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtFlickRailPatternAnalyzer);

	struct Constants
	{
		enum Id
		{
			TrackedSnapshots = 8,
			SnapshotMask = TrackedSnapshots - 1
		};
	};

	struct SnapshotInfo
	{
		f32 Angles[3];
		s32 ServerTimeMs;
		s32 TelePortBit; // If it's different from the previous snapshot's, we just teleported.
	};

	struct PlayerInfo
	{
		bool IsValid() const
		{ 
			if(SnapshotsRead < (u32)Constants::TrackedSnapshots)
			{
				return false;
			}

			s32 serverTimes[Constants::TrackedSnapshots];
			for(u32 i = 0; i < (u32)Constants::TrackedSnapshots; ++i)
			{
				serverTimes[i] = GetMostRecentSnapshot(i).ServerTimeMs;
			}

			const s32 firstDiff = serverTimes[0] - serverTimes[1];
			for(u32 i = 1; i < (u32)Constants::TrackedSnapshots - 1; ++i)
			{
				const s32 diff = serverTimes[i] - serverTimes[i + 1];
				if(diff != firstDiff)
				{
					return false;
				}
			}

			return true;
		}

		SnapshotInfo& GetWriteSnapshot() { return Snapshots[SnapshotIndex]; }

		// 0=most recent, 1=second most recent, etc.
		SnapshotInfo& GetMostRecentSnapshot(u32 offset = 0) { return Snapshots[(SnapshotIndex + Constants::TrackedSnapshots - 1 - offset) & Constants::SnapshotMask]; }
		const SnapshotInfo& GetMostRecentSnapshot(u32 offset = 0) const { return Snapshots[(SnapshotIndex + Constants::TrackedSnapshots - 1 - offset) & Constants::SnapshotMask]; }

		void IncrementIndex() { SnapshotIndex = (SnapshotIndex + 1) & Constants::SnapshotMask; ++SnapshotsRead; }

	private:
		SnapshotInfo Snapshots[Constants::TrackedSnapshots];
		u32 SnapshotIndex; // Where to write to next.
		u32 SnapshotsRead;
	};

	PlayerInfo _players[64];
	s32 _gameStateIndex;
};
