/// \file DS_ThreadsafeAllocatingQueue.h
/// \internal
/// Returns pointers to objects of the desired type, in a queue

#ifndef __THREADSAFE_ALLOCATING_QUEUE
#define __THREADSAFE_ALLOCATING_QUEUE

#include "DS_Queue.h"
#include "SimpleMutex.h"
#include "DS_MemoryPool.h"

#if defined(new)
#pragma push_macro("new")
#undef new
#define RMO_NEW_UNDEF_ALLOCATING_QUEUE
#endif

namespace DataStructures
{

template <class structureType>
class RAK_DLL_EXPORT ThreadsafeAllocatingQueue
{
public:
	void Push(structureType *s);
	structureType *PopInaccurate(void);
	structureType *Pop(void);
	structureType *Allocate(void);
	void Deallocate(structureType *s);
	void Clear(void);
	void SetPageSize(int size);
	
protected:
	MemoryPool<structureType> memoryPool;
	SimpleMutex memoryPoolMutex;
	Queue<structureType*> queue;
	SimpleMutex queueMutex;
};
	
template <class structureType>
void ThreadsafeAllocatingQueue<structureType>::Push(structureType *s)
{
	queueMutex.Lock();
	queue.Push(s);
	queueMutex.Unlock();
}

template <class structureType>
structureType *ThreadsafeAllocatingQueue<structureType>::PopInaccurate(void)
{
	structureType *s;
	if (queue.IsEmpty())
		return 0;
	queueMutex.Lock();
	if (queue.IsEmpty()==false)
		s=queue.Pop();
	else
		s=0;
	queueMutex.Unlock();	
	return s;
}

template <class structureType>
structureType *ThreadsafeAllocatingQueue<structureType>::Pop(void)
{
	structureType *s;
	queueMutex.Lock();
	if (queue.IsEmpty())
	{
		queueMutex.Unlock();	
		return 0;
	}
	s=queue.Pop();
	queueMutex.Unlock();	
	return s;
}

template <class structureType>
structureType *ThreadsafeAllocatingQueue<structureType>::Allocate(void)
{
	structureType *s;
	memoryPoolMutex.Lock();
	s=memoryPool.Allocate();
	memoryPoolMutex.Unlock();
	// Call new operator, memoryPool doesn't do this
	s = new ((void*)s) structureType;
	return s;
}

template <class structureType>
void ThreadsafeAllocatingQueue<structureType>::Deallocate(structureType *s)
{
	// Call delete operator, memory pool doesn't do this
	s->~structureType();
	memoryPoolMutex.Lock();
	memoryPool.Release(s);
	memoryPoolMutex.Unlock();
}

template <class structureType>
void ThreadsafeAllocatingQueue<structureType>::Clear(void)
{
	memoryPoolMutex.Lock();
	for (unsigned int i=0; i < queue.Size(); i++)
	{
		queue[i]->~structureType();
		memoryPool.Release(queue[i]);
	}
	queue.Clear();
	memoryPoolMutex.Unlock();
	memoryPoolMutex.Lock();
	memoryPool.Clear();
	memoryPoolMutex.Unlock();
}

template <class structureType>
void ThreadsafeAllocatingQueue<structureType>::SetPageSize(int size)
{
	memoryPool.SetPageSize(size);
}

};


#if defined(RMO_NEW_UNDEF_ALLOCATING_QUEUE)
#pragma pop_macro("new")
#undef RMO_NEW_UNDEF_ALLOCATING_QUEUE
#endif

#endif
