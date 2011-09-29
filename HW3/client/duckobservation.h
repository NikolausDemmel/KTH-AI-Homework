/*
 * duckobservation.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef DUCKOBSERVATION_H_
#define DUCKOBSERVATION_H_


#include <sstream>

using namespace std;


namespace ducks {


class DuckObservation
{
public:
	DuckObservation(const CAction& action)
	{
		mObs = action.GetHAction() + 3*action.GetVAction();
	}

	DuckObservation(int obs):
		mObs(obs)
	{
	}

	DuckObservation():
		mObs(0)
	{
	}

	string str() const
	{
		stringstream ss;
		int h = mObs % 3;
		int v = mObs / 3;
		ss << "" << actionToShortString(static_cast<EAction>(h));
		ss << "" << actionToShortString(static_cast<EAction>(v));
		return ss.str();
	}

	operator int() const
	{
		return mObs;
	}

private:
	int mObs;
};


}


#endif /* DUCKOBSERVATION_H_ */
