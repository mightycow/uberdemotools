#pragma once


#include "analysis_cut_by_pattern.hpp"


struct udtCutByFlickRailAnalyzer : public udtCutByPatternAnalyzerBase
{
public:
	udtCutByFlickRailAnalyzer();
	~udtCutByFlickRailAnalyzer();

	void StartAnalysis() override;
	void ProcessGamestateMessage(const udtGamestateCallbackArg& arg, udtBaseParser& parser) override;
	void ProcessSnapshotMessage(const udtSnapshotCallbackArg& arg, udtBaseParser& parser) override;

private:
	UDT_NO_COPY_SEMANTICS(udtCutByFlickRailAnalyzer);

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
	};

	struct PlayerInfo
	{
		SnapshotInfo& GetWriteSnapshot() { return Snapshots[SnapshotIndex]; }

		// 0=most recent, 1=second most recent, etc.
		SnapshotInfo& GetMostRecentSnapshot(u32 offset = 0) { return Snapshots[(SnapshotIndex + Constants::TrackedSnapshots - 1 - offset) & Constants::SnapshotMask]; }

		void IncrementIndex() { SnapshotIndex = (SnapshotIndex + 1) & Constants::SnapshotMask; }

		SnapshotInfo Snapshots[Constants::TrackedSnapshots];
		u32 SnapshotIndex; // Where to write to next.
	};

	PlayerInfo _players[64];
	s32 _gameStateIndex;
};
