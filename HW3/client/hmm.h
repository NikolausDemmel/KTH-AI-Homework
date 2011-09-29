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
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>


using namespace boost::numeric::ublas;
using std::cout;
using std::fixed;
using std::endl;
using std::numeric_limits;

namespace ducks {


template<int N, int M, class observation, class prob = double>
class HMM {
public:

	HMM() {
		standardInitialization();
	}

	static void normalizeRow(prob *row, int n) {
		prob sum = 0;
		for(int i = 0; i < n; ++i) {
			sum += row[i];
		}
		for(int i = 0; i < n; ++i) {
			row[i] /= sum;
		}
	}

	void standardInitialization() {
		for(int i = 0; i < N; ++i) {
			pi[i] = 1.0/N + random_delta<prob>(1.0/N);
			for(int j = 0; j < N; ++j) {
				A[i][j] = 1.0/N + random_delta<prob>(1.0/N);
			}
			normalizeRow(A[i], N);
			for(int j = 0; j < M; ++j) {
				B[i][j] = 1.0/M + random_delta<prob>(1.0/M);
			}
			normalizeRow(B[i], M);
		}
		normalizeRow(pi, N);
	}

	bool verifyObservations(std::vector<observation> obs) {
		for(auto it = obs.begin(); it != obs.end(); ++it) {
			if (*it < 0 || *it >= M) {
				return false;
			}
		}
		return true;
	}

	void learnModel(std::vector<observation> obs) {
		assert(verifyObservations(obs));

#ifdef DEBUG
		cout << "learnModel call with observations: " << endl;
		for (int t = 0; t < obs.size(); ++t) {
			cout << obs[t].str() << " ";
		}
		cout << endl;
#endif

		// 1. Initialization


		int T = obs.size();

		int maxIters = 1000; // FIXME: this is arbitrary
		int iters = 0;
		prob oldLogProb = -numeric_limits<prob>::max();

		c.resize(T);
		alpha.resize(T);
		beta.resize(T);
		gamma.resize(T);
		bigamma.resize(T);

		while(true)
		{
			// 2. alpha pass

			// compute alpha(0)
			c[0] = 0;
			for (int i = 0; i < N; ++i) {
				alpha[0][i] = pi[i]*B[i][obs[0]];
				c[0] += alpha[0][i];
			}

			// scale alpha(0,i)
			c[0] = 1/c[0];
			for (int i = 0; i < N; ++i) {
				alpha[0][i] *= c[0];
			}

			// compute alpha(t,i)
			for (int t = 1; t < T; ++t) {
				c[t] = 0;
				for (int i = 0; i < N; ++i) {
					alpha[t][i] = 0;
					for (int j = 0; j < N; ++j) {
						alpha[t][i] += alpha[t-1][j]*A[j][i];
					}
					alpha[t][i] *= B[i][obs[t]];
					c[t] += alpha[t][i];
				}

				// scale alpha(t,i)
				c[t] = 1/c[t];
				for (int i = 0; i < N; ++i) {
					alpha[t][i] *= c[t];
				}
			}

			// 3. beta pass

			// let beta(T-1,i) = 1 scaled by c(T-1)
			for (int i = 0; i < N; ++i) {
				beta[T-1][i] = c[T-1];
			}

			// beta pass
			for (int t = T-2; t >= 0; --t) {
				for (int i = 0; i < N; ++i) {
					beta[t][i] = 0;
					for (int j = 0; j < N; ++j) {
						beta[t][i] += A[i][j]*B[j][obs[t+1]]*beta[t+1][j];
					}
					// scale beta(t,i) with same factor as alpha(t,i)
					beta[t][i] *= c[t];
				}
			}

			// 4. compute bi_gamma and gamma

			for (int t = 0; t < T-1; ++t) {
				prob denom = 0;
				for (int i = 0; i < N; ++i) {
					for (int j = 0; j < N; ++j) {
						denom += alpha[t][i]*A[i][j]*B[j][obs[t+1]]*beta[t+1][j];
					}
				}
				for (int i = 0; i < N; ++i) {
					gamma[t][i] = 0;
					for (int j = 0; j < N; ++j) {
						bigamma[t][i][j] = (alpha[t][i]*A[i][j]*B[j][obs[t+1]]*beta[t+1][j]) / denom;
						gamma[t][i] += bigamma[t][i][j];
					}
				}
			}

			// 5. re-estimate A, B and pi

			// re-estimate pi
			for (int i = 0; i < N; ++i) {
				pi[i] = gamma[0][i];
			}

			// re-estimate A
			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < N; ++j) {
					prob numer = 0;
					prob denom = 0;
					for (int t = 0; t < T-1; ++t) {
						numer += bigamma[t][i][j];
						denom += gamma[t][i];
					}
					A[i][j] = numer/denom;
				}
			}

			// re-estimate B
			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < M; ++j) {
					prob numer = 0;
					prob denom = 0;
					for (int t = 0; t < T-1; ++t) {
						if (obs[t] == j) {
							numer += gamma[t][i];
						}
						denom += gamma[t][i];
					}
					B[i][j] = numer/denom;
				}
			}

			// 6. compute log[P(O|lambda)]

			prob logProb = 0;
			for (int t = 0; t < T; ++t) {
				logProb += log2(c[t]);
			}
			logProb = -logProb;


#ifdef FOO
			cout.precision(10);
			cout << "LogProb delta:" << logProb - oldLogProb << endl;
#endif

			// 7. To iterate or not to iterate, that is the question :)

			++iters;
			if (iters < maxIters && logProb > oldLogProb) {
				oldLogProb = logProb;
				continue;
			}
			break;
		}

#ifdef DEBUG
		cout << "learnModel ended after iteration " << iters << endl;
#endif

	}

	void printState() {
		cout << "A:" << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				cout << fixed << A[i][j] << " ";
			}
			cout << endl;
		}
		cout << endl << "B: " << endl;
		for (int j = 0; j < M; ++j) {
			cout << observation(j).str() << "      ";
		}
		cout << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < M; ++j) {
				cout << fixed << B[i][j] << " ";
			}
			cout << endl;
		}

		/// Not general:
		cout << endl << "B-H: " << endl;
		cout << "A       K       S" << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < 3; ++j) {
				prob val = 0;
				for (int k = 0; k < 3; ++k) {
					val += B[i][j+3*k];
				}
				cout << fixed << val << " ";
			}
			cout << endl;
		}
		cout << endl << "B-V: " << endl;
		cout << "A       K       S" << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < 3; ++j) {
				prob val = 0;
				for (int k = 0; k < 3; ++k) {
					val += B[i][3*j+k];
				}
				cout << fixed << val << " ";
			}
			cout << endl;
		}

		cout << endl << "pi: " << endl;
		for (int i = 0; i < N; ++i) {
			cout << fixed << pi[i] << " ";
		}
		cout << endl << endl;
	}

public:
	typedef c_vector<prob, N> state_dist_t;
	typedef c_matrix<prob, N, N> state_state_dist_t;
	typedef c_matrix<prob, N, M> state_obs_dist_t;
	typedef state_state_dist_t A_t;
	typedef state_state_dist_t bigamma_t;

	vector<prob> c;
	vector<state_dist_t> alpha;
	vector<state_dist_t> beta;
	vector<state_dist_t> gamma;
	vector<state_state_dist_t> bigamma;

	state_state_dist_t A;
	state_obs_dist_t B;
	state_dist_t pi;
};


}


#endif /* HMM_H_ */
