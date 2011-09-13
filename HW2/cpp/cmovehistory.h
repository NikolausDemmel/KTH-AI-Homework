/*
 * cmovehistory.h
 *
 *  Created on: 13.09.2011
 *      Author: demmeln
 */

#ifndef CMOVEHISTORY_H_
#define CMOVEHISTORY_H_

#include <cstring>

namespace chk {

typedef uint32_t movescore_t;

class CMoveHistory
{
public:

	static const int cBitsPerMove = 10;
	static const int cTableSize = 1 << cBitsPerMove;

	CMoveHistory()
	{
		mTable = new movescore_t[cTableSize];
		Reset();
	}

	~CMoveHistory()
	{
		delete mTable;
	}

	void Reset()
	{
		memset(mTable, 0, sizeof(movescore_t)*cTableSize);
	}

	int Index(const CMove &move)
	{
		uint8_t from = move[0];
		uint8_t to = move[move.Length() -1];
		return (from << 5) & to;
	}

	movescore_t Lookup(const CMove &move)
	{
		return mTable[Index(move)];
	}

	void Set(const CMove &move, movescore_t score) {
		mTable[Index(move)] = score;
	}

	void Increase(const CMove &move, movescore_t delta) {
		mTable[Index(move)] += delta;
	}

private:
	movescore_t mTable[];
};

}

#endif /* CMOVEHISTORY_H_ */
