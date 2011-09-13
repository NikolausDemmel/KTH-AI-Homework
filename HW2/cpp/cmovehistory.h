/*
 * cmovehistory.h
 *
 *  Created on: 13.09.2011
 *      Author: demmeln
 */

#ifndef CMOVEHISTORY_H_
#define CMOVEHISTORY_H_

#include <cstring>
#include <functional>

using namespace std;

namespace chk {

typedef uint32_t movescore_t;

class CMoveHistory
{
public:

	static const int cBitsPerMove = 10;
	static const int cTableSize = 1 << cBitsPerMove;

	CMoveHistory() :
		mCompareMoves(this)
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

	movescore_t MaxScore() {
		movescore_t s = 0;
		for(int i = 0; i < cTableSize; ++i) {
			s = max(s, mTable[i]);
		}
		return s;
	}

	void DampScores(int bits) {
		for(int i = 0; i < cTableSize; ++i) {
			mTable[i] >>= bits;
		}
	}

	int Index(const CMove &move) const
	{
		uint8_t from = move[0];
		uint8_t to = move[move.Length() -1];
		return (from << 5) | to;
	}

	movescore_t Lookup(const CMove &move) const
	{
		return mTable[Index(move)];
	}

	void Set(const CMove &move, movescore_t score) {
		mTable[Index(move)] = score;
	}

	void Increase(const CMove &move, movescore_t delta) {
		mTable[Index(move)] += delta;
	}

	struct CompareMoves : public binary_function<const CMove &, const CMove &, bool>
	{
		CompareMoves(const CMoveHistory* const history) :
			mHistory(history)
		{
		}

		bool operator()(const CMove &lhs,const CMove &rhs)
	    {
	          return mHistory->Lookup(lhs) > mHistory->Lookup(rhs);
	    }

		const CMoveHistory* mHistory;
	};

	const CompareMoves mCompareMoves;

private:
	movescore_t *mTable;
};

}

#endif /* CMOVEHISTORY_H_ */
