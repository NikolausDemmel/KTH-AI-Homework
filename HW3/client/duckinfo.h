/*
 * duckinfo.h
 *
 *  Created on: 02.10.2011
 *      Author: demmeln
 */

#ifndef DUCKINFO_H_
#define DUCKINFO_H_


#include "cduck.h"
#include "duckobservation.h"


namespace ducks {


class DuckInfo
{
public:
	typedef HMM<3,9, DuckObservation> hmm_t;


	DuckInfo():
		mDuck(0),
		mModel(),
		mObs()
	{
	}

	/// FIXME copy constructur would really be needed, otherwise everything will be screwed up

	void setDuck(const CDuck *duck) {
		mModel.setObservations(&mObs);
		mDuck = duck;
	}

	void update(int newTurns)
	{
		int n = mObs.size();
		for (int i = n; i < n + newTurns; ++i) {
			mObs.push_back(mDuck->GetAction(i));
		}

		// Sanity check
#ifdef DEBUG
		if (mObs.size() != mDuck->GetSeqLength()) {
			throw std::runtime_error("FOOOOOOO!");
		}
#endif

		mModel.learnModel(10);
	}

	hmm_t& getModel() {
		return mModel;
	}

private:

	std::vector<DuckObservation> mObs;
	const CDuck *mDuck;
	hmm_t mModel;
};


}


#endif /* DUCKINFO_H_ */
