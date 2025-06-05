print-%:; @echo $($*)

UNAME = $(shell uname -s)

CCFLAGS =
LDFLAGS =

ifeq ($(UNAME), Darwin)
	CC = $(shell brew --prefix llvm)/bin/clang
	LD = $(shell brew --prefix llvm)/bin/clang

	CCFLAGS += -I$(shell brew --prefix llvm)/include
	LDFLAGS += -L$(shell brew --prefix llvm)/lib
else
	CC = clang
	LD = clang
endif

ifeq ($(UNAME), Darwin)
	SHDC = lib/sokol-tools/osx-ninja-release/sokol-shdc
else
	# TODO
endif

# library paths
PATH_LIB	   = lib
PATH_SDL       = $(PATH_LIB)/SDL
PATH_SDL_IMAGE = $(PATH_LIB)/SDL_image
PATH_CIMGUI    = $(PATH_LIB)/cimgui
PATH_SOLOUD    = $(PATH_LIB)/soloud
PATH_CGLM      = $(PATH_LIB)/cglm
PATH_SOKOL     = $(PATH_LIB)/sokol
PATH_SOKOL_GP  = $(PATH_LIB)/sokol_gp
PATH_GLUTESS   = $(PATH_LIB)/glutess
PATH_GLAD      = $(PATH_LIB)/glad

INCFLAGS = -iquotesrc

INCFLAGS += -I$(PATH_SDL)/include
INCFLAGS += -I$(PATH_SDL_IMAGE)/include
INCFLAGS += -I$(PATH_CIMGUI)
INCFLAGS += -I$(PATH_CIMGUI)/generator/output
INCFLAGS += -I$(PATH_SOLOUD)/include
INCFLAGS += -I$(PATH_CGLM)/include
INCFLAGS += -I$(PATH_SOKOL)
INCFLAGS += -I$(PATH_SOKOL_GP)
INCFLAGS += -I$(PATH_GLUTESS)
INCFLAGS += -I$(PATH_GLAD)/include

CCFLAGS += -std=c2x
CCFLAGS += -O1
CCFLAGS += -g
CCFLAGS += -Wall
CCFLAGS += -Wextra
CCFLAGS += -Wpedantic
CCFLAGS += -Wno-gnu
CCFLAGS += -Wno-format-nonliteral
CCFLAGS += -Wno-unused-parameter
CCFLAGS += -fno-omit-frame-pointer
CCFLAGS += -fsanitize=undefined,address,leak

LDFLAGS += -lm
LDFLAGS += -fsanitize=undefined,address,leak

### TARGET SPECIFICS ###

# TODO : add more targets
# possible values : osx_arm64, osx_x86_64 < add more>
ifeq ($(UNAME), Darwin)
	TARGET = osx_arm64
else
	TARGET = linux
endif

ifeq ($(TARGET),osx_arm64)
	GAME_PLATFORM 	 = osx
	GAME_TARGET 	 = osx_arm64
	LDFLAGS 	+= -framework OpenGL
else
	GAME_PLATFORM	 = linux
	GAME_TARGET 	 = linux
	CCFLAGS 	+= -D_GNU_SOURCE
endif

### END TARGET SPECIFICS ###

# add defines for specific platforms, shader targets
CCFLAGS += -DGAME_PLATFORM_$(GAME_PLATFORM)
CCFLAGS += -DGAME_TARGET_$(GAME_TARGET)

LDFLAGS += -lstdc++

LDFLAGS_STATIC = $(LDFLAGS)
LDFLAGS_STATIC += $(shell $(BIN)/sdl/sdl2-config --prefix=$(BIN) --static-libs)
LDFLAGS_STATIC += $(BIN)/lib/libSDL2.a
LDFLAGS_STATIC += $(BIN)/lib/libSDL2_Image.a
LDFLAGS_STATIC += $(BIN)/lib/cimgui.a
LDFLAGS_STATIC += $(BIN)/lib/libimguibackend.a
LDFLAGS_STATIC += $(BIN)/lib/libsoloud.a
LDFLAGS_STATIC += $(BIN)/lib/libsoloudc.a
LDFLAGS_STATIC += $(BIN)/lib/libglutess.a

LDFLAGS_SHARED = $(LDFLAGS)
LDFLAGS_SHARED += -Lbin/lib
LDFLAGS_SHARED += -lSDL2-2.0
LDFLAGS_SHARED += -lSDL2_Image-2.0
LDFLAGS_SHARED += bin/lib/libsoloud.dylib
LDFLAGS_SHARED += bin/lib/libsoloudc.dylib
LDFLAGS_SHARED += -limguibackend
LDFLAGS_SHARED += bin/lib/cimgui.dylib
LDFLAGS_SHARED += -Wl,-rpath -Wl,bin/lib

ROOT      = src
BIN  	  = bin
SRC  	  = $(shell find $(ROOT) -name "*.c") $(PATH_GLAD)/src/glad.c
OBJ 	  = $(SRC:%.c=$(BIN)/%.o)
DEP  	  = $(SRC:%.c=$(BIN)/%.d)
OUT  	  = $(BIN)/game
OUTSHARED = $(BIN)/game.dylib

UTIL 	= $(shell find util -name "*.c")
UTILDEP = $(UTIL:%.c=$(BIN)/%.d)
UTILOUT = $(UTIL:%.c=$(BIN)/%)

CCUTIL 	= $(shell find util -name "*.cc")
CCUTILDEP = $(CCUTIL:%.cc=$(BIN)/%.d)
CCUTILOUT = $(CCUTIL:%.cc=$(BIN)/%)

TEST 	= $(shell find test -name "*.c")
TESTDEP = $(TEST:%.c=$(BIN)/%.d)
TESTOUT = $(TEST:%.c=$(BIN)/%)

TESTEXE = bin/test/runner

SHADERS = $(ROOT)/shader

SHDSRC = $(shell find $(SHADERS) -name "*.glsl")
SHDOUT = $(SHDSRC:%.glsl=%.glsl.h)

$(SHDOUT): %.glsl.h: %.glsl
	$(SHDC) --input $^ --output $@ --slang glsl300es:glsl330 --format=sokol_impl

shaders: $(SHDOUT)

-include $(DEP) $(UTILDEP) $(TESTDEP)

export LD_LIBRARY_PATH=bin/lib
export LIBRARY_PATH=bin/lib

all: dirs libs build util test

$(BIN):
	$(shell mkdir -p  $@)

dirs: $(BIN) FORCE
	$(shell mkdir -p $(BIN)/src)
	$(shell mkdir -p $(BIN)/util)
	$(shell mkdir -p $(BIN)/test)
	$(shell mkdir -p $(BIN)/ext)
	$(shell mkdir -p $(BIN)/lib)
	$(shell mkdir -p $(BIN)/lib/glutess)
	$(shell mkdir -p $(BIN)/lib/glad/src)
	$(shell mkdir -p $(BIN)/lib/sokol/util)
	rsync -a --include '*/' --exclude '*' "src" "bin"

lib-sdl:
	$(shell mkdir -p  $(BIN)/sdl)
	$(shell mkdir -p  $(BIN)/lib)
	cmake -S $(PATH_SDL) -B $(BIN)/sdl -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
	cd $(BIN)/sdl && make -j 10
	chmod +x $(BIN)/sdl/sdl2-config
	cp $(BIN)/sdl/libSDL2-2.0.dylib $(BIN)/lib
	cp $(BIN)/lib/libSDL2-2.0.dylib $(BIN)/lib/libSDL2.dylib
	cp $(BIN)/lib/libSDL2-2.0.dylib $(BIN)/lib/libSDL2-2.0.0.dylib
	cp $(BIN)/sdl/libSDL2.a $(BIN)/lib

lib-sdl-image:
	$(shell mkdir -p  $(BIN)/sdl-image)
	$(shell mkdir -p  $(BIN)/lib)
	cmake -S $(PATH_SDL_IMAGE) -B $(BIN)/sdl-image -DBUILD_SHARED_LIBS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
	cd $(BIN)/sdl-image && make -j 1
	cmake -S $(PATH_SDL_IMAGE) -B $(BIN)/sdl-image -DBUILD_SHARED_LIBS=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
	cd $(BIN)/sdl-image && make -j 10
	cp $(BIN)/sdl-image/libSDL2_Image-2.0.dylib $(BIN)/lib
	cp $(BIN)/lib/libSDL2_Image-2.0.dylib $(BIN)/lib/libSDL2_Image.dylib
	cp $(BIN)/lib/libSDL2_Image-2.0.dylib $(BIN)/lib/libSDL2_Image-2.0.701.0.0.dylib
	cp $(BIN)/sdl-image/libSDL2_Image.a $(BIN)/lib

lib-cimgui: lib-sdl
	$(shell mkdir -p $(BIN)/cimgui)
	$(shell mkdir -p $(BIN)/lib)
	$(shell mkdir -p $(BIN)/ext)
	cmake -S $(PATH_CIMGUI) -B $(BIN)/cimgui -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
	cd $(BIN)/cimgui && make
	cp $(BIN)/cimgui/cimgui.dylib $(BIN)/lib
	cmake -S $(PATH_CIMGUI) -B $(BIN)/cimgui -DIMGUI_STATIC=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 
	cd $(BIN)/cimgui && make
	cp $(BIN)/cimgui/cimgui.a $(BIN)/lib
	$(CC) -o $(BIN)/ext/imgui_backend_sdl.o -c -g 						 		 \
		-Ilib/cimgui 														 \
		-iquote$(PATH_CIMGUI)/imgui 										 \
		-Ilib/SDL/include 													 \
		ext/imgui_backend_sdl.cpp
	$(CC) -o $(BIN)/ext/cimgui.o -c -g 						 				 \
		-Ilib/cimgui 														 \
		-iquote$(PATH_CIMGUI)/imgui 										 \
		-Ilib/SDL/include 													 \
		ext/cimgui.cpp
	$(LD) -shared -o $(BIN)/lib/libimguibackend.dylib -g 						 \
		-Wl,-rpath,bin/lib/libimguibackend.dylib                             \
		-Lbin/lib                                                            \
		-lstdc++ 															 \
		bin/lib/cimgui.dylib                                                 \
		-lSDL2-2.0                                                           \
		$(BIN)/ext/imgui_backend_sdl.o 								 	     \
		$(BIN)/ext/cimgui.o
	ar rcs $(BIN)/lib/libimguibackend.a\
		$(BIN)/ext/imgui_backend_sdl.o 								 	     \
		$(BIN)/ext/cimgui.o

lib-soloud:
	cd $(PATH_SOLOUD) 						                                 \
		&& genie --file=build/genie.lua gmake                                \
		&& cd build/gmake 					                                 \
		&& make SoloudDynamic SoloudStatic -j 10
	$(CC) -o $(BIN)/ext/soloud_c.o -c 										 \
		-I$(PATH_SOLOUD)/include                                             \
		$(PATH_SOLOUD)/src/c_api/soloud_c.cpp
	$(shell mkdir -p $(BIN)/lib)
	cp $(PATH_SOLOUD)/lib/libsoloud.dylib $(BIN)/lib
	cp $(PATH_SOLOUD)/lib/libsoloud_static.a $(BIN)/lib/libsoloud.a
	install_name_tool -id bin/lib/libsoloud.dylib bin/lib/libsoloud.dylib
	$(LD) -shared -o $(BIN)/lib/libsoloudc.dylib 							 \
		-Wl,-rpath,bin/lib/libsoloudc.dylib                                  \
		-Lbin/lib 														     \
		bin/lib/libsoloud.dylib                                              \
		-lstdc++                                                             \
		$(BIN)/ext/soloud_c.o
	ar rcs $(BIN)/lib/libsoloudc.a\
		$(BIN)/ext/soloud_c.o

GLUTESS_SRC = $(shell find $(PATH_GLUTESS) -name "*.c")
GLUTESS_OBJ = $(GLUTESS_SRC:$(PATH_GLUTESS)/%.c=$(BIN)/lib/glutess/%.o)

$(GLUTESS_OBJ): $(BIN)/lib/glutess/%.o: $(PATH_GLUTESS)/%.c
	$(CC) -o $@ -c $(CCFLAGS) -O3 $<

lib-glutess: dirs $(GLUTESS_OBJ)
	ar rcs bin/lib/libglutess.a $(GLUTESS_OBJ)

libs: dirs lib-sdl lib-sdl-image lib-cimgui lib-soloud lib-glutess

$(UTILOUT): $(BIN)/%: %.c
	$(CC) -o $@ -MMD $(CCFLAGS) $(INCFLAGS) $<

$(CCUTILOUT): $(BIN)/%: %.cc
	$(CC) -o $@ -MMD -lstdc++ $(INCFLAGS) $<

util: $(UTILOUT) $(CCUTILOUT)

$(TESTOUT): $(BIN)/%: %.c
	$(CC) -o $@ -MMD $(CCFLAGS) $(INCFLAGS) $<

test: $(TESTOUT)
	$(TESTEXE) bin/test

$(BIN)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) $(INCFLAGS) -MMD -MP -c $< -o $@

run: build
	DYLD_LIBRARY_PATH=bin/lib $(OUT)

run-reloadable: util build-shared
	$(BIN)/util/reloadhost $(OUTSHARED)

debug-reloadable: util build-shared
	lldb $(BIN)/util/reloadhost -o 'run $(OUTSHARED)'

build-shared: dirs $(OBJ)
	$(LD) -shared -o $(OUTSHARED) $(filter %.o,$^) $(LDFLAGS_SHARED)

build: dirs shaders $(OBJ)
	$(LD) -o $(OUT) $(filter %.o,$^) $(LDFLAGS_STATIC)

add-code:
	git add .

commit-code: add-code
	git commit -a -m "I was done from markdown"

push-code: commit-code
	git push --force

clean-obj:
	rm -rf bin/src bin/util bin/test

clean-deps:
	rm $(DEP) $(UTILDEP) $(TESTDEP)

clean-libs:
	 find bin ! -name 'src' ! -name 'util' -type d -maxdepth 1 -mindepth 1 -exec rm -rf {} +

clean-shaders:
	find $(SHADERS) -name '*.glsl.h' -exec rm -rf {} +

clean: clean-obj clean-shaders clean-libs

FORCE: ;
