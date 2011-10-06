/*
 * duckobservation.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef DUCKOBSERVATION_H_
#define DUCKOBSERVATION_H_


#include "defines.h"

#include <sstream>
#include <vector>
#include <list>

using std::string;
using std::list;


namespace ducks {


class DuckObservation
{
public:
	static const int numObservations = 9;

	DuckObservation(const CAction& action)
	{
		mObs = hvToObs(action.GetHAction(), action.GetVAction());
	}

	DuckObservation(int obs):
		mObs(obs)
	{
	}

	DuckObservation():
		mObs(0)
	{
	}

	operator int() const
	{
		return mObs;
	}

	static int hvToObs(int h, int v) {
		return h + 3*v;
	}

	CAction toAction(int birdNumber = 0, int movement = 0) const
	{
		EAction h = static_cast<EAction>(mObs % 3);
		EAction v = static_cast<EAction>(mObs / 3);
		return CAction(birdNumber, h, v, movement);
	}

	string str() const
	{
		std::stringstream ss;
		int h = mObs % 3;
		int v = mObs / 3;
		ss << "" << actionToShortString(static_cast<EAction>(h));
		ss << "" << actionToShortString(static_cast<EAction>(v));
		return ss.str();
	}

	static const int splitHindex = 0;
	static const int splitVindex = 1;

	static std::vector<list<string>> getSplitNames() {
		std::vector<list<string>> names;

		list<string> hori;
		hori.push_back("H");
		for(int a = 0; a < 3; ++a) {
			hori.push_back(actionToShortString(static_cast<EAction>(a)));
		}
		names.push_back(hori);

		list<string> vert;
		vert.push_back("V");
		for(int a = 0; a < 3; ++a) {
			vert.push_back(actionToShortString(static_cast<EAction>(a)));
		}
		names.push_back(vert);

		return names;
	}

private:
	int mObs;
};


}


#endif /* DUCKOBSERVATION_H_ */
