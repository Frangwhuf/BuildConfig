BranchSrc = $(shell pwd)
BranchRoot = $(BranchSrc)/debug

CC = gcc
LINK = gcc
AR = ar
RANLIB = ranlib

BIT_SIZE = $(shell getconf LONG_BIT)
FLAGS_COMMON = -ggdb -m$(BIT_SIZE) -march=core2 -mssse3 -Wall
IFLAGS_COMMON = -I $(BranchSrc)
CFLAGS_COMMON = $(IFLAGS_COMMON) -MMD
# other flags to think about -Wno-invalid-offsetof
CPPFLAGS_COMMON = -ffor-scope -fno-enforce-eh-specs -fno-threadsafe-statics -Wreorder -std=c++0x
# other flags to think about -Wl,-R$(BranchRoot)/lib
EXE_STDLIBS_COMMON = -lstdc++
CFLAGS_STATIC = $(FLAGS_COMMON) $(CFLAGS_COMMON) -D_DEBUG
# other flags to think about -mcmodel=small (64-bit only)
CFLAGS = $(CFLAGS_STATIC) -fPIC
EXE_LFLAGS = $(FLAGS_COMMON) -fPIC
EXE_STDLIBS = $(EXE_STDLIBS_COMMON)

############################
# General building targets #
############################

.PHONY: all
all : makedirs
	@echo Done building debug oconfig.

############################
# General cleaning targets #
############################

.PHONY: clean
clean : 
	@echo Done cleaning debug oconfig.

# Make sure the root of the build tree is there
$(BranchRoot) :
	@mkdir $(BranchRoot)
makedirs : $(BranchRoot)

###################
## project: oconfig
###################

.PHONY: oconfig clean-oconfig
oconifg : $(BranchRoot)/oconfig
	@echo Built debug oconfigoconfig.
all : oconfig
clean-oconfig : 
	@if test -f $(BranchRoot)/oconfig; then rm $(BranchRoot)/oconfig; fi;
	@echo Cleaned debug oconfig.
clean : clean-oconfig

$(BranchRoot)/check_include.o : $(BranchSrc)/check_include.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/check_include.o -c $(BranchSrc)/check_include.cxx
.PHONY : clean-$(BranchRoot)/check_include.o
clean-$(BranchRoot)/check_include.o :
	@if test -f $(BranchRoot)/check_include.o; then rm $(BranchRoot)/check_include.o; fi;
	@if test -f $(BranchRoot)/check_include.d; then rm $(BranchRoot)/check_include.d; fi;
clean-oconfig : clean-$(BranchRoot)/check_include.o

-include $(BranchRoot)/check_include.d

$(BranchRoot)/executionEnvironment.o : $(BranchSrc)/executionEnvironment.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/executionEnvironment.o -c $(BranchSrc)/executionEnvironment.cxx
.PHONY : clean-$(BranchRoot)/executionEnvironment.o
clean-$(BranchRoot)/executionEnvironment.o :
	@if test -f $(BranchRoot)/executionEnvironment.o; then rm $(BranchRoot)/executionEnvironment.o; fi;
	@if test -f $(BranchRoot)/executionEnvironment.d; then rm $(BranchRoot)/executionEnvironment.d; fi;
clean-oconfig : clean-$(BranchRoot)/executionEnvironment.o

-include $(BranchRoot)/executionEnvironment.d

$(BranchRoot)/filename.o : $(BranchSrc)/filename.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/filename.o -c $(BranchSrc)/filename.cxx
.PHONY : clean-$(BranchRoot)/filename.o
clean-$(BranchRoot)/filename.o :
	@if test -f $(BranchRoot)/filename.o; then rm $(BranchRoot)/filename.o; fi;
	@if test -f $(BranchRoot)/filename.d; then rm $(BranchRoot)/filename.d; fi;
clean-oconfig : clean-$(BranchRoot)/filename.o

-include $(BranchRoot)/filename.d

$(BranchRoot)/globPattern.o : $(BranchSrc)/globPattern.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/globPattern.o -c $(BranchSrc)/globPattern.cxx
.PHONY : clean-$(BranchRoot)/globPattern.o
clean-$(BranchRoot)/globPattern.o :
	@if test -f $(BranchRoot)/globPattern.o; then rm $(BranchRoot)/globPattern.o; fi;
	@if test -f $(BranchRoot)/globPattern.d; then rm $(BranchRoot)/globPattern.d; fi;
clean-oconfig : clean-$(BranchRoot)/globPattern.o

-include $(BranchRoot)/globPattern.d

$(BranchRoot)/gnu_getopt.o : $(BranchSrc)/gnu_getopt.c $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) -o $(BranchRoot)/gnu_getopt.o -c $(BranchSrc)/gnu_getopt.c
.PHONY : clean-$(BranchRoot)/gnu_getopt.o
clean-$(BranchRoot)/gnu_getopt.o :
	@if test -f $(BranchRoot)/gnu_getopt.o; then rm $(BranchRoot)/gnu_getopt.o; fi;
	@if test -f $(BranchRoot)/gnu_getopt.d; then rm $(BranchRoot)/gnu_getopt.d; fi;
clean-oconfig : clean-$(BranchRoot)/gnu_getopt.o

-include $(BranchRoot)/gnu_getopt.d

$(BranchRoot)/gnu_regex.o : $(BranchSrc)/gnu_regex.c $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) -o $(BranchRoot)/gnu_regex.o -c $(BranchSrc)/gnu_regex.c
.PHONY : clean-$(BranchRoot)/gnu_regex.o
clean-$(BranchRoot)/gnu_regex.o :
	@if test -f $(BranchRoot)/gnu_regex.o; then rm $(BranchRoot)/gnu_regex.o; fi;
	@if test -f $(BranchRoot)/gnu_regex.d; then rm $(BranchRoot)/gnu_regex.d; fi;
clean-oconfig : clean-$(BranchRoot)/gnu_regex.o

-include $(BranchRoot)/gnu_regex.d

$(BranchRoot)/isSpace.o : $(BranchSrc)/isSpace.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/isSpace.o -c $(BranchSrc)/isSpace.cxx
.PHONY : clean-$(BranchRoot)/isSpace.o
clean-$(BranchRoot)/isSpace.o :
	@if test -f $(BranchRoot)/isSpace.o; then rm $(BranchRoot)/isSpace.o; fi;
	@if test -f $(BranchRoot)/isSpace.d; then rm $(BranchRoot)/isSpace.d; fi;
clean-oconfig : clean-$(BranchRoot)/isSpace.o

-include $(BranchRoot)/isSpace.d

$(BranchRoot)/md5.o : $(BranchSrc)/md5.c $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) -o $(BranchRoot)/md5.o -c $(BranchSrc)/md5.c
.PHONY : clean-$(BranchRoot)/md5.o
clean-$(BranchRoot)/md5.o :
	@if test -f $(BranchRoot)/md5.o; then rm $(BranchRoot)/md5.o; fi;
	@if test -f $(BranchRoot)/md5.d; then rm $(BranchRoot)/md5.d; fi;
clean-oconfig : clean-$(BranchRoot)/md5.o

-include $(BranchRoot)/md5.d

$(BranchRoot)/ocCommandFile.o : $(BranchSrc)/ocCommandFile.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocCommandFile.o -c $(BranchSrc)/ocCommandFile.cxx
.PHONY : clean-$(BranchRoot)/ocCommandFile.o
clean-$(BranchRoot)/ocCommandFile.o :
	@if test -f $(BranchRoot)/ocCommandFile.o; then rm $(BranchRoot)/ocCommandFile.o; fi;
	@if test -f $(BranchRoot)/ocCommandFile.d; then rm $(BranchRoot)/ocCommandFile.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocCommandFile.o

-include $(BranchRoot)/ocCommandFile.d

$(BranchRoot)/ocDependableFile.o : $(BranchSrc)/ocDependableFile.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocDependableFile.o -c $(BranchSrc)/ocDependableFile.cxx
.PHONY : clean-$(BranchRoot)/ocDependableFile.o
clean-$(BranchRoot)/ocDependableFile.o :
	@if test -f $(BranchRoot)/ocDependableFile.o; then rm $(BranchRoot)/ocDependableFile.o; fi;
	@if test -f $(BranchRoot)/ocDependableFile.d; then rm $(BranchRoot)/ocDependableFile.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocDependableFile.o

-include $(BranchRoot)/ocDependableFile.d

$(BranchRoot)/ocDirectory.o : $(BranchSrc)/ocDirectory.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocDirectory.o -c $(BranchSrc)/ocDirectory.cxx
.PHONY : clean-$(BranchRoot)/ocDirectory.o
clean-$(BranchRoot)/ocDirectory.o :
	@if test -f $(BranchRoot)/ocDirectory.o; then rm $(BranchRoot)/ocDirectory.o; fi;
	@if test -f $(BranchRoot)/ocDirectory.d; then rm $(BranchRoot)/ocDirectory.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocDirectory.o

-include $(BranchRoot)/ocDirectory.d

$(BranchRoot)/ocDirectoryTree.o : $(BranchSrc)/ocDirectoryTree.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocDirectoryTree.o -c $(BranchSrc)/ocDirectoryTree.cxx
.PHONY : clean-$(BranchRoot)/ocDirectoryTree.o
clean-$(BranchRoot)/ocDirectoryTree.o :
	@if test -f $(BranchRoot)/ocDirectoryTree.o; then rm $(BranchRoot)/ocDirectoryTree.o; fi;
	@if test -f $(BranchRoot)/ocDirectoryTree.d; then rm $(BranchRoot)/ocDirectoryTree.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocDirectoryTree.o

-include $(BranchRoot)/ocDirectoryTree.d

$(BranchRoot)/ocFilenamePattern.o : $(BranchSrc)/ocFilenamePattern.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocFilenamePattern.o -c $(BranchSrc)/ocFilenamePattern.cxx
.PHONY : clean-$(BranchRoot)/ocFilenamePattern.o
clean-$(BranchRoot)/ocFilenamePattern.o :
	@if test -f $(BranchRoot)/ocFilenamePattern.o; then rm $(BranchRoot)/ocFilenamePattern.o; fi;
	@if test -f $(BranchRoot)/ocFilenamePattern.d; then rm $(BranchRoot)/ocFilenamePattern.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocFilenamePattern.o

-include $(BranchRoot)/ocFilenamePattern.d

$(BranchRoot)/ocMain.o : $(BranchSrc)/ocMain.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocMain.o -c $(BranchSrc)/ocMain.cxx
.PHONY : clean-$(BranchRoot)/ocMain.o
clean-$(BranchRoot)/ocMain.o :
	@if test -f $(BranchRoot)/ocMain.o; then rm $(BranchRoot)/ocMain.o; fi;
	@if test -f $(BranchRoot)/ocMain.d; then rm $(BranchRoot)/ocMain.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocMain.o

-include $(BranchRoot)/ocMain.d

$(BranchRoot)/ocNamedScopes.o : $(BranchSrc)/ocNamedScopes.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocNamedScopes.o -c $(BranchSrc)/ocNamedScopes.cxx
.PHONY : clean-$(BranchRoot)/ocNamedScopes.o
clean-$(BranchRoot)/ocNamedScopes.o :
	@if test -f $(BranchRoot)/ocNamedScopes.o; then rm $(BranchRoot)/ocNamedScopes.o; fi;
	@if test -f $(BranchRoot)/ocNamedScopes.d; then rm $(BranchRoot)/ocNamedScopes.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocNamedScopes.o

-include $(BranchRoot)/ocNamedScopes.d

$(BranchRoot)/oconfig.o : $(BranchSrc)/oconfig.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/oconfig.o -c $(BranchSrc)/oconfig.cxx
.PHONY : clean-$(BranchRoot)/oconfig.o
clean-$(BranchRoot)/oconfig.o :
	@if test -f $(BranchRoot)/oconfig.o; then rm $(BranchRoot)/oconfig.o; fi;
	@if test -f $(BranchRoot)/oconfig.d; then rm $(BranchRoot)/oconfig.d; fi;
clean-oconfig : clean-$(BranchRoot)/oconfig.o

-include $(BranchRoot)/oconfig.d

$(BranchRoot)/ocProfileInfo.o : $(BranchSrc)/ocProfileInfo.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocProfileInfo.o -c $(BranchSrc)/ocProfileInfo.cxx
.PHONY : clean-$(BranchRoot)/ocProfileInfo.o
clean-$(BranchRoot)/ocProfileInfo.o :
	@if test -f $(BranchRoot)/ocProfileInfo.o; then rm $(BranchRoot)/ocProfileInfo.o; fi;
	@if test -f $(BranchRoot)/ocProfileInfo.d; then rm $(BranchRoot)/ocProfileInfo.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocProfileInfo.o

-include $(BranchRoot)/ocProfileInfo.d

$(BranchRoot)/ocScope.o : $(BranchSrc)/ocScope.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocScope.o -c $(BranchSrc)/ocScope.cxx
.PHONY : clean-$(BranchRoot)/ocScope.o
clean-$(BranchRoot)/ocScope.o :
	@if test -f $(BranchRoot)/ocScope.o; then rm $(BranchRoot)/ocScope.o; fi;
	@if test -f $(BranchRoot)/ocScope.d; then rm $(BranchRoot)/ocScope.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocScope.o

-include $(BranchRoot)/ocScope.d

$(BranchRoot)/ocSearchPath.o : $(BranchSrc)/ocSearchPath.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocSearchPath.o -c $(BranchSrc)/ocSearchPath.cxx
.PHONY : clean-$(BranchRoot)/ocSearchPath.o
clean-$(BranchRoot)/ocSearchPath.o :
	@if test -f $(BranchRoot)/ocSearchPath.o; then rm $(BranchRoot)/ocSearchPath.o; fi;
	@if test -f $(BranchRoot)/ocSearchPath.d; then rm $(BranchRoot)/ocSearchPath.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocSearchPath.o

-include $(BranchRoot)/ocSearchPath.d

$(BranchRoot)/ocSubroutine.o : $(BranchSrc)/ocSubroutine.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/ocSubroutine.o -c $(BranchSrc)/ocSubroutine.cxx
.PHONY : clean-$(BranchRoot)/ocSubroutine.o
clean-$(BranchRoot)/ocSubroutine.o :
	@if test -f $(BranchRoot)/ocSubroutine.o; then rm $(BranchRoot)/ocSubroutine.o; fi;
	@if test -f $(BranchRoot)/ocSubroutine.d; then rm $(BranchRoot)/ocSubroutine.d; fi;
clean-oconfig : clean-$(BranchRoot)/ocSubroutine.o

-include $(BranchRoot)/ocSubroutine.d

$(BranchRoot)/sedAddress.o : $(BranchSrc)/sedAddress.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/sedAddress.o -c $(BranchSrc)/sedAddress.cxx
.PHONY : clean-$(BranchRoot)/sedAddress.o
clean-$(BranchRoot)/sedAddress.o :
	@if test -f $(BranchRoot)/sedAddress.o; then rm $(BranchRoot)/sedAddress.o; fi;
	@if test -f $(BranchRoot)/sedAddress.d; then rm $(BranchRoot)/sedAddress.d; fi;
clean-oconfig : clean-$(BranchRoot)/sedAddress.o

-include $(BranchRoot)/sedAddress.d

$(BranchRoot)/sedCommand.o : $(BranchSrc)/sedCommand.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/sedCommand.o -c $(BranchSrc)/sedCommand.cxx
.PHONY : clean-$(BranchRoot)/sedCommand.o
clean-$(BranchRoot)/sedCommand.o :
	@if test -f $(BranchRoot)/sedCommand.o; then rm $(BranchRoot)/sedCommand.o; fi;
	@if test -f $(BranchRoot)/sedCommand.d; then rm $(BranchRoot)/sedCommand.d; fi;
clean-oconfig : clean-$(BranchRoot)/sedCommand.o

-include $(BranchRoot)/sedCommand.d

$(BranchRoot)/sedContext.o : $(BranchSrc)/sedContext.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/sedContext.o -c $(BranchSrc)/sedContext.cxx
.PHONY : clean-$(BranchRoot)/sedContext.o
clean-$(BranchRoot)/sedContext.o :
	@if test -f $(BranchRoot)/sedContext.o; then rm $(BranchRoot)/sedContext.o; fi;
	@if test -f $(BranchRoot)/sedContext.d; then rm $(BranchRoot)/sedContext.d; fi;
clean-oconfig : clean-$(BranchRoot)/sedContext.o

-include $(BranchRoot)/sedContext.d

$(BranchRoot)/sedProcess.o : $(BranchSrc)/sedProcess.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/sedProcess.o -c $(BranchSrc)/sedProcess.cxx
.PHONY : clean-$(BranchRoot)/sedProcess.o
clean-$(BranchRoot)/sedProcess.o :
	@if test -f $(BranchRoot)/sedProcess.o; then rm $(BranchRoot)/sedProcess.o; fi;
	@if test -f $(BranchRoot)/sedProcess.d; then rm $(BranchRoot)/sedProcess.d; fi;
clean-oconfig : clean-$(BranchRoot)/sedProcess.o

-include $(BranchRoot)/sedProcess.d

$(BranchRoot)/sedScript.o : $(BranchSrc)/sedScript.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/sedScript.o -c $(BranchSrc)/sedScript.cxx
.PHONY : clean-$(BranchRoot)/sedScript.o
clean-$(BranchRoot)/sedScript.o :
	@if test -f $(BranchRoot)/sedScript.o; then rm $(BranchRoot)/sedScript.o; fi;
	@if test -f $(BranchRoot)/sedScript.d; then rm $(BranchRoot)/sedScript.d; fi;
clean-oconfig : clean-$(BranchRoot)/sedScript.o

-include $(BranchRoot)/sedScript.d

$(BranchRoot)/tokenize.o : $(BranchSrc)/tokenize.cxx $(BranchSrc)/Makefile | $(BranchRoot)
	$(CC) $(CFLAGS) $(CPPFLAGS_COMMON) -o $(BranchRoot)/tokenize.o -c $(BranchSrc)/tokenize.cxx
.PHONY : clean-$(BranchRoot)/tokenize.o
clean-$(BranchRoot)/tokenize.o :
	@if test -f $(BranchRoot)/tokenize.o; then rm $(BranchRoot)/tokenize.o; fi;
	@if test -f $(BranchRoot)/tokenize.d; then rm $(BranchRoot)/tokenize.d; fi;
clean-oconfig : clean-$(BranchRoot)/tokenize.o

-include $(BranchRoot)/tokenize.d

$(BranchRoot)/oconfig : $(BranchRoot)/check_include.o $(BranchRoot)/executionEnvironment.o $(BranchRoot)/filename.o $(BranchRoot)/globPattern.o $(BranchRoot)/gnu_getopt.o $(BranchRoot)/gnu_regex.o $(BranchRoot)/isSpace.o $(BranchRoot)/md5.o $(BranchRoot)/ocCommandFile.o $(BranchRoot)/ocDependableFile.o $(BranchRoot)/ocDirectory.o $(BranchRoot)/ocDirectoryTree.o $(BranchRoot)/ocFilenamePattern.o $(BranchRoot)/ocMain.o $(BranchRoot)/ocNamedScopes.o $(BranchRoot)/oconfig.o $(BranchRoot)/ocProfileInfo.o $(BranchRoot)/ocScope.o $(BranchRoot)/ocSearchPath.o $(BranchRoot)/ocSubroutine.o $(BranchRoot)/sedAddress.o $(BranchRoot)/sedCommand.o $(BranchRoot)/sedContext.o $(BranchRoot)/sedProcess.o $(BranchRoot)/sedScript.o $(BranchRoot)/tokenize.o $(BranchSrc)/Makefile
	$(LINK) $(EXE_FLAGS) -o $(BranchRoot)/oconfig $(BranchRoot)/check_include.o $(BranchRoot)/executionEnvironment.o $(BranchRoot)/filename.o $(BranchRoot)/globPattern.o $(BranchRoot)/gnu_getopt.o $(BranchRoot)/gnu_regex.o $(BranchRoot)/isSpace.o $(BranchRoot)/md5.o $(BranchRoot)/ocCommandFile.o $(BranchRoot)/ocDependableFile.o $(BranchRoot)/ocDirectory.o $(BranchRoot)/ocDirectoryTree.o $(BranchRoot)/ocFilenamePattern.o $(BranchRoot)/ocMain.o $(BranchRoot)/ocNamedScopes.o $(BranchRoot)/oconfig.o $(BranchRoot)/ocProfileInfo.o $(BranchRoot)/ocScope.o $(BranchRoot)/ocSearchPath.o $(BranchRoot)/ocSubroutine.o $(BranchRoot)/sedAddress.o $(BranchRoot)/sedCommand.o $(BranchRoot)/sedContext.o $(BranchRoot)/sedProcess.o $(BranchRoot)/sedScript.o $(BranchRoot)/tokenize.o $(EXE_STDLIBS)

all : $(BranchRoot)/oconfig
