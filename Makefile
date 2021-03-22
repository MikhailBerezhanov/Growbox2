CCPP=g++
CC=gcc
CFLAGS=-Wall -Wno-format-security -Wno-unused-variable -Wno-unused-function

# Default value for application home path
DEFINES = -DAPP_DIR=\"$(APP_DIRECTORY)\"
DEFINES += -DAPP_MULTITHREADED=1
DEFINES += -DDEBUG_LOG_FILE_SUPPORT=1

EXECNAME=gbweb

# PATHS
SRC=./src/
INC=-I./src/

SRC_FILES=$(SRC)main.cpp \
		  $(SRC)utils.c \
		  $(SRC)httpserv.cpp \
		  $(SRC)app_cl.cpp \
		  $(SRC)app_th.cpp \
		  $(SRC)app.cpp \
		  $(SRC)jsont.c \
		  $(SRC)cJSON/cJSON.c \
		  $(SRC)flc.cpp \
		  $(SRC)sensors.cpp \
		  $(SRC)mydb.cpp \
		  $(SRC)config.cpp \
		  $(SRC)dbgmsg.cpp \
		  
LLIBS=-lfcgi -lpthread -lsqlite3 -lconfig -lrt


.PHONY : clean install

all: clean
	@LC_ALL=C $(CCPP) $(CFLAGS) -O0 $(DEFINES) \
	$(SRC_FILES) \
	-o $(EXECNAME) \
	$(INC) \
	$(LLIBS) 

debug: clean
	@LC_ALL=C $(CC) $(CFLAGS) -O3 -g $(DEFINES) \
	$(SRC_FILES) \
	-o $(EXECNAME) \
	$(INC) \
	$(LLIBS) 

run:
	@./$(EXECNAME)

install:
	@sshpass -p "khadas" scp -r $(SRC) root@192.168.0.21:/home/mik/projects/gb_web/
	@sshpass -p "khadas" scp -r ./data/ root@192.168.0.21:/home/mik/projects/gb_web/

mem_check: debug
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-out.txt ./$(EXECNAME)
	
service:
	@sudo mv ./systemd_units/growbox.service /etc/systemd/system/
	@sudo systemctl daemon-reload

update:
	@sudo systemctl restart gbweb

clean:
	@rm -f ./$(EXECNAME)
