#include "mtf/SM/GridTrackerCV.h"
#include "mtf/Utilities/miscUtils.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdexcept>

_MTF_BEGIN_NAMESPACE


GridTrackerCVParams::GridTrackerCVParams(
int _grid_size_x, int _grid_size_y,
int _search_window_x, int _search_window_y,
int _pyramid_levels, bool _use_min_eig_vals,
double _min_eig_thresh, int _max_iters, 
double _epsilon, bool _show_trackers, 
bool _debug_mode) :
grid_size_x(_grid_size_x),
grid_size_y(_grid_size_y),
search_window_x(_search_window_x),
search_window_y(_search_window_y),
pyramid_levels(_pyramid_levels),
use_min_eig_vals(_use_min_eig_vals),
min_eig_thresh(_min_eig_thresh),
max_iters(_max_iters),
epsilon(_epsilon),
show_trackers(_show_trackers),
debug_mode(_debug_mode){
	resx = grid_size_x;
	resy = grid_size_y;
}
GridTrackerCVParams::GridTrackerCVParams(const GridTrackerCVParams *params) :
grid_size_x(GTCV_GRID_SIZE_X),
grid_size_y(GTCV_GRID_SIZE_Y),
search_window_x(GTCV_SEARCH_WINDOW_X),
search_window_y(GTCV_SEARCH_WINDOW_Y),
pyramid_levels(GTCV_PYRAMID_LEVELS),
use_min_eig_vals(GTCV_USE_MIN_EIG_VALS),
min_eig_thresh(GTCV_MIN_EIG_THRESH),
max_iters(GTCV_MAX_ITERS), epsilon(GTCV_EPSILON),
show_trackers(GTCV_SHOW_TRACKERS),
debug_mode(GTCV_DEBUG_MODE){
	if(params){
		grid_size_x = params->grid_size_x;
		grid_size_y = params->grid_size_y;
		search_window_x = params->search_window_x;
		search_window_y = params->search_window_y;
		pyramid_levels = params->pyramid_levels;
		use_min_eig_vals = params->use_min_eig_vals;
		min_eig_thresh = params->min_eig_thresh;
		max_iters = params->max_iters;
		epsilon = params->epsilon;
		show_trackers = params->show_trackers;
		debug_mode = params->debug_mode;
	}
	resx = grid_size_x;
	resy = grid_size_y;
}

template<class SSM>
GridTrackerCV<SSM>::GridTrackerCV(const ParamType *grid_params,
	const EstimatorParams *_est_params, const SSMParams *_ssm_params) :
	GridBase(), ssm(_ssm_params), params(grid_params), est_params(_est_params){
	printf("\n");
	printf("Using OpenCV Grid tracker with:\n");
	printf("grid_size: %d x %d\n", params.grid_size_x, params.grid_size_y);
	printf("search window size: %d x %d\n", params.search_window_x, params.search_window_y);
	printf("pyramid_levels: %d\n", params.pyramid_levels);
	printf("use_min_eig_vals: %d\n", params.use_min_eig_vals);
	printf("min_eig_thresh: %f\n", params.min_eig_thresh);
	printf("max_iters: %d\n", params.max_iters);
	printf("show_trackers: %d\n", params.show_trackers);
	printf("debug_mode: %d\n", params.debug_mode);
	printf("\n");

	printf("Using %s estimator with:\n", ssm.name.c_str());
	est_params.print();

	name = "grid_cv";

	if(ssm.getResX() != params.getResX() || ssm.getResY() != params.getResY()){
		throw std::invalid_argument(
			cv::format("GridTrackerCV: SSM has invalid sampling resolution: %d x %d",
			ssm.getResX(), ssm.getResY()));
	}

	n_pts = params.grid_size_x *params.grid_size_y;
	search_window = cv::Size(params.search_window_x, params.search_window_x);
	lk_termination_criteria = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS,
		params.max_iters, params.epsilon);

	patch_corners.create(2, 4, CV_64FC1);
	cv_corners_mat.create(2, 4, CV_64FC1);

	prev_pts_mat.create(n_pts, 2, CV_32FC1);
	curr_pts_mat.create(n_pts, 2, CV_32FC1);

	prev_pts.resize(n_pts);
	curr_pts.resize(n_pts);

	ssm_update.resize(ssm.getStateSize());
	lk_status.resize(n_pts);
	pix_mask.resize(n_pts);
	std::fill(pix_mask.begin(), pix_mask.end(), 1);
	pause_seq = 0;

	if(params.use_min_eig_vals){
		lk_flags = cv::OPTFLOW_LK_GET_MIN_EIGENVALS;
	} else{
		lk_flags = 0;
	}

	if(params.show_trackers){
		patch_win_name = "Patch Trackers";
		cv::namedWindow(patch_win_name);
	}
}

template<class SSM>
void GridTrackerCV<SSM>::initialize(const cv::Mat &corners) {
	curr_img_float.convertTo(curr_img, curr_img.type());

	ssm.initialize(corners);

	for(int pt_id = 0; pt_id < n_pts; pt_id++){
		Vector2d patch_centroid = ssm.getPts().col(pt_id);
		prev_pts[pt_id].x = patch_centroid(0);
		prev_pts[pt_id].y = patch_centroid(1);
	}
	curr_img.copyTo(prev_img);

	ssm.getCorners(cv_corners_mat);
	if(params.show_trackers){ showTrackers(); }
}
template<class SSM>
void GridTrackerCV<SSM>::update() {
	curr_img_float.convertTo(curr_img, curr_img.type());
	cv::calcOpticalFlowPyrLK(prev_img, curr_img,
		prev_pts, curr_pts, lk_status, lk_error,
		search_window,
		params.pyramid_levels, lk_termination_criteria,
		lk_flags, params.min_eig_thresh);

	ssm.estimateWarpFromPts(ssm_update, pix_mask, prev_pts, curr_pts, est_params);

	Matrix24d opt_warped_corners;
	ssm.applyWarpToCorners(opt_warped_corners, ssm.getCorners(), ssm_update);
	ssm.setCorners(opt_warped_corners);

	for(int pt_id = 0; pt_id < n_pts; pt_id++){
		prev_pts[pt_id].x = curr_pts[pt_id].x;
		prev_pts[pt_id].y = curr_pts[pt_id].y;
	}
	ssm.getCorners(cv_corners_mat);
	curr_img.copyTo(prev_img);

	if(params.show_trackers){ showTrackers(); }
}
template<class SSM>
void GridTrackerCV<SSM>::setImage(const cv::Mat &img){
	if(curr_img_float.empty()){
		prev_img.create(img.rows, img.cols, CV_8UC1);
		curr_img.create(img.rows, img.cols, CV_8UC1);
	}
	if(params.show_trackers && curr_img_uchar.empty()){
		curr_img_uchar.create(img.rows, img.cols, CV_8UC3);
	}
	curr_img_float = img;
}

template<class SSM>
void GridTrackerCV<SSM>::setRegion(const cv::Mat& corners) {
	ssm.setCorners(corners);
	for(int pt_id = 0; pt_id < n_pts; pt_id++){
		prev_pts[pt_id].x = ssm.getPts()(0, pt_id);
		prev_pts[pt_id].y = ssm.getPts()(1, pt_id);
	}
	ssm.getCorners(cv_corners_mat);
}

template<class SSM>
void GridTrackerCV<SSM>::showTrackers(){
	curr_img_float.convertTo(curr_img_uchar, curr_img_uchar.type());
	cv::cvtColor(curr_img_uchar, curr_img_uchar, CV_GRAY2BGR);
	utils::drawRegion(curr_img_uchar, cv_corners_mat, CV_RGB(0, 0, 255), 2);
	for(int tracker_id = 0; tracker_id < n_pts; tracker_id++) {
		cv::Scalar tracker_color;
		if(pix_mask[tracker_id]){
			tracker_color = cv::Scalar(0, 255, 0);
		} else{
			tracker_color = cv::Scalar(0, 0, 255);
		}
		circle(curr_img_uchar, curr_pts[tracker_id], 2, tracker_color, 2);
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
_REGISTER_TRACKERS_SSM(GridTrackerCV);
#endif