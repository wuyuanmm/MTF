#ifndef MTF_NN_H
#define MTF_NN_H

#include "SearchMethod.h"
#include "FGNN.h"
#include "NNParams.h"
#include "FLANNParams.h"

#include <flann/flann.hpp>
#include <memory>

_MTF_BEGIN_NAMESPACE

template<class AM, class SSM>
class NN : public SearchMethod < AM, SSM > {
	init_profiling();
public:
	typedef flann::Matrix<double> flannMatT;
	typedef flann::Matrix<int> flannResultT;
	typedef unique_ptr<flannMatT> flannMatT_;

	//typedef flann::Index<flann::L2<double> > flannIdxT;
	typedef flann::Index<AM> FLANN;
	typedef unique_ptr<FLANN> FLANN_;
	typedef gnn::FGNN<AM> FGNN;
	typedef unique_ptr<FGNN> FGNN_;


	typedef NNParams ParamType;
	typedef FLANNParams::IdxType IdxType;
	typedef FLANNParams::SearchType SearchType;

	using SearchMethod<AM, SSM> ::am;
	using SearchMethod<AM, SSM> ::ssm;
	using typename SearchMethod<AM, SSM> ::AMParams;
	using typename SearchMethod<AM, SSM> ::SSMParams;
	using SearchMethod<AM, SSM> ::cv_corners_mat;
	using SearchMethod<AM, SSM> ::name;
	using SearchMethod<AM, SSM> ::initialize;
	using SearchMethod<AM, SSM> ::update;	

	NN(const ParamType *nn_params = nullptr, 
		const FLANNParams *flann_params = nullptr,
		const AMParams *am_params = nullptr,
		const SSMParams *ssm_params = nullptr);
	~NN();

	void initialize(const cv::Mat &corners) override;
	void update() override;

private:

	ParamType params;
	FLANNParams flann_params;
	FLANN_ flann_index;
	FGNN_ gnn_index;

	flannMatT_ flann_dataset;

	int frame_id;
	int am_dist_size;
	int ssm_state_size;

	//! no. of Gaussian distributions used for generating samples
	int n_distr;
	//! no. of samples generated by each distribution
	VectorXi distr_n_samples;

	bool using_pix_sigma;
	vector<VectorXd> state_sigma, state_mean;

	Matrix24d prev_corners;
	VectorXd inv_state_update;

	vector<VectorXd> ssm_perturbations;

	MatrixXdr eig_dataset;
	VectorXd eig_query;
	VectorXi eig_result;
	VectorXd eig_dists;

	MatrixXdr eig_dataset_added;
	vector<VectorXd> ssm_perturbations_added;
	VectorXi distr_n_samples_added;


	int best_idx;
	double best_dist;

	string saved_db_path, saved_idx_path;
	cv::Mat curr_img_uchar;
	cv::Point2d curr_corners[4];

	bool dataset_loaded;

	char *log_fname;
	char *time_fname;

	void generateDataset(vector<VectorXd> &perturbations, 
		MatrixXdr &dataset, const VectorXi &_distr_n_samples);
	void loadDataset();
	void saveDataset();
};

_MTF_END_NAMESPACE

#endif

