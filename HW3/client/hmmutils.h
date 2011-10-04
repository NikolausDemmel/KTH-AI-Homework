/*
 * hmmutils.h
 *
 *  Created on: 03.10.2011
 *      Author: demmeln
 */

#ifndef HMMUTILS_H_
#define HMMUTILS_H_


#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>


using namespace boost::numeric::ublas;


namespace ducks {


template<class prob>
prob roughly(prob val) {
	return val + random_delta<prob>(val, 0.1); // +- 10 %
}

template<class T>
void normalize(T &vec) {
	vec /= sum(vec);
}

template<class M>
void normalize(matrix_row<M> row) {
	row /= sum(row);
}

template<class V>
void addNoise(V &vec, double noise = 0.001) {
	// add noise to stachastic vector
	for (int i = 0; i < vec.size(); ++i) {
		vec[i] += random<double>(0, noise);
	}
	normalize(vec);
}

template<class M, class prob>
void addNoise(matrix_row<M> row, prob noise = 0.001) {
	// add noise to stachastic vector
	for (int i = 0; i < row.size(); ++i) {
		row[i] += random<prob>(0, noise);
	}
	normalize(row);
}

template<int N, int M, class prob>
void addNoise(c_matrix<prob,N,M> &mat, prob noise = 0.001) {
	// Add noise to a row stachastic matrix
	for (int i = 0; i < mat.size1(); ++i) {
		addNoise<c_matrix<prob,N,M>, prob>(row(mat,i));
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


}


#endif /* HMMUTILS_H_ */
