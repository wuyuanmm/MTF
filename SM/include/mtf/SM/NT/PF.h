#ifndef MTF_PF_NT_H
#define MTF_PF_NT_H

#include "SearchMethod.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>
#include "mtf/SM/PFParams.h"

_MTF_BEGIN_NAMESPACE
namespace nt{

	//! Particle Filter
	class PF : public SearchMethod{
	public:
		typedef PFParams ParamType;
		typedef ParamType::DynamicModel DynamicModel;
		typedef ParamType::UpdateType UpdateType;
		typedef ParamType::LikelihoodFunc LikelihoodFunc;
		typedef ParamType::ResamplingType ResamplingType;
		typedef ParamType::MeanType MeanType;

		typedef boost::minstd_rand RandGenT;
		typedef boost::normal_distribution<double> MeasureDistT;
		typedef boost::random::discrete_distribution<unsigned int> DistrDistT;
		//typedef boost::variate_generator<RandGenT&, MeasureDistT> MeasureGenT;
		typedef boost::random::uniform_real_distribution<double> ResampleDistT;
		//typedef boost::variate_generator<RandGenT&, ResampleDistT> ResampleGenT;
		typedef ResampleDistT::param_type ResampleDistParamT;

		using SearchMethod::am;
		using SearchMethod::ssm;
		using SearchMethod::cv_corners_mat;
		using SearchMethod::name;
		using SearchMethod::initialize;
		using SearchMethod::update;

		PF(AM _am, SSM _ssm, const ParamType *pf_params = nullptr);
		~PF(){}

		void initialize(const cv::Mat &corners) override;
		void update() override;
		void setRegion(const cv::Mat& corners) override;

	private:
		ParamType params;

		//! similarity of the initial patch (or template) with itself
		double max_similarity;

		//! Gaussian random number generator for measurement function
		RandGenT measurement_gen;
		MeasureDistT measurement_dist;
		//! Gaussian random number generator for resampling particles
		RandGenT resample_gen;
		ResampleDistT resample_dist;

		RandGenT distr_id_gen;
		DistrDistT distr_id_dist;

		int frame_id;

		Matrix3d warp_update;

		CornersT mean_corners;
		CornersT prev_corners;

		int ssm_state_size;

		int n_distr;
		VectorXd mean_state;
		/**
		SSM states for all particles.
		2 sets of particles are stored for efficient resampling
		*/
		std::vector<VectorXd> particle_states[2];
		//! Update history for Auto Regression
		std::vector<VectorXd> particle_ar[2];
		//! ID of the distribution used for generating each particle
		VectorXd particle_distr[2];
		//! weights of the different Gaussian distributions
		vector<double> distr_wts;
		//! no. of particles generated by each distribution
		VectorXi distr_n_particles;

		//! minimum particles alloted to each distribution
		int min_particles;
		int dynamic_particles;
		//! ID of the particle with the maximum weight
		int max_wt_id;

		int curr_set_id;
		VectorXd particle_wts;
		VectorXd particle_cum_wts;

		VectorXd perturbed_state;
		VectorXd perturbed_ar;

		vector<VectorXd> state_sigma, state_mean;
		VectorXi resample_ids, resample_ids_hist;
		VectorXd uniform_rand_nums;

		double measurement_likelihood;
		double measurement_factor;
		cv::Mat curr_img_uchar;

		bool using_pix_sigma;

		//! 1 x S Jacobian of the appearance model w.r.t. SSM state vector
		RowVectorXd df_dp;
		//! N x S jacobians of the pix values w.r.t the SSM state vector 
		MatrixXd dI_dp;
		MatrixXd d2f_dp2;

		char *log_fname, *time_fname;
		char *wts_fname, *cum_wts_fname;
		char *state_fname, *corners_fname;

		void linearMultinomialResampling();
		void binaryMultinomialResampling();
		void residualResampling();
		void initializeParticles();
		void initializeDistributions();
		void updateMeanCorners();
	};
}

_MTF_END_NAMESPACE

#endif
