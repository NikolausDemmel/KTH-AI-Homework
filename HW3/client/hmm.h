/*
 * hmm.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef HMM_H_
#define HMM_H_

#include "common.h"
#include <cassert>
#include <limits>
#include <list>
#include <iomanip>
#include <algorithm>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>


using namespace boost::numeric::ublas;
using std::cout;
using std::endl;
using std::setw;
using std::numeric_limits;
using std::list;
using std::pair;
using std::string;

namespace ducks {


template<int N, int M, class observation, class prob = double>
class HMM {
public:

	HMM(std::vector<list<string>> obs_split_names = std::vector<list<string>>()):
		obs_names(obs_split_names)
	{
		set1();
		standardInitialization();
		A2 = A1;
		B2 = B1;
		pi2 = pi1;
	}

	void standardInitialization() {
		for(int i = 0; i < N; ++i) {
			(*pi)[i] = 1.0/N + random_delta<prob>(1.0/N);
			for(int j = 0; j < N; ++j) {
				(*A)(i,j) = 1.0/N + random_delta<prob>(1.0/N);
			}
			row((*A),i) /= sum(row((*A),i));
			for(int j = 0; j < M; ++j) {
				(*B)(i,j) = 1.0/M + random_delta<prob>(1.0/M);
			}
			row((*B),i) /= sum(row((*B),i));
		}
		(*pi) /= sum((*pi));
	}

	bool validObservations() const {
		if (T > obs.size())
			return false;
		foreach(observation o, obs) {
			if (o < 0 || o >= M) {
				return false;
			}
		}
		return true;
	}

	void setObservations(const std::vector<observation> &obs_, int T_ = -1) {
		if (T_ < 0) {
			T_ = obs_.size();
		}
		obs = obs_;
		T = T_;
	}

	void set1() {
		c = &c1;
		alpha = &alpha1;
		beta = &beta1;
		gamma = &gamma1;
		bigamma = &bigamma1;
		A = &A1;
		B = &B1;
		pi = &pi1;
	}

	void set2() {
		c = &c2;
		alpha = &alpha2;
		beta = &beta2;
		gamma = &gamma2;
		bigamma = &bigamma2;
		A = &A2;
		B = &B2;
		pi = &pi2;
	}

	observation predictNext() {
		c_vector<prob, M> obs_dist = prod((*alpha)[T-1],(*B));
		return observation(std::max_element(obs_dist.begin(),obs_dist.end()) - obs_dist.begin());
	}

	void alphaPass() {
		noalias((*alpha)[0]) = element_prod((*pi),column((*B),obs[0]));
		(*c)[0] = 1 / sum((*alpha)[0]);
		(*alpha)[0] *= (*c)[0];

		for (int t = 1; t < T; ++t) {
			noalias((*alpha)[t]) = element_prod(prod((*alpha)[t-1], (*A)), column((*B),obs[t]));
			(*c)[t] = 1 / sum((*alpha)[t]);
			(*alpha)[t] *= (*c)[t];
		}
	}

	void alphaPassLoopy() {
		// compute alpha(0)
		(*c)[0] = 0;
		for (int i = 0; i < N; ++i) {
			(*alpha)[0][i] = (*pi)[i]*(*B)(i,obs[0]);
			(*c)[0] += (*alpha)[0][i];
		}

		// scale alpha(0,i)
		(*c)[0] = 1/(*c)[0];
		for (int i = 0; i < N; ++i) {
			(*alpha)[0][i] *= (*c)[0];
		}

		// compute alpha(t,i)
		for (int t = 1; t < T; ++t) {
			(*c)[t] = 0;
			for (int i = 0; i < N; ++i) {
				(*alpha)[t][i] = 0;
				for (int j = 0; j < N; ++j) {
					(*alpha)[t][i] += (*alpha)[t-1][j]*(*A)(j,i);
				}
				(*alpha)[t][i] *= (*B)(i,obs[t]);
				(*c)[t] += (*alpha)[t][i];
			}

			// scale alpha(t,i)
			(*c)[t] = 1/(*c)[t];
			for (int i = 0; i < N; ++i) {
				(*alpha)[t][i] *= (*c)[t];
			}
		}
	}

	void betaPass() {
		noalias((*beta)[T-1]) = scalar_vector<prob>(N, (*c)[T-1]);

		for (int t = T-2; t >= 0; --t) {
			noalias((*beta)[t]) = prod((*A), element_prod(column((*B),obs[t+1]), (*beta)[t+1])) * (*c)[t];
		}
	}

	void betaPassLoopy() {
		// let beta(T-1,i) = 1 scaled by c(T-1)
		for (int i = 0; i < N; ++i) {
			(*beta)[T-1][i] = (*c)[T-1];
		}

		// beta pass
		for (int t = T-2; t >= 0; --t) {
			for (int i = 0; i < N; ++i) {
				(*beta)[t][i] = 0;
				for (int j = 0; j < N; ++j) {
					(*beta)[t][i] += (*A)(i,j)*(*B)(j,obs[t+1])*(*beta)[t+1][j];
				}
				// scale beta(t,i) with same factor as alpha(t,i)
				(*beta)[t][i] *= (*c)[t];
			}
		}
	}

	void gammaPass() {
		for (int t = 0; t < T-1; ++t) {

			noalias((*bigamma)[t]) =
				element_prod((*A), outer_prod((*alpha)[t],
										   element_prod(column((*B),obs[t+1]),
												   	    (*beta)[t+1])));
			(*bigamma)[t] /= sum(prod(scalar_vector<prob>(N, 1.0), (*bigamma)[t]));
			noalias((*gamma)[t]) = prod((*bigamma)[t], scalar_vector<prob>(N,1));
		}
	}

	void gammaPassLoopy() {
		for (int t = 0; t < T-1; ++t) {
			prob denom = 0;
			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < N; ++j) {
					denom += (*alpha)[t][i]*(*A)(i,j)*(*B)(j,obs[t+1])*(*beta)[t+1][j];
				}
			}
			for (int i = 0; i < N; ++i) {
				(*gamma)[t][i] = 0;
				for (int j = 0; j < N; ++j) {
					(*bigamma)[t](i,j) = ((*alpha)[t][i]*(*A)(i,j)*(*B)(j,obs[t+1])*(*beta)[t+1][j]) / denom;
					(*gamma)[t][i] += (*bigamma)[t](i,j);
				}
			}
		}
	}

	void reestimateModel() {
		// re-estimate pi
		(*pi) = (*gamma)[0];

		// re-estimate A
		(*A).clear();
		state_dist_t gammasum;
		for (int t = 0; t < T; ++t) {
			(*A) += (*bigamma)[t];
			gammasum += (*gamma)[t];
		}
		(*A) = element_div((*A), outer_prod(gammasum, scalar_vector<prob>(N,1)));

		// re-estimate B
		(*B).clear();
		for (int t = 0; t < T-1; ++t) {
			noalias(column((*B),obs[t])) += (*gamma)[t];
		}
		(*B) = element_div((*B), outer_prod(gammasum-(*gamma)[T-1], scalar_vector<prob>(M,1)));
	}

	void reestimateModelLoopy() {
		// re-estimate pi
		for (int i = 0; i < N; ++i) {
			(*pi)[i] = (*gamma)[0][i];
		}

		// re-estimate A
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				prob numer = 0;
				prob denom = 0;
				for (int t = 0; t < T-1; ++t) {
					numer += (*bigamma)[t](i,j);
					denom += (*gamma)[t][i];
				}
				(*A)(i,j) = numer/denom;
			}
		}

		// re-estimate B
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < M; ++j) {
				prob numer = 0;
				prob denom = 0;
				for (int t = 0; t < T-1; ++t) {
					if (obs[t] == j) {
						numer += (*gamma)[t][i];
					}
					denom += (*gamma)[t][i];
				}
				(*B)(i,j) = numer/denom;
			}
		}
	}

	void learnModel() {
		assert(validObservations());

#ifdef DEBUGfg
		cout << "learnModel call with T = " << T << " observations: " << endl;
		for (int t = 0; t < obs.size(); ++t) {
			cout << obs[t].str() << " ";
		}
		cout << endl;
#endif
		////////////////
		// 1. Initialization

		int maxIters = 10000; // FIXME: this is arbitrary
		prob eps = 1e-7; // FIXME: this is arbitrary
		int iters = 0;
		prob logProb = -numeric_limits<prob>::max();
		prob oldLogProb = 0;

		c1.resize(T);
		alpha1.resize(T);
		beta1.resize(T);
		gamma1.resize(T);
		bigamma1.resize(T);

//		c2.resize(T);
//		alpha2.resize(T);
//		beta2.resize(T);
//		gamma2.resize(T);
//		bigamma2.resize(T);

//		check_differences();

		do
		{
			oldLogProb = logProb;
			check_timeout();

#ifdef DEBUGjj
			//cout << "        ITERATION " << iters << endl << endl;
#endif
			////////////////
			// 2. alpha pass
			set1();
			alphaPass();

//			set2();
//			alphaPassLoopy();

			////////////////
			// 3. beta pass
			set1();
			betaPass();

//			set2();
//			betaPassLoopy();

			////////////////
			// 4. compute bi_gamma and gamma
			set1();
			gammaPass();

//			set2();
//			gammaPassLoopy();

			////////////////
			// 5. re-estimate A, B and pi
			set1();
			reestimateModel();

//			set2();
//			reestimateModelLoopy();

			////////////////
			// 6. compute log[P(O|lambda)]

//			check_differences();
			//printState();

			set1();
			logProb = 0;
			for (int t = 0; t < T; ++t) {
				logProb += log2((*c)[t]);
			}
			logProb = -logProb;

#ifdef DEBUG
			cout << "LogProb delta:" << logProb - oldLogProb << ", " << logProb << ", " << oldLogProb << endl;
#endif

			// 7. To iterate or not to iterate, that is the question :)

			++iters;
		}
		while (iters < maxIters && logProb-eps > oldLogProb);

#ifdef DEBUG
		cout << "learnModel ended after iteration " << iters << endl;
		std::cerr << iters << endl;
#endif

	}

	void printState() {

		// print A
		cout << "A:" << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				cout << (*A)(i,j) << " ";
			}
			cout << endl;
		}

		// print B
		cout << endl << "B: " << endl;
		for (int j = 0; j < M; ++j) {
			cout << setw(7) << observation(j).str() << " ";
		}
		cout << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < M; ++j) {
				cout << (*B)(i,j) << " ";
			}
			cout << endl;
		}

		// print pi
		cout << endl << "pi: " << endl;
		for (int i = 0; i < N; ++i) {
			cout << (*pi)[i] << " ";
		}
		cout << endl << endl;

		// print split of observations
		calculate_B_split();
		print_B_split();
	}

	void calculate_B_split()
	{
		// split up observation in independent parts.
		if (!obs_names.empty()) {
			int total = 1;
			foreach (list<string> x, obs_names) {
				total *= x.size() - 1;
			}
			assert (total == M);
			B_split.clear();

			int factor = 1;
			foreach (list<string> x, obs_names) {
				int m = x.size() - 1; // first one is name of group
				matrix<prob> mat = matrix<prob>(N,m);
				mat.clear();
				for (int i = 0; i < N; ++i) {
					for (int j = 0; j < M; ++j) {
						mat(i, (j/factor) % m) += (*B)(i,j);
					}
				}
				factor *= m;
				B_split.push_back(mat);
			}
		}
	}

	void print_B_split()
	{
		if (!B_split.empty()) {
			cout << "Split of B in " << obs_names.size() << " independent distributions:" << endl;
			int k = 0;
			foreach (list<string> names, obs_names) {
				matrix<prob> mat = B_split[k];
				++k;
				int m = names.size() - 1;
				cout << "B-" << names.front() << endl;
				for (auto it = ++(names.begin()); it != names.end(); ++it) {
					cout << setw(7) << *it << " ";
				}
				cout << endl;
				for (int i = 0; i < N; ++i) {
					for (int j = 0; j < m; ++j) {
						cout << mat(i,j) << " ";
					}
					cout << endl;
				}
				cout << endl;
			}
		}
	}

	template<class T>
	void check_difference_many_abs(const string name, const vector<T> &a, const vector<T> &b) const {
		cout << name << "(" << a.size() << "," << b.size() << "): ";
		double diff = 0;
		for (int i = 0; i < a.size(); ++i) {
			diff += abs(a[i] - b[i]);
		}
		diff /= a.size();
		cout << diff;
	}

	template<class T>
	void check_difference_many_norm1(const string name, const vector<T> &a, const vector<T> &b) const {
		cout << name << "(" << a.size() << "," << b.size() << "): ";
		double diff = 0;
		for (int i = 0; i < a.size(); ++i) {
			diff += norm_1(a[i] - b[i]);
		}
		diff /= a.size();
		cout << diff;
	}

	template<class T>
	void check_difference_many_norm2(const string name, const vector<T> &a, const vector<T> &b) const {
		cout << name << "(" << a.size() << "," << b.size() << "): ";
		double diff = 0;
		for (int i = 0; i < a.size(); ++i) {
			diff += norm_2(a[i] - b[i]);
		}
		diff /= a.size();
		cout << diff;
	}

	template<class T>
	void check_difference_one_norm1(const string name, const T &a, const T &b) const {
		cout << name << ": " << norm_1(a - b);
	}

	template<class T>
	void check_difference_one_norm2(const string name, const T &a, const T &b) const {
		cout << name << ": " << norm_2(a - b);
	}

	void check_differences() const {
		cout << "Checking differences:" << endl;
		check_difference_many_abs("c", c1, c2);
		cout << endl;
		check_difference_many_norm2("alpha", alpha1, alpha2);
		cout << endl;
		check_difference_many_norm2("beta", beta1, beta2);
		cout << endl;
		check_difference_many_norm2("gamma", gamma1, gamma2);
		cout << endl;
		check_difference_many_norm1("bigamma", bigamma1, bigamma2);
		cout << endl;

		check_difference_one_norm1("A", A1, A2);
		cout << endl;
		check_difference_one_norm1("B", B1, B2);
		cout << endl;
		check_difference_one_norm2("pi", pi1, pi2);
		cout << endl;
	}

public:
	typedef c_vector<prob, N> state_dist_t;
	typedef c_matrix<prob, N, N> state_state_trans_t;
	typedef c_matrix<prob, N, M> state_obs_trans_t;

	vector<prob> *c;
	vector<state_dist_t> *alpha;
	vector<state_dist_t> *beta;
	vector<state_dist_t> *gamma;
	vector<state_state_trans_t> *bigamma;

	state_state_trans_t *A;
	state_obs_trans_t *B;
	state_dist_t *pi;

	vector<prob> c1;
	vector<state_dist_t> alpha1;
	vector<state_dist_t> beta1;
	vector<state_dist_t> gamma1;
	vector<state_state_trans_t> bigamma1;

	state_state_trans_t A1;
	state_obs_trans_t B1;
	state_dist_t pi1;

	vector<prob> c2;
	vector<state_dist_t> alpha2;
	vector<state_dist_t> beta2;
	vector<state_dist_t> gamma2;
	vector<state_state_trans_t> bigamma2;

	state_state_trans_t A2;
	state_obs_trans_t B2;
	state_dist_t pi2;

	std::vector<list<string>> obs_names;
	std::vector<matrix<prob>> B_split;

	int T;
	std::vector<observation> obs;
};


}


#endif /* HMM_H_ */
