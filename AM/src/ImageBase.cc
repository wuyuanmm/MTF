#include "mtf/AM/ImageBase.h"
#include "mtf/Utilities/imgUtils.h"

_MTF_BEGIN_NAMESPACE

ImgParams::ImgParams(int _resx, int _resy,
double _grad_eps, double _hess_eps) :
resx(_resx), resy(_resy),
grad_eps(_grad_eps), hess_eps(_hess_eps){}

ImgParams::ImgParams(const ImgParams *img_params) :
resx(MTF_RES), resy(MTF_RES),
grad_eps(GRAD_EPS), hess_eps(HESS_EPS){
	if(img_params){
		resx = img_params->resx;
		resy = img_params->resy;
		grad_eps = img_params->grad_eps;
		hess_eps = img_params->hess_eps;
	}
}

ImageBase::ImageBase(const ImgParams *img_params, const int _n_channels) :
curr_img(nullptr, 0, 0), img_height(0), img_width(0),
resx(MTF_RES), resy(MTF_RES), n_pix(MTF_RES*MTF_RES), n_channels(_n_channels),
pix_norm_add(0.0), pix_norm_mult(1.0), frame_count(0),
grad_eps(GRAD_EPS), hess_eps(HESS_EPS) {
	if(img_params) {
		if(img_params->resx <= 0 || img_params->resy <= 0) {
			throw std::invalid_argument("ImageBase::Invalid sampling resolution provided");
		}
		resx = img_params->resx;
		resy = img_params->resy;
		n_pix = resx*resy;	
		grad_eps = img_params->grad_eps;
		hess_eps = img_params->hess_eps;
	}
	patch_size = n_pix*n_channels;
}

void ImageBase::setCurrImg(const cv::Mat &cv_img){
	assert(cv_img.type() == inputType());
	img_height = cv_img.rows;
	img_width = cv_img.cols;
	curr_img_cv = cv_img;
	if(n_channels == 1){
		// single channel image can share data with an Eigen matrix
		// not really necessary but remains as a relic from the past
		new (&curr_img) EigImgT((EigPixT*)(cv_img.data), img_height, img_width);
	}
}

void ImageBase::initializePixVals(const Matrix2Xd& init_pts){
	assert(init_pts.cols() == n_pix);
	if(!isInitialized()->pix_vals){
		I0.resize(patch_size);
		It.resize(patch_size);
#if !defined DEFAULT_PIX_INTERP_TYPE
		printf("Using pixel interpolation type: %s\n", utils::toString(PIX_INTERP_TYPE));
#endif
#if !defined DEFAULT_PIX_BORDER_TYPE
		printf("Using pixel border type: %s\n", utils::toString(PIX_BORDER_TYPE));
#endif
	}
	++frame_count;
	switch(n_channels){
	case 1:
		utils::getPixVals(I0, curr_img, init_pts, n_pix,
			img_height, img_width, pix_norm_mult, pix_norm_add);
		break;
	case 3:
		utils::getPixVals(I0, curr_img_cv, init_pts, n_pix,
			img_height, img_width, pix_norm_mult, pix_norm_add);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
	if(!isInitialized()->pix_vals){
		It = I0;
		isInitialized()->pix_vals = true;
	}
}

void ImageBase::initializePixGrad(const Matrix2Xd &init_pts){
	assert(init_pts.cols() == n_pix);

	if(!isInitialized()->pix_grad){
		dI0_dx.resize(patch_size, Eigen::NoChange);
		dIt_dx.resize(patch_size, Eigen::NoChange);
	}
	switch(n_channels){
	case 1:
		utils::getImgGrad(dI0_dx, curr_img, init_pts, grad_eps, n_pix,
			img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getImgGrad(dI0_dx, curr_img_cv, init_pts, grad_eps, n_pix,
			img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
	if(!isInitialized()->pix_grad){
		setCurrPixGrad(getInitPixGrad());
		isInitialized()->pix_grad = true;
	}
}

void ImageBase::initializePixGrad(const Matrix8Xd &warped_offset_pts){
	assert(warped_offset_pts.cols() == n_pix);
	if(!isInitialized()->pix_grad){
#if !defined DEFAULT_GRAD_INTERP_TYPE
		printf("Using gradient interpolation type: %s\n",
			utils::toString(GRAD_INTERP_TYPE));
#endif
		dI0_dx.resize(patch_size, Eigen::NoChange);
		dIt_dx.resize(patch_size, Eigen::NoChange);
	}
	switch(n_channels){
	case 1:
		utils::getWarpedImgGrad(dI0_dx,
			curr_img, warped_offset_pts, grad_eps, n_pix,
			img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getWarpedImgGrad(dI0_dx,
			curr_img_cv, warped_offset_pts, grad_eps, n_pix,
			img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
	if(!isInitialized()->pix_grad){
		setCurrPixGrad(getInitPixGrad());
		isInitialized()->pix_grad = true;
	}
}

void ImageBase::initializePixHess(const Matrix2Xd& init_pts,
	const Matrix16Xd &warped_offset_pts){
	assert(init_pts.cols() == n_pix && warped_offset_pts.cols() == n_pix);
	if(!isInitialized()->pix_hess){
		d2I0_dx2.resize(Eigen::NoChange, patch_size);
		d2It_dx2.resize(Eigen::NoChange, patch_size);
	}
	switch(n_channels){
	case 1:
		utils::getWarpedImgHess(d2I0_dx2, curr_img, init_pts, warped_offset_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getWarpedImgHess(d2I0_dx2, curr_img_cv, init_pts, warped_offset_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
	if(!isInitialized()->pix_hess){
		setCurrPixHess(getInitPixHess());
		isInitialized()->pix_hess = true;
	}
}
void ImageBase::initializePixHess(const Matrix2Xd &init_pts){
	assert(init_pts.cols() == n_pix);
	if(!isInitialized()->pix_hess){
#if !defined DEFAULT_HESS_INTERP_TYPE
		printf("Using Hessian interpolation type: %s\n",
			utils::toString(HESS_INTERP_TYPE));
#endif
		d2I0_dx2.resize(Eigen::NoChange, patch_size);
		d2It_dx2.resize(Eigen::NoChange, patch_size);
	}
	switch(n_channels){
	case 1:
		utils::getImgHess(d2I0_dx2, curr_img, init_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getImgHess(d2I0_dx2, curr_img_cv, init_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
	if(!isInitialized()->pix_hess){
		setCurrPixHess(getInitPixHess());
		isInitialized()->pix_hess = true;
	}
}

void ImageBase::extractPatch(VectorXd &pix_vals, const Matrix2Xd& pts){
	assert(pix_vals.size() == patch_size && pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getPixVals(pix_vals, curr_img, pts, n_pix, img_height, img_width);
		break;
	case 3:
		utils::getPixVals(pix_vals, curr_img_cv, pts, n_pix, img_height, img_width);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

VectorXd ImageBase::getPatch(const PtsT& curr_pts){
	VectorXd curr_patch(patch_size);
	extractPatch(curr_patch, curr_pts);
	return curr_patch;
}

void ImageBase::updatePixVals(const Matrix2Xd& curr_pts){
	assert(curr_pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getPixVals(It, curr_img, curr_pts, n_pix, img_height, img_width,
			pix_norm_mult, pix_norm_add); 
		break;
	case 3:
		utils::getPixVals(It, curr_img_cv, curr_pts, n_pix, img_height, img_width,
			pix_norm_mult, pix_norm_add); 
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

void ImageBase::updatePixGrad(const Matrix2Xd &curr_pts){
	assert(curr_pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getImgGrad(dIt_dx, curr_img, curr_pts,
			grad_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getImgGrad(dIt_dx, curr_img_cv, curr_pts,
			grad_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

void ImageBase::updatePixHess(const Matrix2Xd &curr_pts){
	assert(curr_pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getImgHess(d2It_dx2, curr_img, curr_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getImgHess(d2It_dx2, curr_img_cv, curr_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

void ImageBase::updatePixGrad(const Matrix8Xd &warped_offset_pts){
	assert(warped_offset_pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getWarpedImgGrad(dIt_dx, curr_img, warped_offset_pts,
			grad_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getWarpedImgGrad(dIt_dx, curr_img_cv, warped_offset_pts,
			grad_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

void ImageBase::updatePixHess(const Matrix2Xd& curr_pts,
	const Matrix16Xd &warped_offset_pts){
	assert(curr_pts.cols() == n_pix && warped_offset_pts.cols() == n_pix);
	switch(n_channels){
	case 1:
		utils::getWarpedImgHess(d2It_dx2, curr_img, curr_pts, warped_offset_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	case 3:
		utils::getWarpedImgHess(d2It_dx2, curr_img_cv, curr_pts, warped_offset_pts,
			hess_eps, n_pix, img_height, img_width, pix_norm_mult);
		break;
	default:
		throw std::domain_error(cv::format("%d channel images are not supported yet", n_channels));
	}
}

#ifdef ENABLE_OLD_IMG_GRAD
void ImageBase::initializePixGrad(const HomPtsT& init_pts_hm,
	const ProjWarpT &init_warp){
	assert(init_pts_hm.cols() == n_pix);
	if(!isInitialized()->pix_grad){
		dI0_dx.resize(n_pix, Eigen::NoChange);
		dIt_dx.resize(n_pix, Eigen::NoChange);
	}
	utils::getWarpedImgGrad(dI0_dx,
		curr_img, init_pts_hm, init_warp,
		grad_eps, n_pix, img_height, img_width, pix_norm_mult);

	if(!isInitialized()->pix_grad){
		setCurrPixGrad(getInitPixGrad());
		isInitialized()->pix_grad = true;
	}
}
void ImageBase::initializePixHess(const Matrix3Xd& init_pts_hm,
	const Matrix3d &init_warp){
	assert(init_pts_hm.cols() == n_pix);
	if(!isInitialized()->pix_hess){
		d2I0_dx2.resize(Eigen::NoChange, n_pix);
		d2It_dx2.resize(Eigen::NoChange, n_pix);
	}
	utils::getWarpedImgHess(d2I0_dx2, curr_img, init_pts_hm,
		init_warp, hess_eps, n_pix, img_height, img_width, pix_norm_mult);

	if(!isInitialized()->pix_hess){
		setCurrPixHess(getInitPixHess());
		isInitialized()->pix_hess = true;
	}
}
#endif

_MTF_END_NAMESPACE