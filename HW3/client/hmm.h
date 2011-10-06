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
#include <utility>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>
#include "storage_adaptors.hpp"

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
using std::pair;


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
	typedef c_vector<prob, M> obs_dist_t;
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
		model(),
		obs_names(obs_split_names),
		A(model.A),
		B(model.B),
		pi(model.pi),
		mReinitCount(0)
	{
		hardcodedInitialization();
//		standardInitialization();
	}

	HMM(const model_t &model_):
		model(model_),
		A(model.A),
		B(model.B),
		pi(model.pi),
		mReinitCount(0)
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
		obs(obj.obs), // point to the same observation list
		mReinitCount(obj.mReinitCount)
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

	void hardcodedInitialization() {
		double init_A[3][3] = {
			0.80, 0.05, 0.15,
			0.05, 0.70, 0.25,
			0.09, 0.01, 0.90
		};

		double init_B[3][9] = {
			0.13, 0.11, 0.11, 0.11, 0.13, 0.11, 0.12, 0.11, 0.09,
			0.09, 0.10, 0.13, 0.08, 0.12, 0.09, 0.13, 0.14, 0.12,
			0.09, 0.12, 0.09, 0.13, 0.10, 0.11, 0.11, 0.13, 0.12
		};

		A = make_matrix_from_pointer(init_A);
		B = make_matrix_from_pointer(init_B);

		pi[0] = 0.7;
		pi[1] = 0.2;
		pi[2] = 0.1;

		if(! (is_row_stochastic(A) && is_row_stochastic(B) && is_stochastic(pi)))
			throw "You idiot. Learn to count...";
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

		if(! (is_row_stochastic(A) && is_row_stochastic(B) && is_stochastic(pi)))
			throw "Messed it up...";
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

	state_dist_t distNextState() {
		return prod(alpha[T-1],A);
	}

	obs_dist_t distNextObs() {
		return prod(distNextState(),B);
	}

	observation predictNextObs() {
		auto obs_dist = distNextObs();
		int result = observation(std::max_element(obs_dist.begin(),obs_dist.end()) - obs_dist.begin());
		return result;
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

	prob directLogProb(std::vector<observation> *observations) {
		// locally overwrite these variable
		prob T = obs->size();
		state_dist_t pi = uniform_state_dist();
		auto obs = observations;

		prob logalpha;
		state_dist_t last, curr;

		for (int i = 0; i < N; ++i) {
			curr[i] = elnproduct(eln(pi[i]),eln(B(i,(*obs)[0])));
		}
		for (int t = 1; t < T; ++t) {
			last = curr;
			for (int j = 0; j < N; ++j) {
				logalpha = LOGZERO;
				for (int i = 0; i < N; ++i) {
					logalpha = elnsum(logalpha, elnproduct(last[i], eln(A(i,j))));
				}
				curr[j] = elnproduct(logalpha, eln(B(j,(*obs)[t])));
			}
		}

		logalpha = LOGZERO;
		for (int i = 0; i < N; ++i) {
			logalpha = elnsum(logalpha, curr[i]);
		}

		return logalpha;
	}


	prob logProbability(std::vector<observation>* observations) {
		int T_ = T;
		auto obs_ = obs;
		auto pi_ = pi;

		obs = observations;
		T = obs->size();

		pi = uniform_state_dist(); // when comparing models of idfferent birds, we have the problem that birds of the same species start in different states.

		alpha.resize(T);
		c.resize(T);

		alphaPass();
		prob p = sumLogScaleFactors();

		T = T_;
		obs = obs_;
		pi = pi_;

		return p;
	}

	void learningPhase() {
		alphaPass();
		betaPass();
		gammaPass();
		reestimateModel();
	}

	void learnModel(int minIters, int maxIters, bool practice_mode, bool verbose = false) {
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
		// Initialization
		const int itersToWaitForBestModel = 4;
		//const prob eps = 1e-5; // FIXME: this is arbitrary
		int iters = 0;
		prob noise = 0;
		bool abort = false;
		prob logProb = -numeric_limits<prob>::max();
		prob oldLogProb = 0;

		prob best_log_prob = -numeric_limits<prob>::max();
		model_t best_model;
		int iter_best_model = -1;

		resizeVectors();

		do
		{
			oldLogProb = logProb;
			check_timeout();

#ifdef DEBUGdd
			cout << "ITERATION " << iters << endl << endl;
#endif

			if (model_has_nan()) {
				noise = 0;
				hardcodedInitialization();
				++mReinitCount;
				// cout << "REINIT!" << endl;
			} else {
				noise = calc_noise(iters, maxIters);
				addNoiseToModel(noise);
			}

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

			// compute best model, not last one found
			if (logProb > best_log_prob) {
				best_log_prob = logProb;
				best_model = model;
				iter_best_model = iters;
			}

			if (practice_mode) {
				abort = iters >= maxIters;
#ifdef DEBUG
				static bool foo = true;
				if (foo && iters-iter_best_model >= itersToWaitForBestModel) {
					cout << "Abort rule would have ended at iteration: " << iters << endl;
					foo = false;
				}
#endif
			} else {
				abort = iters >= maxIters ||
						(iters >= minIters && iters-iter_best_model >= itersToWaitForBestModel);
			}
		}
		while (!abort);

		model = best_model;

#ifdef DEBUG
		if (verbose) {
			std::cerr << "learnModel ended after iteration " << iters
					  << "    with logProb of " << best_log_prob << endl;
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

	prob calc_noise (int iters, int maxNumIters) {
		const double steepness = 5; // bigger values mean more extreme curve
		const double linearfraction = 0.1; // which part of the total iterations should be linear decrease of nosie, before we start exponential
		const double startingnoise = 0.5; // what noise should we start with
		const double middlenoise = 0.1; // what noise should we have reached when we switch form linear to exponential
		double delay = maxNumIters*linearfraction;
		if (iters < delay) {
			return ((startingnoise-middlenoise)*(delay - iters)/delay) + middlenoise;
		} else {
			return ((exp(steepness - ((iters-delay)/((maxNumIters-delay)/(steepness))) ) - 1) /
					 (exp(steepness) - 1)) *
					middlenoise;

		}
	}

	void addNoiseToModel(prob noise = 0.001) {
		addNoise(pi,noise);
		addNoise<N,N>(A,noise);
		addNoise<N,M>(B,noise);
	}

	bool model_has_nan() {
		return vec_has_nan(pi) || mat_has_nan(A) || mat_has_nan(B);
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
			dist += (directLogProb(&seq) - other.directLogProb(&seq)) / length_seqs;
		}

		return dist / num_seqs;

	}

	prob distance(self_t &other) {

		prob dist = (directLogProb(obs) - other.directLogProb(obs));

		if (islogzero(dist) ) {
			return 1000;
		}

		return dist;

	}

	state_dist_t uniform_state_dist() {
		state_dist_t dist;
		for (int i = 0; i < N; ++i) {
			dist[i] = 1.0/N;
		}
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

	int mReinitCount;

public:
	model_t& getModel() {
		return model;
	}

	int getT() {
		return T;
	}

	const std::vector<matrix<prob>> * getBSplit() {
		return &B_split;
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
			cout << setw(7) << observation((int)j).str() << " ";
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

	std::vector<std::vector<prob>> split_obs_dist(obs_dist_t obs_dist) {
		if (obs_names.empty())
			throw "No obs name woot";

		int total = 1;
		foreach (list<string> x, obs_names) {
			total *= x.size() - 1;
		}
		assert (total == M);

		std::vector<std::vector<prob>> dists;

		int factor = 1;
		foreach (list<string> x, obs_names) {
			int m = x.size() - 1; // first one is name of group
			std::vector<prob> dist(m,0);
			for (int i = 0; i < M; ++i) {
				dist[(i/factor) % m] += obs_dist(i);
			}
			factor *= m;
			dists.push_back(dist);
		}
		return dists;
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

	void print_warnings() {
		if (mReinitCount > 0) {
			cout << "[warning] Reinit count: " << mReinitCount << endl;
		}
	}

///////////////////////////////////////////////////////////////////////////////
// UBLAS IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////

// this is much nicer code, but it seems to have numerical problems...

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
