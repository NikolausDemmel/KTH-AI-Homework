/*
 * hmmutils.h
 *
 *  Created on: 03.10.2011
 *      Author: demmeln
 */

#ifndef HMMUTILS_H_
#define HMMUTILS_H_


#include "defines.h"


#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>

#include <boost/math/special_functions/fpclassify.hpp>
using namespace boost::math;


using namespace boost::numeric::ublas;


namespace ducks {


const double threshhold = 1e-10;


template<class prob>
prob roughly(prob val) {
	return val + random_delta<prob>(val, 0.1); // +- 10 %
}

template<class T>
bool is_stochastic(T &vec) {
	double sum = 0;
	for (int i = 0; i < vec.size(); ++i) {
		sum += vec[i];
	}
	return abs(sum-1.0) < threshhold;
}

template<class M>
bool is_stochastic(matrix_row<M> row) {
	double sum = 0;
	for (int i = 0; i < row.size(); ++i) {
		sum += row[i];
	}
	return abs(sum-1.0) < threshhold;
}

template<class T>
bool is_row_stochastic(T &mat) {
	for (int i = 0; i < mat.size1(); ++i) {
		if (! is_stochastic(row(mat,i)))
			return false;
	}
	return true;
}

template<class T>
void normalize(T &vec) {
	vec /= sum(vec);

	// I'm getting insane looking for the error, so go crazy with sanity checks
	is_stochastic(vec);
}

template<class M>
void normalize(matrix_row<M> row) {
	row /= sum(row);

	// I'm getting insane looking for the error, so crazy sanity checks
	double sum = 0;
	for (int i = 0; i < row.size(); ++i) {
		sum += row[i];
	}
	if (abs(sum-1.0)>1e-10)
		throw "FOOOO normalize sucks...";
}

template<class V>
void addNoise(V &vec, double noise = 0.001) {
	// add noise to stachastic vector
	for (int i = 0; i < vec.size(); ++i) {
		vec[i] += random<double>(0, noise);
	}
	normalize(vec);


	// I'm getting insane looking for the error, so crazy sanity checks
	double sum = 0;
	for (int i = 0; i < vec.size(); ++i) {
		sum += vec[i];
	}
	if (abs(sum-1.0)>1e-10)
		throw "FOOOO normalize sucks...";
}

template<class M, class prob>
void addNoise(matrix_row<M> row, prob noise = 0.001) {
	// add noise to stachastic vector
	for (int i = 0; i < row.size(); ++i) {
		row[i] += random<prob>(0, noise);
	}
	normalize(row);

	// I'm getting insane looking for the error, so crazy sanity checks
	double sum = 0;
	for (int i = 0; i < row.size(); ++i) {
		sum += row[i];
	}
	if (abs(sum-1.0)>1e-10)
		throw "FOOOO normalize sucks...";
}

template<int N, int M, class prob>
void addNoise(c_matrix<prob,N,M> &mat, prob noise = 0.001) {
	// Add noise to a row stachastic matrix
	for (int i = 0; i < mat.size1(); ++i) {
		addNoise<c_matrix<prob,N,M>, prob>(row(mat,i));

		// I'm getting insane looking for the error, so crazy sanity checks
		double sum = 0;
		for (int j = 0; j < mat.size2(); ++j) {
			sum += mat(i,j);
		}
		if (abs(sum-1.0)>1e-10)
			throw "FOOOO normalize sucks...";
	}
}

template<int N, class prob>
prob manhatten_metric(const c_vector<prob,N> &v1, const c_vector<prob,N> &v2) {
	prob sum = 0;
	for (int i = 0; i < N; ++i) {
		sum += abs(v1(i) - v2(i));
	}
	return sum;
}

template<int N, int M, class prob>
prob manhatten_metric(const c_matrix<prob, N, M> &m1, const c_matrix<prob, N, M> &m2) {
	prob sum = 0;
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < M; ++j) {
			sum += abs(m1(i,j) - m2(i,j));
		}
	}
	return sum;
}

template<int N, int M, class prob>
prob manhattelMetricRow(const c_matrix<prob, N, M> &m1, const c_matrix<prob, N, M> &m2, int row) {
	prob sum = 0;
	for (int j = 0; j < M; ++j) {
		sum += abs(m1(row,j) - m2(row,j));
	}
	return sum;
}

template<class V>
int sample(const V &vec) {
	double rand = random(0,1);
	for (int i = 0; i < vec.size(); ++i) {
		rand -= vec(i);
		if (rand < 0)
			return i;
	}
	return vec.size()-1; // failsafe
}

template<class Mat>
int sample(const Mat &mat, int row) {
	double rand = random(0,1);
	for (int i = 0; i < mat.size2(); ++i) {
		rand -= mat(row, i);
		if (rand < 0)
			return i;
	}
	return mat.size2()-1; // failsafe
}

template<class T>
bool vec_has_nan(T &vec) {
	for (int i = 0; i < vec.size(); ++i) {
		if ((isnan)(vec[i]))
			return true;
	}
	return false;
}

template<class T>
bool mat_has_nan(T &mat) {
	for (int i = 0; i < mat.size1(); ++i) {
		for (int j = 0; j < mat.size2(); ++j) {
			if ((isnan)(mat(i,j)))
				return true;
		}
	}
	return false;
}


/// extended logarithm and friends

const double LOGZERO = std::numeric_limits<double>::quiet_NaN();

static bool islogzero(double x) {
	return (isnan)(x);
}

static double eexp(double x) {
	if (islogzero(x))
		return 0;
	else
		return exp(x);
}

static double eln(double x) {
	if (x == 0) {
		return LOGZERO;
	} else if(x > 0) {
		return log(x);
	} else {
		throw "negative input error";
	}
}

static double elnsum(double elnx, double elny) {
	if ( islogzero(elnx) || islogzero(elny) ) {
		if ( islogzero(elnx) )
			return elny;
		else
			return elnx;
	} else {
		if ( elnx > elny ) {
			return elnx + eln(1 + exp(elny - elnx));
		} else {
			return elny + eln(1 + exp(elnx - elny));
		}
	}

}

static double elnproduct(double elnx, double elny) {
	if (islogzero(elnx) || islogzero(elny))
		return LOGZERO;
	else
		return elnx + elny;
}



}


#endif /* HMMUTILS_H_ */
