/*
 * hmm.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef HMM_H_
#define HMM_H_

#include "common.h"
#include "hmmutils.h"
#include <cassert>
#include <limits>
#include <list>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>

using namespace boost::numeric::ublas;
using std::cout;
using std::cerr;
using std::endl;
using std::setw;
using std::numeric_limits;
using std::list;
using std::pair;
using std::string;
using std::fstream;


#define DEBUG_FILE "./output.csv"
#define DEBUG_LOG

// #define HMM_UBLAS_IMPL


namespace ducks {


typedef double prob;


template<int N, int M, class observation>
class HMM {


	///////////////////////////////////////////////////////////////////////////
	// TYPES
	///////////////////////////////////////////////////////////////////////////

public:

	typedef HMM<N, M, observation> self_t;

	typedef c_vector<prob, N> state_dist_t;
	typedef c_matrix<prob, N, N> state_state_trans_t;
	typedef c_matrix<prob, N, M> state_obs_trans_t;

	struct model_t {
		state_state_trans_t A;
		state_obs_trans_t B;
		state_dist_t pi;
	};

	///////////////////////////////////////////////////////////////////////////
	// INITIALIZATION
	///////////////////////////////////////////////////////////////////////////

public:

	HMM(std::vector<list<string>> obs_split_names = std::vector<list<string>>()):
		obs_names(obs_split_names),
		model(),
		A(model.A),
		B(model.B),
		pi(model.pi)
	{
		standardInitialization();
	}

	HMM(const model_t &model_):
		model(model_),
		A(model.A),
		B(model.B),
		pi(model.pi)
	{
	}

	HMM(const HMM& obj):
		c(obj.c),
		alpha(obj.alpha),
		beta(obj.beta),
		gamma(obj.gamma),
		bigamma(obj.bigamma),
		model(obj.model),
		A(model.A),  // This is the
		B(model.B),  // relevant
		pi(model.pi),// part !
		obs_names(obj.obs_names),
		B_split(obj.B_split),
		T(obj.T),
		obs(obj.obs) // point to the same observation list
	{
	}

	HMM& operator=(const HMM& rhs)
	{
		c = rhs.c;
		alpha = rhs.alpha;
		beta = rhs.beta;
		gamma = rhs.gamma;
		bigamma = rhs.bigamma;

		model = rhs.model;
		// dont need to copy A, B and pi !

		obs_names = rhs.obs_names;
		B_split = rhs.B_split;

		T = rhs.T;
		obs = rhs.obs; // point the same observation list

		return *this;
	}

	void standardInitialization(bool uniform = false) {
		prob nth = 1.0/N;
		prob mth = 1.0/M;

		// A & B
		for(int i = 0; i < N; ++i) {

			// A row
			for(int j = 0; j < N; ++j) {
				if (i == j && uniform == false)
					A(i,j) = 0.5;  // stay in same state with 50% prob
				else
					A(i,j) = roughly(nth/2.0);
			}
			normalize(row(A,i));

			// B row
			for(int j = 0; j < M; ++j) {
				B(i,j) = roughly(mth);
			}
			normalize(row(B,i));
		}

		// PI
		for (int i = 0; i < N; ++i) {
			if (i == 0 && uniform == false)
				pi[i] = 0.75; // give preference to this first state.
			else
				pi[i] = roughly(nth/4.0);
		}
		normalize(pi);
	}

	///////////////////////////////////////////////////////////////////////////
	// HMM - ALGORITHMS
	///////////////////////////////////////////////////////////////////////////

	void generateSequence(const model_t &model, std::vector<observation> &seq, int length) {
		seq.clear();

		int state = sample(model.pi);
		seq.push_back(sample(model.B, state));

		for(int t = 1; t < length; ++t) {
			state = sample(model.A, state);
			seq.push_back(sample(model.B, state));
		}
	}

	observation predictNextObs() {
		c_vector<prob, M> obs_dist = prod(alpha[T-1],B);
		cerr << "best observation prob: " << *std::max_element(obs_dist.begin(),obs_dist.end()) << endl;
		return observation(std::max_element(obs_dist.begin(),obs_dist.end()) - obs_dist.begin());
	}

#ifndef HMM_UBLAS_IMPL

	void alphaPass() {

		// compute alpha(0)
		c[0] = 0;
		for (int i = 0; i < N; ++i) {
			alpha[0][i] = pi[i]*B(i,(*obs)[0]);
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
					alpha[t][i] += alpha[t-1][j]*A(j,i);
				}
				alpha[t][i] *= B(i,(*obs)[t]);
				c[t] += alpha[t][i];
			}

			// scale alpha(t,i)
			c[t] = 1/c[t];
			for (int i = 0; i < N; ++i) {
				alpha[t][i] *= c[t];
			}
		}
	}

	void betaPass() {
		// let beta(T-1,i) = 1 scaled by c(T-1)
		for (int i = 0; i < N; ++i) {
			beta[T-1][i] = c[T-1];
		}

		// beta pass
		for (int t = T-2; t >= 0; --t) {
			for (int i = 0; i < N; ++i) {
				beta[t][i] = 0;
				for (int j = 0; j < N; ++j) {
					beta[t][i] += A(i,j)*B(j,(*obs)[t+1])*beta[t+1][j];
				}
				// scale beta(t,i) with same factor as alpha(t,i)
				beta[t][i] *= c[t];
			}
		}
	}

	void gammaPass() {
		for (int t = 0; t < T-1; ++t) {
			prob denom = 0;
			for (int i = 0; i < N; ++i) {
				for (int j = 0; j < N; ++j) {
					denom += alpha[t][i]*A(i,j)*B(j,(*obs)[t+1])*beta[t+1][j];
				}
			}
			for (int i = 0; i < N; ++i) {
				gamma[t][i] = 0;
				for (int j = 0; j < N; ++j) {
					bigamma[t](i,j) = (alpha[t][i]*A(i,j)*B(j,(*obs)[t+1])*beta[t+1][j]) / denom;
					gamma[t][i] += bigamma[t](i,j);
				}
			}
		}
	}

	void reestimateModel() {
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
					numer += bigamma[t](i,j);
					denom += gamma[t][i];
				}
				A(i,j) = numer/denom;
			}
		}

		// re-estimate B
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < M; ++j) {
				prob numer = 0;
				prob denom = 0;
				for (int t = 0; t < T-1; ++t) {
					if ((*obs)[t] == j) {
						numer += gamma[t][i];
					}
					denom += gamma[t][i];
				}
				B(i,j) = numer/denom;
			}
		}
	}

#endif

	prob sumLogScaleFactors() {
		prob sum = 0;
		for (int t = 0; t < T; ++t) {
			sum -= log(c[t]);
		}
		return sum;
	}

	prob logProbability(std::vector<observation>* observations) {
		int T_ = T;
		auto obs_ = obs;

		obs = observations;
		T = obs->size();

		alpha.resize(T);
		c.resize(T);

		alphaPass();
		prob p = sumLogScaleFactors();

		T = T_;
		obs = obs_;

		return p;
	}

	void learningPhase() {
		alphaPass();
		betaPass();
		gammaPass();
		reestimateModel();
	}

	void learnModel(int maxIters = 200, bool verbose = false, int fixedNumIters = 0) {
		T = obs->size();
		assert(validObservations());

#ifdef DEBUGdd
		cout << "learnModel call with T = " << T << " observations: " << endl;
		for (int t = 0; t < obs->size(); ++t) {
			cout << (*obs)[t].str() << " ";
		}
		cout << endl;
#endif

#ifdef DEBUG_LOG
		fstream logger(DEBUG_FILE, fstream::out);
#endif

		////////////////
		// 1. Initialization

		prob eps = 1e-5; // FIXME: this is arbitrary
		int iters = 0;
		prob logProb = -numeric_limits<prob>::max();
		prob oldLogProb = 0;

		resizeVectors();

		bool abort = false;
		double noise = 0;


		do
		{
			oldLogProb = logProb;
			check_timeout();

#ifdef DEBUGdd
			cout << "ITERATION " << iters << endl << endl;
#endif

			const double steepness = 5; // bigger values mean more extreme curve
			const double linearfraction = 0.4;
			const double startingnoise = 0.1;
			const double middlenoise = 0.001;
			double delay = fixedNumIters*linearfraction;
			if (iters < delay) {
				noise = ((startingnoise-middlenoise)*(delay - iters)/delay) + middlenoise;
			} else {
				noise = ((exp(steepness - ((iters-delay)/((fixedNumIters-delay)/(steepness))) ) - 1) /
						 (exp(steepness) - 1)) *
						middlenoise;

			}
			addNoiseToModel(noise);

			learningPhase();

			// Compute log[P(O|lambda)]
			logProb = sumLogScaleFactors();

#ifdef DEBUGdd
			cout << "LogProb delta:" << logProb - oldLogProb << ", " << logProb << ", " << oldLogProb << endl;
#endif

			++iters;

#ifdef DEBUG_LOG
			logger << iters << ";" << logProb << ";" << noise << endl;
#endif

			if (fixedNumIters > 0) {
				abort = iters >= fixedNumIters;
				static bool foo = true;
				if (foo && logProb - eps <= oldLogProb) {
					cout << "abort rule at: " << iters << endl;
					foo = false;
				}
			} else
				abort = iters >= maxIters || logProb - eps <= oldLogProb;
		}
		while (!abort);

#ifdef DEBUG
		if (verbose) {
			std::cerr << "learnModel ended after iteration " << iters
					  << " with logProb of " << logProb << endl;
		}
#endif

	}

	///////////////////////////////////////////////////////////////////////////
	// UTILITIES
	///////////////////////////////////////////////////////////////////////////

	bool validObservations() const {
		if (T > obs->size() || T < 0)
			return false;
		foreach(observation o, (*obs)) {
			if (o < 0 || o >= M) {
				return false;
			}
		}
		return true;
	}

	void setObservations(std::vector<observation> *obs_) {
		obs = obs_;
	}

	void addNoiseToModel(prob noise = 0.001) {
		addNoise(pi,noise);
		addNoise<N,N>(A,noise);
		addNoise<N,M>(B,noise);
	}

	void resizeVectors() {
		c.resize(T);
		alpha.resize(T);
		beta.resize(T);
		gamma.resize(T);
		bigamma.resize(T);
	}

	prob simple_distance(model_t& other) {
		return(manhatten_metric<N,N>(A, other.A) +
			   manhatten_metric<N,M>(B, other.B) +
			   manhatten_metric<N>(pi, other.pi));
	}

	prob kullback_leibler_distance_sample(self_t &other, int num_seqs = 10, int length_seqs = 4000) {
		std::vector<observation> seq;

		prob dist = 0;

		for (int i = 0; i < num_seqs; ++i) {
			generateSequence(model, seq, length_seqs);
			dist += (logProbability(seq) - other.logProbability(seq)) / length_seqs;
		}

		return dist / num_seqs;

	}

	prob distance(self_t &other) {

		prob dist = (logProbability(obs) - other.logProbability(obs));

		return dist;

	}

	///////////////////////////////////////////////////////////////////////////
	// DATA
	///////////////////////////////////////////////////////////////////////////

private:

	vector<prob> c;
	vector<state_dist_t> alpha;
	vector<state_dist_t> beta;
	vector<state_dist_t> gamma;
	vector<state_state_trans_t> bigamma;

	model_t model;

	state_state_trans_t &A;
	state_obs_trans_t &B;
	state_dist_t &pi;

	std::vector<list<string>> obs_names;
	std::vector<matrix<prob>> B_split;

	int T;
	std::vector<observation> *obs;

public:
	model_t& getModel() {
		return model;
	}


public:

	///////////////////////////////////////////////////////////////////////////
	// PRINTING AND OTHERS
	///////////////////////////////////////////////////////////////////////////

	void printState() {

		// print A
		cout << "A:" << endl;
		for (int i = 0; i < N; ++i) {
			for (int j = 0; j < N; ++j) {
				cout << A(i,j) << " ";
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
				cout << B(i,j) << " ";
			}
			cout << endl;
		}

		// print pi
		cout << endl << "pi: " << endl;
		for (int i = 0; i < N; ++i) {
			cout << pi[i] << " ";
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
						mat(i, (j/factor) % m) += B(i,j);
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

///////////////////////////////////////////////////////////////////////////////
// UBLAS IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

#ifdef HMM_UBLAS_IMPL

	void alphaPass() {
		noalias(alpha[0]) = element_prod(pi,column(B,(*obs)[0]));
		c[0] = 1 / sum(alpha[0]);
		alpha[0] *= c[0];

		for (int t = 1; t < T; ++t) {
			noalias(alpha[t]) = element_prod(prod(alpha[t-1], A), column(B,(*obs)[t]));
			c[t] = 1 / sum(alpha[t]);
			alpha[t] *= c[t];
		}
	}

	void betaPass() {
		noalias(beta[T-1]) = scalar_vector<prob>(N, c[T-1]);

		for (int t = T-2; t >= 0; --t) {
			noalias(beta[t]) = prod(A, element_prod(column(B,(*obs)[t+1]), beta[t+1])) * c[t];
		}
	}

	void gammaPass() {
		for (int t = 0; t < T-1; ++t) {

			noalias(bigamma[t]) =
				element_prod(A, outer_prod(alpha[t],
										   element_prod(column(B,(*obs)[t+1]),
												   	    beta[t+1])));
			bigamma[t] /= sum(prod(scalar_vector<prob>(N, 1.0), bigamma[t]));
			noalias(gamma[t]) = prod(bigamma[t], scalar_vector<prob>(N,1));
		}
	}

	void reestimateModel() {
		// re-estimate pi
		pi = gamma[0];

		// re-estimate A
		A.clear();
		state_dist_t gammasum;
		for (int t = 0; t < T; ++t) {
			A += bigamma[t];
			gammasum += gamma[t];
		}
		A = element_div(A, outer_prod(gammasum, scalar_vector<prob>(N,1)));

		// re-estimate B
		B.clear();
		for (int t = 0; t < T-1; ++t) {
			noalias(column(B,(*obs)[t])) += gamma[t];
		}
		B = element_div(B, outer_prod(gammasum-gamma[T-1], scalar_vector<prob>(M,1)));
	}

#endif



};


}


#endif /* HMM_H_ */
