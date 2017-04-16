option(WITH_THIRD_PARTY "Enable Third Party Trackers" ON)
set(THIRD_PARTY_RUNTIME_FLAGS "")

if(WITH_THIRD_PARTY)
	# set(THIRD_PARTY_TRACKERS "")
	# set(THIRD_PARTY_LIBS "")
	# set(THIRD_PARTY_INCLUDE_DIRS "")
	# set(THIRD_PARTY_SPECIFIC_SRC "")
	# set(THIRD_PARTY_SPECIFIC_PROPERTIES "")
	# set(THIRD_PARTY_EXT_INCLUDE_DIRS "")
	# set(MTF_THIRD_PARTY_LIB_SRC "")
	if (WIN32)
	# third party trackers with cmake build systems do not compile in Windows yet
	set(THIRD_PARTY_SUB_DIRS DSST KCF RCT)
	set(THIRD_PARTY_DEFINITIONS ${THIRD_PARTY_DEFINITIONS} DISABLE_XVISION DISABLE_VISP DISABLE_PFSL3)
	else()
	set(THIRD_PARTY_SUB_DIRS DSST KCF CMT RCT TLD Struck MIL DFT FRG PFSL3 ViSP GOTURN Xvision)
	endif()
	foreach(SUB_DIR ${THIRD_PARTY_SUB_DIRS})
	  include(ThirdParty/${SUB_DIR}/${SUB_DIR}.cmake)
	endforeach(SUB_DIR)
	message(STATUS "Enabled third party trackers: " ${THIRD_PARTY_TRACKERS})
	addPrefixAndSuffix("${THIRD_PARTY_TRACKERS}" "ThirdParty/" ".cc" THIRD_PARTY_SRC)
	addPrefix("${THIRD_PARTY_INCLUDE_DIRS}" "ThirdParty/" THIRD_PARTY_INCLUDE_DIRS_ABS)
	addPrefix("${THIRD_PARTY_SPECIFIC_SRC}" "ThirdParty/" THIRD_PARTY_SPECIFIC_SRC_ABS)
	set(MTF_SRC ${MTF_SRC} ${THIRD_PARTY_SRC})
	set(MTF_LIBS ${MTF_LIBS} ${THIRD_PARTY_LIBS})
	set(MTF_INCLUDE_DIRS ${MTF_INCLUDE_DIRS} ${THIRD_PARTY_INCLUDE_DIRS_ABS})
	set(MTF_EXT_INCLUDE_DIRS ${MTF_EXT_INCLUDE_DIRS} ${THIRD_PARTY_EXT_INCLUDE_DIRS})
	set(MTF_SPECIFIC_SRC ${MTF_SPECIFIC_SRC} ${THIRD_PARTY_SPECIFIC_SRC_ABS})
	set(MTF_SPECIFIC_PROPERTIES ${MTF_SPECIFIC_PROPERTIES} ${THIRD_PARTY_SPECIFIC_PROPERTIES})	
	set(MTF_RUNTIME_FLAGS ${MTF_RUNTIME_FLAGS} ${THIRD_PARTY_RUNTIME_FLAGS})	
	
	# addPrefix("${MTF_THIRD_PARTY_LIB_SRC}" "ThirdParty/" MTF_THIRD_PARTY_LIB_SRC_ABS)
	# add_library (mtf_thirdparty SHARED ${MTF_THIRD_PARTY_LIB_SRC})
	# target_compile_options(mtf PUBLIC ${WARNING_FLAGS} ${MTF_COMPILETIME_FLAGS})
	# target_link_libraries(mtf ${MTF_LIBS} ${THIRD_PARTY_LIBS})
	# target_include_directories(mtf PUBLIC ${MTF_INCLUDE_DIRS} ${MTF_EXT_INCLUDE_DIRS})
	
else(WITH_THIRD_PARTY)
	message(STATUS "Third party trackers disabled")
	set(THIRD_PARTY_DEFINITIONS ${THIRD_PARTY_DEFINITIONS} DISABLE_THIRD_PARTY_TRACKERS DISABLE_XVISION DISABLE_VISP DISABLE_PFSL3)
endif(WITH_THIRD_PARTY)
set(MTF_DEFINITIONS ${MTF_DEFINITIONS} ${THIRD_PARTY_DEFINITIONS})
