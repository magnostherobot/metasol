LINK.o = $(LINK.cc)
CXXFLAGS = -ggdb3 -O0 -pedantic -Wall -Werror #-DNOTHREAD
LDFLAGS = -ggdb3 -O0 -Lcmake-build-release #-DNOTHREAD

OUTPUT_OPTION = -MMD -MP -o $@
LDLIBS = -lapi -lpthread

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
