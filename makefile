SRCDIR=src
CC=gcc
CFLAGS=-I$(SRCDIR)
ODIR=obj
LIBS=-lm -lcairo -lBox2D -lSDL2 -lboost_system -lboost_filesystem -lboost_thread -lstdc++

_DEPS = Animation.hpp Application.hpp Console.hpp ImageCache.hpp
DEPS = $(patsubst %,$(SRCDIR)/%,$(_DEPS))

_OBJS = Animation.o Application.o Console.o ImageCache.o main.o
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SRCDIR)/%.cpp $(DEPS)
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

boxes: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)
