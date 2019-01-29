CXX = g++
CXXFLAGS = -ggdb3 -O0 -Wpedantic -Wall -Werror #-DNOTHREAD
OUTPUT_OPTION = -MMD -MP -o $@

LINK.o = $(LINK.cc)
LDFLAGS = -Lsolvitaire -Lthpool -Ldocopt -Ljsmn
LDLIBS = -lpthread -lsol -lthpool -ldocopt -ljsmn

TRGT = centre
SRC = metasol.cpp centre.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(SRC:.cpp=.d)

.PHONY: default all run

default: all
all: $(TRGT)
$(TRGT): $(OBJ)

run: $(TRGT)
	./$<

-include $(DEP)

clean:
	$(RM) $(TRGT) $(OBJ) $(DEP)
