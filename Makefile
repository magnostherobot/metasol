CXX = g++
CXXFLAGS = -ggdb3 -O0 -Wpedantic -Wall -Werror
OUTPUT_OPTION = -MMD -MP -o $@

LINK.o = $(LINK.cc)
LDFLAGS = -Lthpool -Ldocopt -Ljsmn -Lsolvitaire
LDLIBS = -lpthread -lsol -lthpool -ldocopt -ljsmn

TRGT = centre
SRC = metasol.cpp centre.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

.PHONY: default all init-submodules docs

default: all
all: $(TRGT)
$(TRGT): $(OBJ)

init-submodules:
	git submodule init
	git submodule update
	$(MAKE) -C thpool
	$(MAKE) -C jsmn
	(cd docopt; cmake .) ; $(MAKE) -C docopt docopt_s
	(cd solvitaire; sh build.sh)

docs:
	doxygen ./Doxyfile

-include $(DEP)

clean:
	$(RM) $(TRGT) $(OBJ) $(DEP) -r docs
