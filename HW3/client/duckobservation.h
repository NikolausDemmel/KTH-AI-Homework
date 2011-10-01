/*
 * duckobservation.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef DUCKOBSERVATION_H_
#define DUCKOBSERVATION_H_


#include <sstream>
#include <vector>
#include <list>

using std::string;
using std::list;


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

	string str() const
	{
		std::stringstream ss;
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
