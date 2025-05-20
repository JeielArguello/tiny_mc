# Compilers
CC = icx

# Flags
EXTRA_CFLAGS = -O3 -funroll-loops
CFLAGS = -std=c11 -Wall -Wextra -march=native -ftree-vectorize -funsafe-math-optimizations -ffast-math $(EXTRA_CFLAGS)
TINY_LDFLAGS = -lm -fopenmp 
CG_LDFLAGS = -lm -lglfw -lGL -lGLEW

TARGETS = headless head

# Files
C_SOURCES = wtime.c photon.c
C_OBJS = $(patsubst %.c, %.o, $(C_SOURCES))

headless: tiny_mc.o $(C_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(TINY_LDFLAGS)

head: cg_mc.o $(C_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CG_LDFLAGS)

clean:
	rm -f $(TARGETS) *.o

