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
CMAKE_SOURCE_DIR = /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build

# Include any dependencies generated for this target.
include test/parsing/CMakeFiles/yajl_test.dir/depend.make

# Include the progress variables for this target.
include test/parsing/CMakeFiles/yajl_test.dir/progress.make

# Include the compile flags for this target's objects.
include test/parsing/CMakeFiles/yajl_test.dir/flags.make

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o: test/parsing/CMakeFiles/yajl_test.dir/flags.make
test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o: ../test/parsing/yajl_test.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o"
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/yajl_test.dir/yajl_test.c.o   -c /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/test/parsing/yajl_test.c

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/yajl_test.dir/yajl_test.c.i"
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -E /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/test/parsing/yajl_test.c > CMakeFiles/yajl_test.dir/yajl_test.c.i

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/yajl_test.dir/yajl_test.c.s"
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing && /usr/bin/cc  $(C_DEFINES) $(C_FLAGS) -S /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/test/parsing/yajl_test.c -o CMakeFiles/yajl_test.dir/yajl_test.c.s

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.requires:
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.requires

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.provides: test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.requires
	$(MAKE) -f test/parsing/CMakeFiles/yajl_test.dir/build.make test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.provides.build
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.provides

test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.provides.build: test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o

# Object files for target yajl_test
yajl_test_OBJECTS = \
"CMakeFiles/yajl_test.dir/yajl_test.c.o"

# External object files for target yajl_test
yajl_test_EXTERNAL_OBJECTS =

test/parsing/yajl_test: test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o
test/parsing/yajl_test: test/parsing/CMakeFiles/yajl_test.dir/build.make
test/parsing/yajl_test: yajl-2.1.0/lib/libyajl_s.a
test/parsing/yajl_test: test/parsing/CMakeFiles/yajl_test.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable yajl_test"
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/yajl_test.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/parsing/CMakeFiles/yajl_test.dir/build: test/parsing/yajl_test
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/build

test/parsing/CMakeFiles/yajl_test.dir/requires: test/parsing/CMakeFiles/yajl_test.dir/yajl_test.c.o.requires
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/requires

test/parsing/CMakeFiles/yajl_test.dir/clean:
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing && $(CMAKE_COMMAND) -P CMakeFiles/yajl_test.dir/cmake_clean.cmake
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/clean

test/parsing/CMakeFiles/yajl_test.dir/depend:
	cd /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0 /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/test/parsing /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing /home/controller/cetcd-6db806bd18f8ada6bb7fb775a2f1116450d08f36/third-party/yajl-2.1.0/build/test/parsing/CMakeFiles/yajl_test.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/parsing/CMakeFiles/yajl_test.dir/depend

