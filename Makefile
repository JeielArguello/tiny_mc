# Compilador CUDA
CC = nvcc

# Flags
EXTRA_CFLAGS = -O3 
CFLAGS = -std=c++14 -arch=sm_61 -Xcompiler "-Wall -Wextra" $(EXTRA_CFLAGS) -Wno-deprecated-gpu-targets
TINY_LDFLAGS = -lm
CG_LDFLAGS = -lm -lglfw -lGL -lGLEW

TARGETS = headless head

# Archivos fuente
CU_SOURCES = tiny_mc.cu photon.cu wtime.c
CU_OBJS = $(patsubst %.cu, %.o, $(filter %.cu, $(CU_SOURCES))) $(patsubst %.c, %.o, $(filter %.c, $(CU_SOURCES)))

# Reglas de compilación
%.o: %.cu
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

headless: $(CU_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(TINY_LDFLAGS)

head: cg_mc.o photon.o wtime.o
	$(CC) $(CFLAGS) -o $@ $^ $(CG_LDFLAGS)

clean:
	rm -f $(TARGETS) *.o