# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF/build2

# Utility rule file for uav.

# Include the progress variables for this target.
include CMakeFiles/uav.dir/progress.make

CMakeFiles/uav: trackUAVTrajectory

uav: CMakeFiles/uav
uav: CMakeFiles/uav.dir/build.make
.PHONY : uav

# Rule to build all files generated by this target.
CMakeFiles/uav.dir/build: uav
.PHONY : CMakeFiles/uav.dir/build

CMakeFiles/uav.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/uav.dir/cmake_clean.cmake
.PHONY : CMakeFiles/uav.dir/clean

CMakeFiles/uav.dir/depend:
	cd /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF/build2 && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF/build2 /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF/build2 /home/abhineet/E/UofA/Thesis/Code/TrackingFramework/C++/MTF/build2/CMakeFiles/uav.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/uav.dir/depend

