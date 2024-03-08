#ifndef __TRANFORMATION_HISTORY_H
#define __TRANFORMATION_HISTORY_H

#include "RakNetTypes.h"
#include "OgreVector3.h"
#include "OgreQuaternion.h"
#include "DS_Queue.h"
#include "RakMemoryOverride.h"

struct TransformationHistoryCell
{
	TransformationHistoryCell();
	TransformationHistoryCell(RakNet::TimeMS t, const Ogre::Vector3& pos, const Ogre::Vector3& vel, const Ogre::Quaternion& quat  );

	RakNet::TimeMS time;
	Ogre::Vector3 position;
	Ogre::Quaternion orientation;
	Ogre::Vector3 velocity;
};

class TransformationHistory
{
public:
	void Init(RakNet::TimeMS maxWriteInterval, RakNet::TimeMS maxHistoryTime);
	void Write(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNet::TimeMS curTimeMS);
	void Overwrite(const Ogre::Vector3 &position, const Ogre::Vector3 &velocity, const Ogre::Quaternion &orientation, RakNet::TimeMS when);
	enum ReadResult
	{
		// We are reading so far in the past there is no data yet
		READ_OLDEST,
		// We are not reading in the past, so the input parameters stay the same
		VALUES_UNCHANGED,
		// We are reading in the past
		INTERPOLATED
	};
	// Parameters are in/out, modified to reflect the history
	ReadResult Read(Ogre::Vector3 *position, Ogre::Vector3 *velocity, Ogre::Quaternion *orientation,
		RakNet::TimeMS when, RakNet::TimeMS curTime);
	void Clear(void);
protected:
	DataStructures::Queue<TransformationHistoryCell> history;
	unsigned maxHistoryLength;
	RakNet::TimeMS writeInterval;
};

#endif
