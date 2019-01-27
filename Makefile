LINK.o = $(LINK.cc)
CXXFLAGS = -ggdb3 -Og -pedantic -Wall -Werror #-DNOTHREAD
LDFLAGS = -ggdb3 -Og -Lsolvitaire #-DNOTHREAD

OUTPUT_OPTION = -MMD -MP -o $@
LDLIBS = -lsol -lpthread

TRGT = centre
SRC = metasol.cpp centre.cpp thpool/thpool.c
OBJ = metasol.o centre.o thpool/thpool.o
DEP = metasol.d centre.d thpool/thpool.d

.PHONY: default all run

default: all
all: $(TRGT)
$(TRGT): $(OBJ)

run: $(TRGT)
	./$<

-include $(DEP)

clean:
	$(RM) $(TRGT) $(OBJ) $(DEP)
