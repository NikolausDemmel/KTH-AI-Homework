/*
 * test.cpp
 *
 *  Created on: 13.09.2011
 *      Author: demmeln
 */

#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;

bool compare(int a, int b) {
	return a > b;
}

int main() {
	int foo[] = {4,2,6,3};
	vector<int> vec = vector<int>(&foo[0],&foo[4]);

	sort(vec.begin(), vec.end(), compare);

	for(vector<int>::iterator i = vec.begin(); i != vec.end(); ++i) {
		cout << *i << endl;
	}


	return 0;
}

