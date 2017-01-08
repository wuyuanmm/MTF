#include "mtf/SM/NNParams.h"
#include "mtf/Utilities/miscUtils.h"
#include "mtf/SM/GNN.h"

_MTF_BEGIN_NAMESPACE

NNParams::NNParams(
	const gnn::GNNParams *_gnn,
	int _n_samples,
	int _max_iters,
	double _epsilon,
	const vectorvd &_ssm_sigma, 
	const vectorvd &_ssm_mean,
	const vectord &_pix_sigma,
	bool _additive_update, 
	int _show_samples,
	int _add_points, 
	int _n_samples_to_add,
	int _remove_points,
	bool _load_index, 
	bool _save_index, 
	string _saved_index_dir,
	bool _debug_mode) :
	gnn(_gnn),
	n_samples(_n_samples),
	max_iters(_max_iters),
	epsilon(_epsilon),
	ssm_sigma(_ssm_sigma),
	ssm_mean(_ssm_mean),
	pix_sigma(_pix_sigma),
	additive_update(_additive_update),
	show_samples(_show_samples),
	add_points(_add_points),
	n_samples_to_add(_n_samples_to_add),
	remove_points(_remove_points),
	load_index(_load_index),
	save_index(_save_index),
	saved_index_dir(_saved_index_dir),
	debug_mode(_debug_mode){}

NNParams::NNParams(const NNParams *params) :
n_samples(NN_N_SAMPLES),
max_iters(NN_MAX_ITERS),
epsilon(NN_EPSILON),
additive_update(NN_ADDITIVE_UPDATE),
show_samples(NN_SHOW_SAMPLES),
add_points(NN_ADD_POINTS),
n_samples_to_add(NN_N_SAMPLES_TO_ADD),
remove_points(NN_REMOVE_POINTS),
load_index(NN_LOAD_INDEX),
save_index(NN_SAVE_INDEX),
saved_index_dir(NN_INDEX_FILE_TEMPLATE),
debug_mode(NN_DEBUG_MODE){
	if(params){
		gnn = params->gnn;

		n_samples = params->n_samples;

		max_iters = params->max_iters;
		epsilon = params->epsilon;

		ssm_sigma = params->ssm_sigma;
		ssm_mean = params->ssm_mean;
		pix_sigma = params->pix_sigma;


		additive_update = params->additive_update;
		show_samples = params->show_samples;

		add_points = params->add_points;
		n_samples_to_add = params->n_samples_to_add;
		remove_points = params->remove_points;

		load_index = params->load_index;
		save_index = params->save_index;
		saved_index_dir = params->saved_index_dir;

		debug_mode = params->debug_mode;
	}
}


bool NNParams::processDistributions(vector<VectorXd> &state_sigma, 
	vector<VectorXd> &state_mean, VectorXi &distr_n_samples, 
	int &n_distr, int ssm_state_size){
	bool using_pix_sigma;
	if(pix_sigma.empty() || pix_sigma[0] <= 0){
		if(ssm_sigma.empty()){
			throw std::invalid_argument("Sigma must be provided for at least one sampler");
		}
		using_pix_sigma = false;
		n_distr = max(ssm_sigma.size(), ssm_mean.size());
	} else{
		using_pix_sigma = true;
		n_distr = pix_sigma.size();
		utils::printMatrix(Map<const RowVectorXd>(pix_sigma.data(), pix_sigma.size()),
			"pix_sigma");
	}
	state_sigma.resize(n_distr);
	state_mean.resize(n_distr);
	distr_n_samples.resize(n_distr);
	if(!using_pix_sigma){
		if(ssm_mean.empty()){
			//! default to zero mean
			ssm_mean.push_back(vectord(ssm_state_size, 0));
		}
		int sampler_id = 0, sigma_id = 0, mean_id = 0;
		while(sampler_id < n_distr){
			if(ssm_sigma[sigma_id].size() == 1){
				state_sigma[sampler_id].resize(ssm_state_size);
				state_sigma[sampler_id].fill(ssm_sigma[sigma_id][0]);
			} else if(ssm_sigma[sigma_id].size() < ssm_state_size){
				throw std::invalid_argument(
					cv::format("NNParams :: SSM sigma for distribution %d has invalid size: %d",
					sampler_id, ssm_sigma[sigma_id].size()));
			} else{
				state_sigma[sampler_id] = Map<const VectorXd>(ssm_sigma[sigma_id].data(), ssm_state_size);
			}
			if(ssm_mean[mean_id].size() == 1){
				state_mean[sampler_id].resize(ssm_state_size);
				state_mean[sampler_id].fill(ssm_mean[mean_id][0]);
			} else if(ssm_mean[mean_id].size() < ssm_state_size){
				throw std::invalid_argument(
					cv::format("NNParams :: SSM mean for distribution %d has invalid size: %d",
					sampler_id, ssm_mean[mean_id].size()));
			} else{
				state_mean[sampler_id] = Map<const VectorXd>(ssm_mean[mean_id].data(), ssm_state_size);
			}			
			++sampler_id;
			if(sampler_id < ssm_sigma.size()){ ++sigma_id; }
			if(sampler_id < ssm_mean.size()){ ++mean_id; }
		}
	}
	//! get the no. of samples generated by each sampler
	int samples_per_distr = n_samples / n_distr;
	for(int distr_id = 0; distr_id < n_distr; ++distr_id){
		distr_n_samples[distr_id] = samples_per_distr;
	}
	int residual_samples = n_samples - n_distr*samples_per_distr;
	if(residual_samples >= n_distr){
		throw std::logic_error(
			cv::format("NNParams :: Residual sample count: %d exceeds the no. of distributions: %d",
			residual_samples, n_distr));
	}
	//! distribute the residual samples evenly among the distributions;
	for(int distr_id = 0; distr_id < residual_samples; ++distr_id){
		++distr_n_samples[distr_id];
	}

	if(!using_pix_sigma){
		if(n_distr > 1){
			printf("Using a mixture of %d distributions:\n", n_distr);
			for(int distr_id = 0; distr_id < n_distr; ++distr_id){
				printf("%d: ", distr_id);
				utils::printMatrix(state_sigma[distr_id].transpose(), nullptr, "%e");
			}
		} else{
			printf("Using a single distribution:\n");
			utils::printMatrix(state_sigma[0].transpose(), nullptr, "%e");
		}
	}
	return using_pix_sigma;
}

_MTF_END_NAMESPACE
