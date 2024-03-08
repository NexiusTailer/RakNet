/// \file FileListNodeContext.h
///
/// This file is part of RakNet Copyright 2003 Jenkins Software LLC
///
/// Usage of RakNet is subject to the appropriate license agreement.

#ifndef __FILE_LIST_NODE_CONTEXT_H
#define __FILE_LIST_NODE_CONTEXT_H

#include "BitStream.h"

struct FileListNodeContext
{
	FileListNodeContext() {dataPtr=0; dataLength=0;}
	FileListNodeContext(unsigned char o, unsigned int f) : op(o), flnc_extraData(f) {dataPtr=0; dataLength=0;}
	~FileListNodeContext() {}

	unsigned char op;
	uint32_t flnc_extraData;
	void *dataPtr;
	unsigned int dataLength;
};

inline RakNet::BitStream& operator<<(RakNet::BitStream& out, FileListNodeContext& in)
{
	out.Write(in.op);
	out.Write(in.flnc_extraData);
	return out;
}
inline RakNet::BitStream& operator>>(RakNet::BitStream& in, FileListNodeContext& out)
{
	in.Read(out.op);
	bool success = in.Read(out.flnc_extraData);
	(void) success;
	assert(success);
	return in;
}

#endif
