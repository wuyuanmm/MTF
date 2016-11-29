option(WITH_FMAPS "Enable FMaps appearance model" OFF)
option(WITH_PCA "Enable PCA appearance model" OFF)

set(APPEARANCE_MODELS ImageBase SSDBase SSD NSSD ZNCC SCV LSCV RSCV LRSCV CCRE LKLD MI SSIM NCC SPSS KLD RIU NGF SAD MCSSD MCSCV MCLSCV MCRSCV MCZNCC MCNCC MCMI MCCCRE MCSSIM MCSPSS MCRIU MCSAD SumOfAMs)
set(ILLUMINATION_MODELS GB PGB RBF)


if(WITH_FMAPS)	
	find_package(Caffe)
	if(Caffe_FOUND)
		set(APPEARANCE_MODELS ${APPEARANCE_MODELS} FMaps)
		set(MTF_EXT_INCLUDE_DIRS ${MTF_EXT_INCLUDE_DIRS} ${Caffe_INCLUDE_DIRS})
		set(MTF_LIBS ${MTF_LIBS} ${Caffe_LIBRARIES})
		set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/FMaps.cc PROPERTIES COMPILE_FLAGS ${Caffe_DEFINITIONS})
	else(Caffe_FOUND)
		message(STATUS "Caffe not found so FMaps AM cannot be enabled")
		set(CT_FLAGS ${CT_FLAGS} -DDISABLE_FMAPS)
	endif(Caffe_FOUND)
else(WITH_FMAPS)
	message(STATUS "FMaps disabled")
	set(CT_FLAGS ${CT_FLAGS} -DDISABLE_FMAPS)	
endif(WITH_FMAPS)

if(WITH_PCA)
	set(APPEARANCE_MODELS ${APPEARANCE_MODELS} PCA)
else(WITH_PCA)
	message(STATUS "PCA disabled")
	set(CT_FLAGS ${CT_FLAGS} -DDISABLE_PCA)	
endif(WITH_PCA)

addPrefixAndSuffix("${APPEARANCE_MODELS}" "AM/src/" ".cc" APPEARANCE_MODELS_SRC)
addPrefixAndSuffix("${ILLUMINATION_MODELS}" "AM/src/" ".cc" ILLUMINATION_MODELS_SRC)
set(MTF_SRC ${MTF_SRC} ${APPEARANCE_MODELS_SRC} ${ILLUMINATION_MODELS_SRC})
set(MTF_INCLUDE_DIRS ${MTF_INCLUDE_DIRS} AM/include)