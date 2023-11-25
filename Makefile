SHARED_DIR = shared
CLIENT_DIR = client
SERVER_DIR = server
TESTER_DIR = test

SRC_DIR = src
OBJ_DIR = obj

APP_SERVER = app_server
APP_CLIENT = app_client
APP_TESTER = app_tester

SHARED_INCLUDE_FILES = $(wildcard $(SRC_DIR)/$(SHARED_DIR)/*.h)
CLIENT_INCLUDE_FILES = $(wildcard $(SRC_DIR)/$(CLIENT_DIR)/*.h)
SERVER_INCLUDE_FILES = $(wildcard $(SRC_DIR)/$(SERVER_DIR)/*.h)
TESTER_INCLUDE_FILES = $(wildcard $(SRC_DIR)/$(TESTER_DIR)/*.h)

SHARED_SRC_FILES = $(wildcard $(SRC_DIR)/$(SHARED_DIR)/*.cpp)
CLIENT_SRC_FILES = $(wildcard $(SRC_DIR)/$(CLIENT_DIR)/*.cpp)
SERVER_SRC_FILES = $(wildcard $(SRC_DIR)/$(SERVER_DIR)/*.cpp)
TESTER_SRC_FILES = $(wildcard $(SRC_DIR)/$(TESTER_DIR)/*.cpp)

SHARED_OBJ_FILES = $(patsubst $(SRC_DIR)/$(SHARED_DIR)/%.cpp, \
				   $(OBJ_DIR)/$(SHARED_DIR)/%.o, \
				   $(SHARED_SRC_FILES))
CLIENT_OBJ_FILES = $(patsubst $(SRC_DIR)/$(CLIENT_DIR)/%.cpp, \
				   $(OBJ_DIR)/$(CLIENT_DIR)/%.o, \
				   $(CLIENT_SRC_FILES))
SERVER_OBJ_FILES = $(patsubst $(SRC_DIR)/$(SERVER_DIR)/%.cpp, \
				   $(OBJ_DIR)/$(SERVER_DIR)/%.o, \
				   $(SERVER_SRC_FILES))
TESTER_OBJ_FILES = $(patsubst $(SRC_DIR)/$(TESTER_DIR)/%.cpp, \
				   $(OBJ_DIR)/$(TESTER_DIR)/%.o, \
				   $(TESTER_SRC_FILES))

LINK_FLAGS =
CXX = g++
CXX_FLAGS = -std=c++17 -g

app: $(APP_CLIENT) $(APP_SERVER)

test: $(APP_TESTER)
	./$<

$(APP_CLIENT): $(SHARED_OBJ_FILES) $(CLIENT_OBJ_FILES)
	$(CXX) -o $@ $^ $(CXX_FLAGS) $(LINK_FLAGS)

$(APP_SERVER): $(SHARED_OBJ_FILES) $(SERVER_OBJ_FILES)
	$(CXX) -o $@ $^ $(CXX_FLAGS) $(LINK_FLAGS)

$(APP_TESTER): \
	$(SHARED_OBJ_FILES) \
	$(subst $(OBJ_DIR)/$(CLIENT_DIR)/main.o, , $(CLIENT_OBJ_FILES)) \
	$(subst $(OBJ_DIR)/$(SERVER_DIR)/main.o, , $(SERVER_OBJ_FILES)) \
	$(TESTER_OBJ_FILES)
	$(CXX) -o $@ $^ $(CXX_FLAGS) $(LINK_FLAGS)

$(OBJ_DIR)/$(SHARED_DIR)/%.o: \
	$(SRC_DIR)/$(SHARED_DIR)/%.cpp \
	$(SHARED_INCLUDE_FILES)
	mkdir -p $(OBJ_DIR)/$(SHARED_DIR)
	$(CXX) -o $@ -c $< $(CXX_FLAGS)

$(OBJ_DIR)/$(CLIENT_DIR)/%.o: \
	$(SRC_DIR)/$(CLIENT_DIR)/%.cpp \
	$(SHARED_INCLUDE_FILES) \
	$(CLIENT_INCLUDE_FILES)
	mkdir -p $(OBJ_DIR)/$(CLIENT_DIR)
	$(CXX) -o $@ -c $< $(CXX_FLAGS)

$(OBJ_DIR)/$(SERVER_DIR)/%.o: \
	$(SRC_DIR)/$(SERVER_DIR)/%.cpp \
	$(SHARED_INCLUDE_FILES) \
	$(SERVER_INCLUDE_FILES)
	mkdir -p $(OBJ_DIR)/$(SERVER_DIR)
	$(CXX) -o $@ -c $< $(CXX_FLAGS)

$(OBJ_DIR)/$(TESTER_DIR)/%.o: \
	$(SRC_DIR)/$(TESTER_DIR)/%.cpp \
	$(SHARED_INCLUDE_FILES) \
	$(CLIENT_INCLUDE_FILES) \
	$(SERVER_INCLUDE_FILES) \
	$(TESTER_INCLUDE_FILES)
	mkdir -p $(OBJ_DIR)/$(TESTER_DIR)
	$(CXX) -o $@ -c $< $(CXX_FLAGS)

clean:
	rm -rf $(APP_CLIENT) $(APP_SERVER) $(OBJ_DIR)
