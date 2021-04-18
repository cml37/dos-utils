################################################################################
# Generic makefile for building DOS applications with Open Watcom and NASM.    #
# v. 1.0.0 (2011.01.11)                                                        #
# (C) Tomi Tilli                                                               #
# aitotat@gmail.com                                                            #
#                                                                              #
# NOTE!                                                                        #
# Dependency checks are incomplete for ASM files assembled with NASM. Any      #
# changes to included files (usually .INC) will not be detected.               #
#                                                                              #
# Valid makefile targets for WMAKE are:                                        #
# relese	Builds release version to \Build\Release\                      #
# debug		Builds debug version to \Build\Debug\                          #
# clean		Removes all build files.                                       #
#                                                                              #
# Build\Debug and Build\Release directories must be created manually           #
# if they do not exist!                                                        #
#                                                                              #
################################################################################

###########################################
# Source files and destination executable #
###########################################

# C source code files (*.c):
SRC_C = mainc.c handlers.c

# C++ source code files (*.cpp):
SRC_CXX =

# Assembly source code files (*.asm):
SRC_ASM = tsrresic.asm tsrplex.asm biosndos.asm

# Program executable file name without extension:
PROG = tsrpush

# Other user objects:
#USER_OBJS =

# Libraries needed (without .lib extension):
#LIBRARIES =


######################################
# Source and destination directories #
######################################

# Subdirectories (if any) where source files are:
SOURCES = Src\

# Subdirectories (others than defined in VPATH, if any) where header files are:
HEADERS = Inc\


#####################################################
# Output binary type (COM or EXE) and memory model: #                          
# s = tiny/small (small code / small data)          #
# m = medium (large code / small data)              #
# c = Compact (small code / large data)             #
# l = large (large code / large data)               #
# h = huge (large code / huge data)                 #
#####################################################

# Type of binary to be build. Can be com or exe.
BINARY_TYPE = exe
MEMORY_MODEL = s


#################################################################
# C (and C++) preprocessor defines for compiler                 #
# DEBUG is added automatically when building debug version.     #
# RELEASE is added automatically when building release version. #
#################################################################
DEFINES =


############################
# Make command line macros #
############################

# If building a debug version
!ifeq BUILD debug
BUILD_DIR = Build\Debug\
DEFINES += DEBUG
!else # If building a release version
BUILD_DIR = Build\Release\
DEFINES += RELEASE
!endif


##############################################
# Generated object names and other variables #
##############################################

# list of generated object files (*.obj):
OBJ = $(SRC_ASM:.asm=.obj)		# filename.asm	=> filename.obj
OBJ += $(SRC_C:.c=.obj)			# filename.c	=> filename.obj
OBJ += $(SRC_CXX:.cpp=.obj)		# filename.cpp	=> filename.obj

# List of NASM generated dependency files (*.d):
ASMDEPS = $(SRC_ASM:.asm=.d)	# filename.asm	=> filename.d


# Add -d in front of every define declaration
DEFS = -d$(DEFINES: = -d)


# Add -i (for NASM) and -i= (for WATCOM) in front of all header directories
NASM_INCLUDE = -i$(HEADERS: = -i)
WATCOM_INCLUDE = -i=$(HEADERS: =;)

# Create WLINK directives for library files and search paths
LIBPATH = LIBPATH $(HEADERS: =;)
!ifdef LIBRARIES
LIBRARY = LIBRARY $(LIBRARIES: =,)
!else
LIBRARY =
!endif

# Path + target file to be build
TARGET = $(BUILD_DIR)$(PROG).$(BINARY_TYPE)


#########################
# Compilers and linkers #
#########################

# C compiler
CC = wcc.exe

# C++ compiler
CXX = wpp.exe

# Disassembler
DISASM = wdis.exe

# Assembly compiler
AS = nasm.exe

# Linker
LD = wlink.exe

# Resource script compiler
RC = wrc.exe

# Use this command to erase files.
RM = -del /Q


#############################
# Compiler and linker flags #
#############################

# Common C and C++ compiler flags
C_AND_CXX_FLAGS = -0					# Generate 8086/8088 instructions only
C_AND_CXX_FLAGS += -bt=dos				# Generate program for MS-DOS				
C_AND_CXX_FLAGS += -m$(MEMORY_MODEL)
C_AND_CXX_FLAGS += $(DEFS)				# Preprocessor defines
C_AND_CXX_FLAGS += $(WATCOM_INCLUDE)	# Set header file directory paths
C_AND_CXX_FLAGS += -q					# Operate quietly
!ifeq BUILD debug						# Debug version flags
C_AND_CXX_FLAGS += -d2					# Full symbolic debugging information
C_AND_CXX_FLAGS += -hd					# Generate DWARF debugging information
!else									# Release version flags
C_AND_CXX_FLAGS += -s					# Remove stack overflow checks
C_AND_CXX_FLAGS += -oh					# Enable expensive optimizations (longer compiles)
C_AND_CXX_FLAGS += -os					# Favor code size over execution time in optimizations
#C_AND_CXX_FLAGS += -ot					# Favor execution time over code size in optimizations
!endif

# C compiler flags
CFLAGS = $(C_AND_CXX_FLAGS)

# C++ compiler flags
CXXFLAGS = $(C_AND_CXX_FLAGS)

# Assembly compiler flags
ASFLAGS = -f obj				# Object format
ASFLAGS += $(DEFS)				# Preprocessor defines
ASFLAGS += $(NASM_INCLUDE)		# Set header file directory paths
ASFLAGS += -Worphan-labels		# Warn about labels without colon
ASFLAGS += -O9					# Optimize operands to their shortest forms
!ifeq BUILD debug				# Debug version flags
ASFLAGS += -g					# Generate debug information
!endif

# Linker flags
LDFLAGS = SYSTEM dos
!ifeq BINARY_TYPE com
LDFLAGS = SYSTEM com
!endif
LDFLAGS += OPTION QUIET			# Operate quietly
LDFLAGS += OPTION MAP=$(BUILD_DIR)$(PROG).map	# Generate map file
!ifeq BUILD debug				# Debug version flags
!endif


############################################
# Build process. Actual work is done here. #
############################################

# Make clean debug and release versions
all: .SYMBOLIC
	@echo Building all...
	@$(MAKE) clean
	@$(MAKE) debug
	@$(MAKE) release
	@echo All build!

# Clean
clean: .SYMBOLIC
	@echo Cleaning files...
	@$(RM) Build\Debug\*.obj
	@$(RM) Build\Debug\*.d
	@$(RM) Build\Debug\*.lst
	@$(RM) Build\Debug\$(PROG).map
	@$(RM) Build\Debug\$(PROG).$(BINARY_TYPE)
	@$(RM) Build\Release\*.obj
	@$(RM) Build\Release\*.d
	@$(RM) Build\Release\*.lst
	@$(RM) Build\Release\$(PROG).map
	@$(RM) Build\Release\$(PROG).$(BINARY_TYPE)
	@echo Deleted *.d, *.lst, *.obj, $(PROG).map and $(PROG).$(BINARY_TYPE).

# Build debug
debug: .SYMBOLIC
	@echo Building debug...
	@$(MAKE) -z BUILD=debug internalBuild

# Release build
release: .SYMBOLIC
	@echo Building release...
	@$(MAKE) -z BUILD=release internalBuild

# debug or release selected with command line macro
internalBuild: $(TARGET) .PROCEDURE
#internalBuild: debugData $(TARGET) .PROCEDURE
	@echo Build process completed.

# Print debugging data (when modifying this makefile)
debugData: .PROCEDURE
	@echo Start of makefile debug
	@echo MAKE: $(MAKE)
	@echo __MAKEOPTS__: $(__MAKEOPTS__)
	@echo BUILD: $(BUILD)
	@echo BUILD_DIR: $(BUILD_DIR)
	@echo OBJ: $(OBJ)
	@echo ASMDEPS: $(ASMDEPS)
	@echo DEFS: $(DEFS)
	@echo NASM_INCLUDE: $(NASM_INCLUDE)
	@echo WATCOM_INCLUDE: $(WATCOM_INCLUDE)
	@echo LIBRARY: $(LIBRARY)
	@echo LIBPATH: $(LIBPATH)
	@echo TARGET: $(TARGET)
	@echo End of makefile debug


#######################################
# Actual object files are build below #
#######################################

# Tell Make where destination .OBJ files are found
.obj: $(BUILD_DIR)

# linking rule remains the same as before.
$(TARGET): $(OBJ)
	@echo Linking...
	@$(LD) $(LDFLAGS) NAME $(TARGET) PATH $(BUILD_DIR) FILE {$(OBJ) $(USER_OBJS)} $(LIBPATH) $(LIBRARY)
	@echo Successfully build $(BUILD) version to "$(BUILD_DIR)$^.".


# .d files should be included here to get dependency checks working for .ASM files

# Tell Make where source .ASM files are found
.asm: $(SOURCES: =;)

# Compile *.asm to *.obj and check for dependencies.
.asm.obj:
	@$(AS) $(ASFLAGS) -o "$(BUILD_DIR)$^." -l "$(BUILD_DIR)$^&.lst" "$<"
	@$(AS) $(ASFLAGS) -o "$(BUILD_DIR)$^." -M "$<" > "$(BUILD_DIR)$^&.d"
	@echo Compiled "$<" to "$(BUILD_DIR)$^.".


# Tell Make where source .C files are found
.c: $(SOURCES: =;)

# Compile *.c to *.obj and check for dependencies.
.c.obj: .AUTODEPEND
	@$(CC) $(CFLAGS) -fo="$(BUILD_DIR)$^." "$<"
	@$(DISASM) -s -e -l="$(BUILD_DIR)$^&.lst" "$(BUILD_DIR)$^."
	@echo Compiled "$<" to "$(BUILD_DIR)$^.".


# Tell Make where source .CPP files are found
.cpp: $(SOURCES: =;)

# Compile *.cpp to *.obj and check for dependencies.
.cpp.obj: .AUTODEPEND
	@$(CXX) $(CXXFLAGS) -fo="$(BUILD_DIR)$^." "$<"
	@$(DISASM) -s -e -l="$(BUILD_DIR)$^&.lst" "$(BUILD_DIR)$^."
	@echo Compiled "$<" to "$(BUILD_DIR)$^.".
