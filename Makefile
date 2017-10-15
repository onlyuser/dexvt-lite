#==================
# compile flags
#==================

PARENT = ..
LIB_SRC_PATH = .
INCLUDE_PATH = include
LIB_PATH = $(EXTERN_LIB_PATH)
SRC_PATH = src
BUILD_PATH = build
BIN_PATH = bin
BIN_STEMS = main_ik main_ik_const main_boids main_hexapod main_deltabot main_3dprinter main_terrain main_spider main_freerot main_rail main_stewart main_fanta
BINARIES = $(patsubst %, $(BIN_PATH)/%, $(BIN_STEMS))

INCLUDE_PATHS = $(INCLUDE_PATH) $(EXTERN_INCLUDE_PATH)
INCLUDE_PATH_FLAGS = $(patsubst %, -I%, $(INCLUDE_PATHS))

LIB_PATHS = $(LIB_PATH)
LIB_PATH_FLAGS = $(patsubst %, -L%, $(LIB_PATHS))

LIB_STEMS = glut GLEW GL png
LIBS = $(patsubst %, $(LIB_PATH)/lib%.a, $(LIB_STEMS))
LIB_FLAGS = $(patsubst %, -l%, $(LIB_STEMS))

CXX = g++
DEBUG = -g
CXXFLAGS = -Wall $(DEBUG) $(INCLUDE_PATH_FLAGS) -std=c++0x
LDFLAGS = -Wall $(DEBUG) $(LIB_PATH_FLAGS) $(LIB_FLAGS)

SCRIPT_PATH = scripts

#==================
# all
#==================

.DEFAULT_GOAL : all
all : $(BINARIES) resources

#==================
# bin_only
#==================

.PHONY : bin_only
bin_only : $(BINARIES)

#==================
# for_travis
#==================

.PHONY : for_travis
for_travis : CXXFLAGS += -DNO_GLM_CONSTANTS

for_travis : bin_only
	@echo TARGET=$@ CXXFLAGS=${CXXFLAGS}

#==================
# objects
#==================

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.cpp
	mkdir -p $(BUILD_PATH)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(BUILD_PATH)/%.o : $(SRC_PATH)/%.c
	mkdir -p $(BUILD_PATH)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

.PHONY : clean_objects
clean_objects :
	-rm $(OBJECTS_IK) $(OBJECTS_IK_CONST) $(OBJECTS_BOIDS) $(OBJECTS_HEXAPOD) $(OBJECTS_DELTABOT) $(OBJECTS_3DPRINTER) $(OBJECTS_3DPRINTER) $(OBJECTS_TERRAIN) $(OBJECTS_SPIDER) $(OBJECTS_FREEROT) $(OBJECTS_RAIL) $(OBJECTS_STEWART) $(OBJECTS_FANTA)

#==================
# binaries
#==================

SHARED_CPP_STEMS = BBoxObject \
                   Buffer \
                   Camera \
                   File3ds \
                   FrameBuffer \
                   IdentObject \
                   KeyframeMgr \
                   Light \
                   Modifiers \
                   Material \
                   Mesh \
                   NamedObject \
                   Octree \
                   PrimitiveFactory \
                   Program \
                   Scene \
                   Shader \
                   ShaderContext \
                   shader_utils \
                   Texture \
                   Util \
                   VarAttribute \
                   VarUniform \
                   TransformObject
CPP_STEMS_IK        = $(SHARED_CPP_STEMS) main_ik
CPP_STEMS_IK_CONST  = $(SHARED_CPP_STEMS) main_ik_const
CPP_STEMS_BOIDS     = $(SHARED_CPP_STEMS) main_boids
CPP_STEMS_HEXAPOD   = $(SHARED_CPP_STEMS) main_hexapod
CPP_STEMS_DELTABOT  = $(SHARED_CPP_STEMS) main_deltabot
CPP_STEMS_3DPRINTER = $(SHARED_CPP_STEMS) main_3dprinter
CPP_STEMS_TERRAIN   = $(SHARED_CPP_STEMS) main_terrain
CPP_STEMS_SPIDER    = $(SHARED_CPP_STEMS) main_spider
CPP_STEMS_FREEROT   = $(SHARED_CPP_STEMS) main_freerot
CPP_STEMS_RAIL      = $(SHARED_CPP_STEMS) main_rail
CPP_STEMS_STEWART   = $(SHARED_CPP_STEMS) main_stewart
CPP_STEMS_FANTA     = $(SHARED_CPP_STEMS) main_fanta
OBJECTS_IK        = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_IK))
OBJECTS_IK_CONST  = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_IK_CONST))
OBJECTS_BOIDS     = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_BOIDS))
OBJECTS_HEXAPOD   = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_HEXAPOD))
OBJECTS_DELTABOT  = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_DELTABOT))
OBJECTS_3DPRINTER = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_3DPRINTER))
OBJECTS_TERRAIN   = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_TERRAIN))
OBJECTS_SPIDER    = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_SPIDER))
OBJECTS_FREEROT   = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_FREEROT))
OBJECTS_RAIL      = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_RAIL))
OBJECTS_STEWART   = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_STEWART))
OBJECTS_FANTA     = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS_FANTA))
LINT_FILES        = $(patsubst %, $(BUILD_PATH)/%.lint, $(SHARED_CPP_STEMS))

$(BIN_PATH)/main_ik : $(OBJECTS_IK)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_ik_const : $(OBJECTS_IK_CONST)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_boids : $(OBJECTS_BOIDS)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_hexapod : $(OBJECTS_HEXAPOD)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_deltabot : $(OBJECTS_DELTABOT)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_3dprinter : $(OBJECTS_3DPRINTER)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_terrain : $(OBJECTS_TERRAIN)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_spider : $(OBJECTS_SPIDER)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_freerot : $(OBJECTS_FREEROT)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_rail : $(OBJECTS_RAIL)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_stewart : $(OBJECTS_STEWART)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)
$(BIN_PATH)/main_fanta : $(OBJECTS_FANTA)
	mkdir -p $(BIN_PATH)
	$(CXX) -o $@ $^ $(LDFLAGS)

.PHONY : clean_binaries
clean_binaries :
	-rm $(BINARIES)

#==================
# test
#==================

TEST_PATH = test
TEST_SH = $(SCRIPT_PATH)/test.sh
TEST_FILE_STEMS = main
TEST_FILES = $(patsubst %, $(BUILD_PATH)/%.test, $(TEST_FILE_STEMS))
TEST_PASS_FILES = $(patsubst %, %.pass, $(TEST_FILES))
TEST_FAIL_FILES = $(patsubst %, %.fail, $(TEST_FILES))

$(BUILD_PATH)/main.test.pass : $(BIN_PATH)/main $(TEST_PATH)/main.test
	-$(TEST_SH) $(BIN_PATH)/main arg $(TEST_PATH)/main.test $(TEST_PATH)/main.gold \
			$(BUILD_PATH)/main.test

.PHONY : test
test : $(TEST_PASS_FILES)

.PHONY : clean_tests
clean_tests :
	-rm $(TEST_PASS_FILES) $(TEST_FAIL_FILES)

#==================
# lint
#==================

LINT_PASS_FILES = $(patsubst %, %.pass, $(LINT_FILES))
LINT_FAIL_FILES = $(patsubst %, %.fail, $(LINT_FILES))
LINT_SH = $(SCRIPT_PATH)/lint.sh

$(BUILD_PATH)/%.lint.pass : $(SRC_PATH)/%.c*
	mkdir -p $(BUILD_PATH)
	-$(LINT_SH) $< $(BUILD_PATH)/$*.lint $(INCLUDE_PATH_FLAGS)

.PHONY : lint
lint : $(LINT_PASS_FILES)

.PHONY : clean_lint
clean_lint :
	-rm $(LINT_PASS_FILES) $(LINT_FAIL_FILES)

#==================
# docs
#==================

DOC_PATH = docs
DOC_CONFIG_FILE = dexvt-lite.config
DOC_CONFIG_PATCH_FILE = $(DOC_CONFIG_FILE).patch
DOC_TOOL = doxygen

.PHONY : docs
docs :
	mkdir -p $(BUILD_PATH)
	doxygen -g $(BUILD_PATH)/$(DOC_CONFIG_FILE)
	patch $(BUILD_PATH)/$(DOC_CONFIG_FILE) < $(DOC_PATH)/$(DOC_CONFIG_PATCH_FILE)
	cd $(BUILD_PATH); $(DOC_TOOL) $(DOC_CONFIG_FILE)

.PHONY : clean_docs
clean_docs :
	rm -rf $(BUILD_PATH)/html
	rm -rf $(BUILD_PATH)/$(DOC_CONFIG_FILE)

#==================
# resources
#==================

RESOURCE_PATH = ./data

CHESTERFIELD_MAP_PATH = $(RESOURCE_PATH)
CHESTERFIELD_MAP_STEMS = chesterfield_color chesterfield_normal
CHESTERFIELD_MAP_FILES = $(patsubst %, $(CHESTERFIELD_MAP_PATH)/%.png, $(CHESTERFIELD_MAP_STEMS))
$(CHESTERFIELD_MAP_FILES) :
	mkdir -p $(CHESTERFIELD_MAP_PATH)
	curl "http://4.bp.blogspot.com/-TsdyluFHn6I/ULI2Hzo8BfI/AAAAAAAAAWU/UleYFCG9SQs/s1600/Well+Preserved+Chesterfield.png" > $(CHESTERFIELD_MAP_PATH)/chesterfield_color.png
	curl "http://2.bp.blogspot.com/-4DZbxSZTWUQ/ULI1vc7GKHI/AAAAAAAAAVo/-vuKDtSxUGY/s1600/Well+Preserved+Chesterfield+-+(Normal+Map_2).png" > $(CHESTERFIELD_MAP_PATH)/chesterfield_normal.png

CUBE_MAP_PATH = $(RESOURCE_PATH)/SaintPetersSquare2
CUBE_MAP_STEMS = negx negy negz posx posy posz
CUBE_MAP_FILES = $(patsubst %, $(CUBE_MAP_PATH)/%.png, $(CUBE_MAP_STEMS))
$(CUBE_MAP_FILES) :
	mkdir -p $(CUBE_MAP_PATH)
	curl "http://www.humus.name/Textures/SaintPetersSquare2.zip" > $(CUBE_MAP_PATH)/SaintPetersSquare2.zip
	unzip $(CUBE_MAP_PATH)/SaintPetersSquare2.zip -d $(CUBE_MAP_PATH)
	mogrify -format png $(CUBE_MAP_PATH)/*.jpg
	rm $(CUBE_MAP_PATH)/SaintPetersSquare2.zip
	rm $(CUBE_MAP_PATH)/*.jpg
	rm $(CUBE_MAP_PATH)/*.txt

HEIGHT_MAP_PATH = $(RESOURCE_PATH)
HEIGHT_MAP_STEMS = heightmap
HEIGHT_MAP_FILES = $(patsubst %, $(HEIGHT_MAP_PATH)/%.png, $(HEIGHT_MAP_STEMS))
$(HEIGHT_MAP_FILES) :
	mkdir -p $(HEIGHT_MAP_PATH)
	curl "https://upload.wikimedia.org/wikipedia/commons/5/57/Heightmap.png" > $(HEIGHT_MAP_PATH)/heightmap.png

3DS_MESH_PATH = $(RESOURCE_PATH)/star_wars
3DS_MESH_STEMS = TI_Low0
3DS_MESH_FILES = $(patsubst %, $(3DS_MESH_PATH)/%.3ds, $(3DS_MESH_STEMS))
$(3DS_MESH_FILES) :
	mkdir -p $(3DS_MESH_PATH)
	curl "http://www.jrbassett.com/zips/TI_Low0.Zip" > $(3DS_MESH_PATH)/TI_Low0.Zip
	unzip $(3DS_MESH_PATH)/TI_Low0.Zip -d $(3DS_MESH_PATH)
	rm $(3DS_MESH_PATH)/TI_Low0.Zip
	rm $(3DS_MESH_PATH)/*.txt

resources : $(CHESTERFIELD_MAP_FILES) $(CUBE_MAP_FILES) $(HEIGHT_MAP_FILES) $(3DS_MESH_FILES)

.PHONY : clean_resources
clean_resources :
	-rm $(CHESTERFIELD_MAP_FILES) $(CUBE_MAP_FILES) $(HEIGHT_MAP_FILES) $(3DS_MESH_FILES)
	-rm -rf $(RESOURCE_PATH)

#==================
# clean
#==================

.PHONY : clean
clean : clean_binaries clean_objects clean_tests #clean_lint #clean_docs #clean_resources
	-rmdir $(BUILD_PATH) $(BIN_PATH)
