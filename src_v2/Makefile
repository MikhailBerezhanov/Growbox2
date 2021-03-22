CCPP=g++
CC=gcc
CFLAGS=-Wall -Wno-format-security -Wno-unused-variable -Wno-unused-function

# Default value for application home path
DEFINES = -DAPP_DIR=\"$(APP_DIRECTORY)\"

BIN_NAME=gbweb

# PATHS
SRC_DIR= .
INCLUDES= -I./

SRC_FILES=$(SRC_DIR)main.cpp \
		  
LLIBS=-lfcgi -lsqlite3 -lconfig -lrt -lpthread

TARGET_BIN_DIR = ./bin
TARGET_OBJ_DIR = ./obj

TESTS_DIR=./tests

LOGER_TEST_BIN=$(TESTS_DIR)/loger.test
UTILS_TEST_BIN=$(TESTS_DIR)/utils.test
MAIN_TEST_BIN=$(TESTS_DIR)/main.test
HTTPSERV_TEST_BIN=$(TESTS_DIR)/httpserv.test
APP_TYPES_TEST_BIN=$(TESTS_DIR)/app_types.test
FLC_TEST_BIN=$(TESTS_DIR)/flc.test
SENSORS_TEST_BIN=$(TESTS_DIR)/sensors.test
CONFIG_TEST_BIN=$(TESTS_DIR)/config.test
APP_DB_TEST_BIN=$(TESTS_DIR)/app_db.test

.PHONY : clean install

# Prepare directories for output
prep:
	@if test ! -d $(TARGET_BIN_DIR); then mkdir $(TARGET_BIN_DIR); fi
	@if test ! -d $(TARGET_OBJ_DIR); then mkdir $(TARGET_OBJ_DIR); fi
	@if test ! -d $(TESTS_DIR); then mkdir $(TESTS_DIR); fi

loger-test: prep
	@rm -f $(LOGER_TEST_BIN)
	@$(CCPP) loger.cpp -DUNIT_TEST -o $(LOGER_TEST_BIN) -lpthread

utils-test: prep
	@rm -f $(UTILS_TEST_BIN)
	@$(CCPP) -c loger.cpp 
	@$(CCPP) -c utils.cpp -DUNIT_TEST
	@$(CCPP) -o $(UTILS_TEST_BIN) loger.o utils.o
	@rm -f ./*.o

main-test: prep
	@rm -f $(MAIN_TEST_BIN)
	@$(CCPP) loger.cpp main.cpp -o $(MAIN_TEST_BIN) -lfcgi -lpthread 

httpserv-test: prep
	@rm -f $(HTTPSERV_TEST_BIN)
	@$(CCPP) -c loger.cpp utils.cpp
	@$(CCPP) -c httpserv.cpp -DUNIT_TEST
	@$(CCPP) -o $(HTTPSERV_TEST_BIN) loger.o utils.o httpserv.o -lfcgi -lpthread
	@rm -f ./*.o

app_types-test: prep
	@rm -f $(APP_TYPES_TEST_BIN)
	@$(CCPP) -c loger.cpp
	@$(CCPP) -c app_types.cpp -DUNIT_TEST
	@$(CCPP) -o $(APP_TYPES_TEST_BIN) loger.o app_types.o
	@rm -f ./*.o

flc-test: prep
	@rm -f $(FLC_TEST_BIN)
	@$(CCPP) -c loger.cpp 
	@$(CCPP) -c flc.cpp -DUNIT_TEST
	@$(CCPP) -o $(FLC_TEST_BIN) loger.o flc.o
	@rm -f ./*.o

sensors-test: prep
	@rm -f $(SENSORS_TEST_BIN)
	@$(CCPP) -c loger.cpp utils.cpp
	@$(CCPP) -c sensors.cpp -DUNIT_TEST
	@$(CCPP) -o $(SENSORS_TEST_BIN) loger.o utils.o sensors.o
	@rm -f ./*.o

config-test: prep
	@rm -f $(CONFIG_TEST_BIN)
	@$(CCPP) -c loger.cpp utils.cpp
	@$(CCPP) -c app_cfg.cpp -DUNIT_TEST
	@$(CCPP) -o $(CONFIG_TEST_BIN) loger.o utils.o app_cfg.o -lconfig++
	@rm -f ./*.o

app_db-test: prep
	@rm -f $(APP_DB_TEST_BIN)
	@$(CCPP) -c loger.cpp utils.cpp
	@$(CCPP) -c app_db.cpp -DUNIT_TEST
	@$(CCPP) -o $(APP_DB_TEST_BIN) loger.o utils.o app_db.o -lsqlite3
	@rm -f ./*.o


# TODO
all: clean
	@LC_ALL=C $(CCPP) $(CFLAGS) -O0 $(DEFINES) \
	$(SRC_FILES) \
	-o $(EXECNAME) \
	$(INC) \
	$(LLIBS) 

debug: clean
	@LC_ALL=C $(CCPP) $(CFLAGS) -O3 -g $(DEFINES) \
	$(SRC_FILES) \
	-o $(EXECNAME) \
	$(INC) \
	$(LLIBS) 

####################################################################
$(TARGET_OBJ_DIR)/%.o:$(SRC_DIR)/%.cpp
	@echo Building OBJECT $(@) from C++ SOURCE $<
	@$(CCPP) $(CFLAGS) $(DEFINES) $(INCLUDES) -c -o $(@) $<

OBJS= 	$(TARGET_OBJ_DIR)/loger.o \
		$(TARGET_OBJ_DIR)/main.o

# Start linker
bin:$(OBJS)
	@echo Linking bin file $(BIN_NAME)
	@$(CCPP) -lm -o $(TARGET_BIN_DIR)/$(BIN_NAME) $^ $(LLIBS)
	@echo ---------------------------------------------------------
	@echo "\033[32m###### compile completely ######\033[0m"
####################################################################

run:
	@.$(TARGET_BIN_DIR)/$(BIN_NAME)

install:
	@sshpass -p "khadas" scp -r $(SRC) root@192.168.0.21:/home/mik/projects/gb_web/
	@sshpass -p "khadas" scp -r ./data/ root@192.168.0.21:/home/mik/projects/gb_web/

mem_check: debug
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./$(EXECNAME)
	



clean:
	@rm -fr $(TARGET_OBJ_DIR) $(TARGET_BIN_DIR)



