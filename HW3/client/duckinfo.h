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
#include <iostream>

using std::cout;
using std::endl;


namespace ducks {


class DuckInfo
{
public:
	typedef HMM<3,DuckObservation::numObservations, DuckObservation> hmm_t;


	DuckInfo():
		mDuck(0),
		mModel(),
		mObs()
	{
	}

	void setDuck(const CDuck *duck) {
		mModel.setObservations(&mObs);
		mDuck = duck;
	}

	void iteration(int round) {
		mAllModels.resize(round);
		mAllModels[round-1] = hmm_t::model_t(mModel.getModel());
	}

	void update(bool verbose)
	{
		int n = mObs.size();
		for (int i = n; i < mDuck->GetSeqLength(); ++i) {
			mObs.push_back(mDuck->GetAction(i));
		}

		mModel.learnModel(10, verbose, 50);
	}

	hmm_t& getModel() {
		return mModel;
	}

	void analyseDevelopment() {
		for (int t = 0; t < mAllModels.size(); ++t) {
			hmm_t::model_t m = mAllModels[t];
			cout << "t " << t << ", dist " << std::setprecision(15) << mModel.simple_distance(m);
			hmm_t hmm(mAllModels[t]);
			cout << ", kl dist " << std::setprecision(15) << mModel.distance(hmm) << endl;
		}
	}

	prob currDistance(DuckInfo &other) {
		cout <<  "foo: " << std::setprecision(15) << mModel.simple_distance(other.mModel.getModel());
		return mModel.distance(other.mModel);
	}

private:

	std::vector<DuckObservation> mObs;
	const CDuck *mDuck;
	hmm_t mModel;

	std::vector<hmm_t::model_t> mAllModels;
};


}


#endif /* DUCKINFO_H_ */
