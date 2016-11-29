#include "mtf/SM/GridTracker.h"
#include "mtf/SM/PyramidalTracker.h"

#include "mtf/Utilities/miscUtils.h"
#include <vector>
#include <stdexcept>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#ifdef ENABLE_TBB
#include "tbb/tbb.h" 
#endif

//! OpenMP scheduler
#ifndef GRID_OMP_SCHD
#define GRID_OMP_SCHD auto
#endif

_MTF_BEGIN_NAMESPACE

GridTrackerParams::GridTrackerParams(
int _grid_size_x, int _grid_size_y,
int _patch_size_x, int _patch_size_y,
bool _init_at_each_frame,
bool _dyn_patch_size, bool _use_tbb,
int _max_iters, double _epsilon, bool _enable_pyr,
bool _show_trackers, bool _show_tracker_edges,
bool _debug_mode) :
grid_size_x(_grid_size_x),
grid_size_y(_grid_size_y),
patch_size_x(_patch_size_x),
patch_size_y(_patch_size_y),
init_at_each_frame(_init_at_each_frame),
dyn_patch_size(_dyn_patch_size),
use_tbb(_use_tbb),
max_iters(_max_iters),
epsilon(_epsilon),
enable_pyr(_enable_pyr),
show_trackers(_show_trackers),
show_tracker_edges(_show_tracker_edges),
debug_mode(_debug_mode){
	//resx = dyn_patch_size ? grid_size_x + 1 : grid_size_x;
	//resy = dyn_patch_size ? grid_size_y + 1 : grid_size_y;
	resx = grid_size_x + 1;
	resy = grid_size_y + 1;
}

GridTrackerParams::GridTrackerParams(const GridTrackerParams *params) :
grid_size_x(GT_GRID_SIZE_X),
grid_size_y(GT_GRID_SIZE_Y),
patch_size_x(GT_PATCH_SIZE_X),
patch_size_y(GT_PATCH_SIZE_Y),
init_at_each_frame(GT_INIT_AT_EACH_FRAME),
dyn_patch_size(GT_DYN_PATCH_SIZE),
use_tbb(GT_USE_TBB),
max_iters(GT_MAX_ITERS),
epsilon(GT_EPSILON),
enable_pyr(GT_ENABLE_PYR),
show_trackers(GT_SHOW_TRACKERS),
show_tracker_edges(GT_SHOW_TRACKER_EDGES),
debug_mode(GT_DEBUG_MODE){
	if(params){
		grid_size_x = params->grid_size_x;
		grid_size_y = params->grid_size_y;
		patch_size_x = params->patch_size_x;
		patch_size_y = params->patch_size_y;
		init_at_each_frame = params->init_at_each_frame;
		dyn_patch_size = params->dyn_patch_size;
		use_tbb = params->use_tbb;
		max_iters = params->max_iters;
		epsilon = params->epsilon;
		enable_pyr = params->enable_pyr;
		show_trackers = params->show_trackers;
		show_tracker_edges = params->show_tracker_edges;
		debug_mode = params->debug_mode;
	}
	//resx = dyn_patch_size ? grid_size_x + 1 : grid_size_x;
	//resy = dyn_patch_size ? grid_size_y + 1 : grid_size_y;
	resx = grid_size_x + 1;
	resy = grid_size_y + 1;
}
template<class SSM>
GridTracker<SSM>::GridTracker(const vector<TrackerBase*> _trackers,
	const ParamType *grid_params, const EstimatorParams *_est_params,
	const SSMParams *_ssm_params) :
	GridBase(_trackers), ssm(_ssm_params),
	params(grid_params), est_params(_est_params){
	printf("\n");
	printf("Using Grid tracker with:\n");
	printf("grid_size: %d x %d\n", params.grid_size_x, params.grid_size_y);
	printf("patch_size: %d x %d\n", params.patch_size_x, params.patch_size_y);
	printf("init_at_each_frame: %d\n", params.init_at_each_frame);
	printf("use_tbb: %d\n", params.use_tbb);
	printf("max_iters: %d\n", params.max_iters);
	printf("epsilon: %f\n", params.epsilon);
	printf("enable_pyr: %d\n", params.enable_pyr);
	printf("show_trackers: %d\n", params.show_trackers);
	printf("show_tracker_edges: %d\n", params.show_tracker_edges);
	printf("debug_mode: %d\n", params.debug_mode);
	printf("\n");

	printf("Using %s estimator with:\n", ssm.name.c_str());
	est_params.print();

	name = "grid";

	int grid_size = params.grid_size_x*params.grid_size_y;
	if(grid_size != n_trackers){
		printf("No. of trackers provided: %d\n", n_trackers);
		printf("No. of trackers needed for the grid: %d\n", grid_size);
		throw std::invalid_argument("GridTracker :: Mismatch between grid dimensions and no. of trackers");
	}
	if(ssm.getResX() != params.getResX() || ssm.getResY() != params.getResY()){
		throw std::invalid_argument(
			cv::format("GridTrackerCV: SSM has invalid sampling resolution: %d x %d",
			ssm.getResX(), ssm.getResY()));
	}

	int sub_regions_x = params.grid_size_x + 1, sub_regions_y = params.grid_size_y + 1;
	_linear_idx.resize(sub_regions_y, sub_regions_x);
	for(int idy = 0; idy < sub_regions_y; idy++){
		for(int idx = 0; idx < sub_regions_x; idx++){
			_linear_idx(idy, idx) = idy * sub_regions_x + idx;
		}
	}

	if(params.dyn_patch_size){
		printf("Using dynamic patch sizes\n");
	}
	patch_corners.create(2, 4, CV_64FC1);
	cv_corners_mat.create(2, 4, CV_64FC1);
	prev_pts.resize(n_trackers);
	curr_pts.resize(n_trackers);
	ssm_update.resize(ssm.getStateSize());
	centrod_dist_x = params.patch_size_x / 2.0;
	centrod_dist_y = params.patch_size_y / 2.0;

	pix_mask.resize(n_trackers);
	std::fill(pix_mask.begin(), pix_mask.end(), 1);
	pause_seq = 0;

	//centroid_offset.resize(Eigen::NoChange, n_trackers);
	//int tracker_id = 0;
	//VectorXd x_offsets = VectorXd::LinSpaced(resx, centrod_dist_x, -centrod_dist_x);
	//VectorXd y_offsets = VectorXd::LinSpaced(resy, centrod_dist_y, -centrod_dist_y);
	//for(int row_id = 0; row_id < resy; row_id++){
	//	//double offset_y = row_id == 0 ? patch_centrod_dist
	//	//	: row_id == resy - 1 ? -patch_centrod_dist
	//	//	: 0;
	//	for(int col_id = 0; col_id < resx; col_id++){
	//		//double offset_x = col_id == 0 ? patch_centrod_dist
	//		//	: col_id == resx - 1 ? -patch_centrod_dist
	//		//	: 0;
	//		centroid_offset(0, tracker_id) = x_offsets(col_id);
	//		centroid_offset(1, tracker_id) = y_offsets(row_id);
	//		tracker_id++;
	//	}
	//}
	if(input_type == HETEROGENEOUS_INPUT){
		if(params.enable_pyr){
			throw std::invalid_argument("GridTracker::Pyramidal patch trackers with heterogeneous inputs are currently not supported");
		}
		printf("Warning: Grid tracker used with heterogeneous patch trackers\n");
	}

	if(params.show_trackers){
		patch_win_name = "Patch Trackers";
		cv::namedWindow(patch_win_name);
	}
#if defined ENABLE_TBB
	printf("Parallelization enabled using TBB\n");
#elif defined ENABLE_OMP
	printf("Parallelization enabled using OpenMP\n");
#endif
}
template<class SSM>
void GridTracker<SSM>::setImage(const cv::Mat &img) {
	if(params.enable_pyr){
		PyramidalTracker *init_pyr_tracker = dynamic_cast<PyramidalTracker*>(trackers[0]);
		if(!init_pyr_tracker){
			throw std::invalid_argument("GridTracker:: Patch tracker 0 is not a valid pyramidal tracker");
		}
		trackers[0]->setImage(img);
		for(int tracker_id = 1; tracker_id < n_trackers; ++tracker_id){
			PyramidalTracker *curr_pyr_tracker = dynamic_cast<PyramidalTracker*>(trackers[tracker_id]);
			if(!curr_pyr_tracker){
				throw std::invalid_argument(cv::format("GridTracker:: Patch tracker %d is not a valid pyramidal tracker", tracker_id));
			}
			curr_pyr_tracker->setImagePyramid(init_pyr_tracker->getImagePyramid());
		}
	} else{
		for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++){
			if(img.type() == trackers[tracker_id]->inputType()){
				trackers[tracker_id]->setImage(img);
			}
		}
	}
	curr_img = img;
	if(params.show_trackers && curr_img_uchar.empty()){
		curr_img_uchar.create(curr_img.rows, curr_img.cols, CV_8UC3);
	}
}

template<class SSM>
void GridTracker<SSM>::initialize(const cv::Mat &corners) {
	ssm.initialize(corners);
	initTrackers();
	ssm.getCorners(cv_corners_mat);
	if(params.show_trackers){ showTrackers(); }
}
template<class SSM>
void GridTracker<SSM>::update() {
#ifdef ENABLE_TBB
	parallel_for(tbb::blocked_range<size_t>(0, n_trackers),
		[&](const tbb::blocked_range<size_t>& r){
		for(size_t tracker_id = r.begin(); tracker_id != r.end(); ++tracker_id){
#else
#pragma omp parallel for schedule(GRID_OMP_SCHD)
	for(int tracker_id = 0; tracker_id < n_trackers; ++tracker_id){
#endif		
		trackers[tracker_id]->update();
		utils::getCentroid(curr_pts[tracker_id], trackers[tracker_id]->getRegion());
		//patch_corners = trackers[tracker_id]->getRegion();
		
	}
#ifdef ENABLE_TBB
		});
#endif
		ssm.estimateWarpFromPts(ssm_update, pix_mask, prev_pts, curr_pts, est_params);
		Matrix24d opt_warped_corners;
		ssm.applyWarpToCorners(opt_warped_corners, ssm.getCorners(), ssm_update);
		ssm.setCorners(opt_warped_corners);

		if(params.init_at_each_frame){
			initTrackers();
		} else {
			for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++){
				prev_pts[tracker_id].x = curr_pts[tracker_id].x;
				prev_pts[tracker_id].y = curr_pts[tracker_id].y;
			}
		}
		ssm.getCorners(cv_corners_mat);
		if(params.show_trackers){ showTrackers(); }
}
template<class SSM>
void GridTracker<SSM>::setRegion(const cv::Mat& corners) {
	ssm.setCorners(corners);
	if(params.init_at_each_frame){
		initTrackers();
	} else {
		if(params.dyn_patch_size){
			for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++){
				int row_id = tracker_id / params.grid_size_x;
				int col_id = tracker_id % params.grid_size_x;
				prev_pts[tracker_id].x = (ssm.getPts()(0, _linear_idx(row_id, col_id)) + ssm.getPts()(0, _linear_idx(row_id, col_id + 1))
					+ ssm.getPts()(0, _linear_idx(row_id + 1, col_id + 1)) + ssm.getPts()(0, _linear_idx(row_id + 1, col_id))) / 4.0;
				prev_pts[tracker_id].y = (ssm.getPts()(1, _linear_idx(row_id, col_id)) + ssm.getPts()(1, _linear_idx(row_id, col_id + 1))
					+ ssm.getPts()(1, _linear_idx(row_id + 1, col_id + 1)) + ssm.getPts()(1, _linear_idx(row_id + 1, col_id))) / 4.0;
			}
		} else{
			for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++){
				prev_pts[tracker_id].x = ssm.getPts()(0, tracker_id);
				prev_pts[tracker_id].y = ssm.getPts()(1, tracker_id);
			}
		}
	}
	ssm.getCorners(cv_corners_mat);
}

template<class SSM>
void GridTracker<SSM>::initTrackers(){
#ifdef ENABLE_TBB
	parallel_for(tbb::blocked_range<size_t>(0, n_trackers),
		[&](const tbb::blocked_range<size_t>& r){
		for(size_t tracker_id = r.begin(); tracker_id != r.end(); ++tracker_id){
#else
#pragma omp parallel for schedule(GRID_OMP_SCHD)
	for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++){

#endif
		int row_id = tracker_id / params.grid_size_x;
		int col_id = tracker_id % params.grid_size_x;

		patch_corners.at<double>(0, 0) = ssm.getPts()(0, _linear_idx(row_id, col_id));
		patch_corners.at<double>(1, 0) = ssm.getPts()(1, _linear_idx(row_id, col_id));

		patch_corners.at<double>(0, 1) = ssm.getPts()(0, _linear_idx(row_id, col_id + 1));
		patch_corners.at<double>(1, 1) = ssm.getPts()(1, _linear_idx(row_id, col_id + 1));

		patch_corners.at<double>(0, 2) = ssm.getPts()(0, _linear_idx(row_id + 1, col_id + 1));
		patch_corners.at<double>(1, 2) = ssm.getPts()(1, _linear_idx(row_id + 1, col_id + 1));

		patch_corners.at<double>(0, 3) = ssm.getPts()(0, _linear_idx(row_id + 1, col_id));
		patch_corners.at<double>(1, 3) = ssm.getPts()(1, _linear_idx(row_id + 1, col_id));

		if(params.dyn_patch_size){
			trackers[tracker_id]->initialize(patch_corners);
			//patch_corners = trackers[tracker_id]->getRegion();
			utils::getCentroid(prev_pts[tracker_id], trackers[tracker_id]->getRegion());
		} else{
			Vector2d patch_centroid = ssm.getPts().col(tracker_id);
			patch_centroid(0) = (patch_corners.at<double>(0, 0) + patch_corners.at<double>(0, 1)
				+ patch_corners.at<double>(0, 2) + patch_corners.at<double>(0, 3)) / 4.0;
			patch_centroid(1) = (patch_corners.at<double>(1, 0) + patch_corners.at<double>(1, 1)
				+ patch_corners.at<double>(1, 2) + patch_corners.at<double>(1, 3)) / 4.0;
			double min_x = patch_centroid(0) - centrod_dist_x;
			double max_x = patch_centroid(0) + centrod_dist_x;
			double min_y = patch_centroid(1) - centrod_dist_y;
			double max_y = patch_centroid(1) + centrod_dist_y;

			patch_corners.at<double>(0, 0) = patch_corners.at<double>(0, 3) = min_x;
			patch_corners.at<double>(0, 1) = patch_corners.at<double>(0, 2) = max_x;
			patch_corners.at<double>(1, 0) = patch_corners.at<double>(1, 1) = min_y;
			patch_corners.at<double>(1, 2) = patch_corners.at<double>(1, 3) = max_y;

			trackers[tracker_id]->initialize(patch_corners);

			prev_pts[tracker_id].x = patch_centroid(0);
			prev_pts[tracker_id].y = patch_centroid(1);
		}
	}
#ifdef ENABLE_TBB
	});
#endif		
}

template<class SSM>
void GridTracker<SSM>::showTrackers(){
	curr_img.convertTo(curr_img_uchar, curr_img_uchar.type());
	cv::cvtColor(curr_img_uchar, curr_img_uchar, CV_GRAY2BGR);
	utils::drawRegion(curr_img_uchar, cv_corners_mat, CV_RGB(0, 0, 255), 2);
	for(int tracker_id = 0; tracker_id < n_trackers; tracker_id++) {
		cv::Scalar tracker_color = pix_mask[tracker_id] ? CV_RGB(0, 255, 0) : CV_RGB(255, 0, 0);
		circle(curr_img_uchar, curr_pts[tracker_id], 2, tracker_color, 2);
		if(params.show_tracker_edges){
			utils::drawRegion(curr_img_uchar, trackers[tracker_id]->getRegion(), tracker_color, 1);
		}
	}
	imshow(patch_win_name, curr_img_uchar);
	//int key = cv::waitKey(1 - pause_seq);
	//if(key == 32){
	//	pause_seq = 1 - pause_seq;
	//}
}

_MTF_END_NAMESPACE

#ifndef HEADER_ONLY_MODE
#include "mtf/Macros/register.h"
_REGISTER_TRACKERS_SSM(GridTracker);
#endif