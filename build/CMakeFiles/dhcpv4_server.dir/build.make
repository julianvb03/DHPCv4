# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.30

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/Halbacis/Documents/DHCPv4

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/Halbacis/Documents/DHCPv4/build

# Include any dependencies generated for this target.
include CMakeFiles/dhcpv4_server.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/dhcpv4_server.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/dhcpv4_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/dhcpv4_server.dir/flags.make

CMakeFiles/dhcpv4_server.dir/text.c.o: CMakeFiles/dhcpv4_server.dir/flags.make
CMakeFiles/dhcpv4_server.dir/text.c.o: /home/Halbacis/Documents/DHCPv4/text.c
CMakeFiles/dhcpv4_server.dir/text.c.o: CMakeFiles/dhcpv4_server.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/Halbacis/Documents/DHCPv4/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/dhcpv4_server.dir/text.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT CMakeFiles/dhcpv4_server.dir/text.c.o -MF CMakeFiles/dhcpv4_server.dir/text.c.o.d -o CMakeFiles/dhcpv4_server.dir/text.c.o -c /home/Halbacis/Documents/DHCPv4/text.c

CMakeFiles/dhcpv4_server.dir/text.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing C source to CMakeFiles/dhcpv4_server.dir/text.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/Halbacis/Documents/DHCPv4/text.c > CMakeFiles/dhcpv4_server.dir/text.c.i

CMakeFiles/dhcpv4_server.dir/text.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling C source to assembly CMakeFiles/dhcpv4_server.dir/text.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/Halbacis/Documents/DHCPv4/text.c -o CMakeFiles/dhcpv4_server.dir/text.c.s

# Object files for target dhcpv4_server
dhcpv4_server_OBJECTS = \
"CMakeFiles/dhcpv4_server.dir/text.c.o"

# External object files for target dhcpv4_server
dhcpv4_server_EXTERNAL_OBJECTS =

bin/dhcpv4_server: CMakeFiles/dhcpv4_server.dir/text.c.o
bin/dhcpv4_server: CMakeFiles/dhcpv4_server.dir/build.make
bin/dhcpv4_server: CMakeFiles/dhcpv4_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/Halbacis/Documents/DHCPv4/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable bin/dhcpv4_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dhcpv4_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/dhcpv4_server.dir/build: bin/dhcpv4_server
.PHONY : CMakeFiles/dhcpv4_server.dir/build

CMakeFiles/dhcpv4_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/dhcpv4_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/dhcpv4_server.dir/clean

CMakeFiles/dhcpv4_server.dir/depend:
	cd /home/Halbacis/Documents/DHCPv4/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/Halbacis/Documents/DHCPv4 /home/Halbacis/Documents/DHCPv4 /home/Halbacis/Documents/DHCPv4/build /home/Halbacis/Documents/DHCPv4/build /home/Halbacis/Documents/DHCPv4/build/CMakeFiles/dhcpv4_server.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : CMakeFiles/dhcpv4_server.dir/depend
