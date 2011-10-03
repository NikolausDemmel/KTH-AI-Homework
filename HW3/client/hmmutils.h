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


}


#endif /* HMMUTILS_H_ */
