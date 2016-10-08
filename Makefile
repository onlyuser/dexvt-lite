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
BIN_STEMS = main
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

SCRIPT_PATH = test
TEST_PATH = test

#==================
# all
#==================

.DEFAULT_GOAL : all
all : $(BINARIES) resources

#==================
# for_travis
#==================

.PHONY : for_travis
for_travis : CXXFLAGS += -DNO_GLM_CONSTANTS

for_travis : all
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
	-rm $(OBJECTS)

#==================
# binaries
#==================

CPP_STEMS = BBoxObject Buffer Camera File3ds FrameBuffer IdentObject Light main Material Mesh NamedObject PrimitiveFactory Program res_texture res_texture2 Scene Shader ShaderContext shader_utils Texture Util VarAttribute VarUniform XformObject
OBJECTS = $(patsubst %, $(BUILD_PATH)/%.o, $(CPP_STEMS))

$(BIN_PATH)/main : $(OBJECTS)
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

3DS_MESH_PATH = $(RESOURCE_PATH)/star_wars
3DS_MESH_STEMS = TI_Low0
3DS_MESH_FILES = $(patsubst %, $(3DS_MESH_PATH)/%.3ds, $(3DS_MESH_STEMS))
$(3DS_MESH_FILES) :
	mkdir -p $(3DS_MESH_PATH)
	curl "http://www.jrbassett.com/zips/TI_Low0.Zip" > $(3DS_MESH_PATH)/TI_Low0.Zip
	unzip $(3DS_MESH_PATH)/TI_Low0.Zip -d $(3DS_MESH_PATH)
	rm $(3DS_MESH_PATH)/TI_Low0.Zip
	rm $(3DS_MESH_PATH)/*.txt

resources : $(CHESTERFIELD_MAP_FILES) $(CUBE_MAP_FILES) $(3DS_MESH_FILES)

.PHONY : clean_resources
clean_resources :
	-rm $(CHESTERFIELD_MAP_FILES) $(CUBE_MAP_FILES) $(3DS_MESH_FILES)
	-rm -rf $(RESOURCE_PATH)

#==================
# clean
#==================

.PHONY : clean
clean : clean_binaries clean_objects clean_tests #clean_resources
	-rmdir $(BUILD_PATH) $(BIN_PATH)
