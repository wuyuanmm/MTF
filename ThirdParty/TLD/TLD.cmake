add_subdirectory(${CMAKE_CURRENT_LIST_DIR})
set(LEARNING_TRACKERS ${LEARNING_TRACKERS} TLD/src/TLD)
set(THIRD_PARTY_INCLUDE_DIRS ${THIRD_PARTY_INCLUDE_DIRS} TLD/include)
set(LIBS_LEARNING ${LIBS_LEARNING} opentld cvblobs)
