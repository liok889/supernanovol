# Super nanovol Makefile
# ==================================================
CXX=g++
CXXFLAGS=-O3 -fopenmp
LDFLAGS=-O3 

NVCC=nvcc
NVCCFLAGS=-m64
GENCODE_FLAGS=

# machine name and architecture
MACHINE=$(shell uname -s)
ARCH=$(shell uname -p)

# includes
# =========
SDL_INCLUDE=$(shell sdl-config --cflags)

# Libs
# =========
SDL_LIB=$(shell sdl-config --libs)
ifeq ($(MACHINE), Darwin)
LIBPATH= /opt/local/lib
SDL_LIB+= 
CXXFLAGS+= -arch x86_64
else
SDL_INCLUDE+= -I$(HOME)/devlib/include
SDL_LIB+= -lGLU -lGL 
endif

# everything together
INCLUDE=$(SDL_INCLUDE)
LIB=$(SDL_LIB) -lGLEW -ljpeg -lpng -ltiff

# static linking?
# ===============
ifeq ($(MACHINE), Darwin)
ifeq ($(STATIC), 1)

X11LIB = $(LIBPATH)/libX11.a $(LIBPATH)/libxcb.a $(LIBPATH)/libXau.a $(LIBPATH)/libXdmcp.a
LIB = $(LIBPATH)/libSDL.a $(LIBPATH)/libSDLmain.a $(LIBPATH)/libGLEW.a $(LIBPATH)/libpng.a $(LIBPATH)/libz.a $(LIBPATH)/libjpeg.a $(LIBPATH)/libtiff.a $(LIBPATH)/liblzma.a
LIB += $(LIBPATH)/libXrender.a $(LIBPATH)/libXrandr.a $(LIBPATH)/libXext.a $(X11LIB)
LIB += -framework OpenGL -Wl,-framework,Cocoa -Wl,-framework,ApplicationServices -Wl,-framework,Carbon -Wl,-framework,AudioToolbox -Wl,-framework,AudioUnit -Wl,-framework,IOKit
#LIB += -static-libgcc -static-libstdc++
endif
endif


# add additional headers for MacOSX
ifeq ($(MACHINE), Darwin)
INCLUDE+= -I/opt/local/include
LIB+= -framework OpenGL -static-libstdc++
endif

# Object files
# =================================================
OBJ=					\
	graphics/graphics.o		\
	graphics/shader.o		\
	graphics/framebuffer.o		\
	graphics/texture_unit.o	\
	graphics/image.o		\
	graphics/misc.o			\
	graphics/common.o		\
					\
	Timer.o				\
	camera.o			\
	data.o				\
	macrocells.o			\
	atoms.o				\
	charge_volume.o			\
	tf.o				\
	scalable_widget.o		\
	color_wheel.o			\
	debug.o				\
	video_out.o			\
	main.o

MPI_OBJ=				\
	parallel/rendernode.o		\
	parallel/cluster.o		\
	parallel/masternode.o		\
	parallel/tracker.o
	

# Header files
# =================================================
HEADER=					\
	graphics/graphics.h		\
	graphics/color.h		\
	graphics/common.h		\
	graphics/shader.h		\
	graphics/framebuffer.h		\
	camera.h			\
	data.h				\
	atoms.h				\
	grid.h				\
	rbf.h				\
	charge_volume.h			\
	scalable_widget.h		\
	tf.h				\
	color_wheel.h			\
	macrocells.h			

MPI_HEADER=				\
	parallel/rendernode.h		\
	parallel/cluster.h		\
	parallel/masternode.h		\
	parallel/tracker.h

TARGET=supernanovol

# compile with Oculus Rift support
# ---------------------------------
ifeq ($(RIFT), 1)
OBJ+= rift.o
HEADER+= rift.h
CXXFLAGS+= -DDO_RIFT -fno-rtti -fno-exceptions

ifeq ($(MACHINE), Darwin)
INCLUDE+= -I$(HOME)/devlib/LibOVR/Include
LIB+= $(HOME)/devlib/LibOVR/Lib/MacOS/Release/libovr.a -framework IOKit
else
INCLUDE+= -I$(HOME)/Downloads/OculusSDK/LibOVR/Include
LIB+= $(HOME)/Downloads/OculusSDK/LibOVR/Lib/Linux/Release/x86_64/libovr.a -lX11 -ludev -lXinerama
LIB+= -L$(HOME)/devlib/lib -L/lib/x86_64-linux-gnu
endif
endif


# compile with MPI support
# (note, we change the executable name to supernanovol-mpi)
# ----------------------------------------------------------
ifeq ($(MPI), 1)

OBJ+= $(MPI_OBJ)
HEADER+= $(MPI_HEADER)
CXXFLAGS+= -DDO_MPI
CXX=mpic++
TARGET=supernanovol-mpi

endif

ifeq ($(RIFT), 1)
TARGET=supernanovol-ovr
endif

# rules
# -------------------------

$(TARGET):	$(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) $(LIB) -o $(TARGET)

animate:	$(OBJ) animate.o
	$(CXX) $(LDFLAGS) atoms.o data.o graphics/misc.o animate.o macrocells.o charge_volume.o $(LIB) -o animate

%.o: %.cpp $(HEADER)
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o $@ $<

%.o: %.cxx $(HEADER)
	$(CXX) -c $(CXXFLAGS) $(INCLUDE) -o $@ $<

	
%.o: %.cu $(HEADER)
	$(NVCC) -c $(NVCCFLAGS) $(GENCODE_FLAGS)  -o $@ $<
	
all:	$(TARGET)

clean:
	rm -rf $(OBJ)
	rm -rf $(TARGET) animate

