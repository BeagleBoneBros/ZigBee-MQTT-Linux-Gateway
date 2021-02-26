# Makefile for Network Manager Server

# ARM Protobuf directories - Fill these in with the name of your protobuf-c directory
PROTOC_INC_DIR = $(PROTOINC)
PROTOC_LIB_PATH = -L../protobuf-c/lib/$(PLATFORM)

# Compilation tools - make sure to change the directory if its not called ti-sdk-am... 
ARM_LIB_PATH = -L$(TCLIB)

# Variables for path locations
COMPONENTS = ../../../../Components
PROJECTS = ../../../../Projects

# Compilation outputl
OBJS = out

# Library to use
LIBS= -lpthread -lprotobuf-c

# Include paths
INCLUDES = -I ../zstackpb \
	-I ../hal \
	-I . \
	-I $(COMPONENTS)/zmac \
	-I $(COMPONENTS)/mac/include \
	-I $(COMPONENTS)/stack/af \
    -I $(COMPONENTS)/stack/nwk \
    -I $(COMPONENTS)/stack/sec \
    -I $(COMPONENTS)/stack/sys \
    -I $(COMPONENTS)/stack/zdo \
	-I $(COMPONENTS)/stack/zcl \
	-I $(PROJECTS)/zstack/linux/nwkmgr \
	-I $(PROJECTS)/zstack/linux/serverpb \
	-I $(PROJECTS)/zstack/linux/srvepconfig \
	-I $(PROJECTS)/zstack/linux/sdb \
	-I $(PROJECTS)/zstack/linux/srvwrapper \
	-I $(PROJECTS)/zstack/linux/hagateway \
	-I $(PROJECTS)/zstack/linux/linux_osal/include \
	-I $(PROJECTS)/zstack/linux/hal \
	-I $(PROJECTS)/zstack/linux/zmac \
	-I $(PROJECTS)/zstack/linux/protobuf-c/include/google/protobuf-c \
	-I $(PROJECTS)/zstack/linux/source \
	-I $(PROJECTS)/zstack/linux/zstackserverznp \
	-I $(PROJECTS)/zstack/linux/serverpb

# Predefine
DEFINES = \
 	-D_GNU_SOURCE \
 	-D__USE_UNIX98 \
 	-DNWKMGR_SRVR \
 	-DMAX_BINDING_CLUSTER_IDS \
 	-DZCL_ON_OFF \
 	-DZCL_STANDALONE \
	-DSERVER_NAME=NWK_MGR \
	-D__APP_UI__


# Compilation Option
COMPILO_FLAGS_x86 = "-Wall $(INCLUDES) $(DEFINES) -include ../hal/hal_types.h -include f8wConfig.h -m32" 
COMPILO_FLAGS_arm = "-Wall $(INCLUDES) $(DEFINES) -include ../hal/hal_types.h -include f8wConfig.h -mcpu=cortex-a8 -march=armv7-a -I $(PROTOC_INC_DIR)"

# List of object file to compile
APP_OBJS= \
	$(OBJS)/serverep.o \
	$(OBJS)/nwkmgrsrv.o \
	$(OBJS)/nwkmgrdatabase.o \
	$(OBJS)/trace.o \
	$(OBJS)/SimpleDB.o \
	$(OBJS)/SimpleDBTxt.o \
	$(OBJS)/nwkmgrservices.o \
	$(OBJS)/nwkmgrp2p.o \
	$(OBJS)/nwkmgr.pb-c.o \
	$(OBJS)/zstack.pb-c.o \
	$(OBJS)/server.pb-c.o \
	$(OBJS)/api_client.o \
	$(OBJS)/main.o \
	$(OBJS)/api_server.o \
	$(OBJS)/configparser.o \
	$(OBJS)/pb_utils.o \
	$(OBJS)/zstack_pb_ctrl.o \
	$(OBJS)/nwkmgr_pb_ctrl.o \
	$(OBJS)/server_pb_ctrl.o

.PHONY: all clean lib create_output arch-all-x86 arch-all-arm clean_obj clean_obj2

all: \
	create_output \
	clean_obj \
	arch-all-x86 \
	clean_obj2 \
	arch-all-arm \

create_output:
	@echo "********************************************************" 
	@echo "Check existance of output folder" 
	-if test ! -d $(OBJS); then mkdir $(OBJS); fi

arch-all-x86: 
	@echo "********************************************************" 
	@echo "COMPILING APP FOR x86" 
	@$(MAKE) COMPILO=$(COMPILER) COMPILO_FLAGS=$(COMPILO_FLAGS_x86) exec_app_x86

arch-all-arm: 
	@echo "********************************************************" 
	@echo "COMPILING APP FOR ARM" 
	@$(MAKE) COMPILO=$(COMPILER) COMPILO_FLAGS=$(COMPILO_FLAGS_arm) exec_app_arm

arch-all-arago: 
	@echo "********************************************************" 
	@echo "COMPILING APP FOR ARAGO" 
	@$(MAKE) COMPILO=$(COMPILER) COMPILO_FLAGS=$(COMPILO_FLAGS_arm) exec_app_arago

exec_app_x86: $(OBJS)/NWKMGR_SRVR_x86

exec_app_arm: $(OBJS)/NWKMGR_SRVR_arm

exec_app_arago: $(OBJS)/NWKMGR_SRVR_arago

clean_obj:
	@echo "********************************************************" 
	@echo "Cleaning Existing object" 
	-if test -d $(OBJS); then rm -f $(OBJS)/*.o; fi

clean_obj2:
	@echo "********************************************************" 
	@echo "Cleaning Existing object" 
	-if test -d $(OBJS); then rm -f $(OBJS)/*.o; fi

clean:
	@echo "********************************************************" 
	@echo "Cleaning all" 
	-if test -d $(OBJS); then rm -rf $(OBJS); fi
	/bin/rm -f project_file_list.txt
	/bin/rm -f actual_standard_project_file_list.txt
	/bin/rm -f actual_specific_project_file_list.txt

$(OBJS)/NWKMGR_SRVR_arm: $(APP_OBJS)
	@echo "Building target" $@ "..."
	@$(COMPILO) -rdynamic -o $@ $(APP_OBJS) $(ARM_LIB_PATH) $(PROTOC_LIB_PATH) $(LIBS) 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | ../scripts/extract_files >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/NWKMGR_SRVR_arago: $(APP_OBJS)
	@echo "Building target" $@ "..."
	@$(COMPILO) -rdynamic -o $@ $(APP_OBJS) $(ARM_LIB_PATH) $(PROTOC_LIB_PATH) $(LIBS) 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | ../scripts/extract_files >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/NWKMGR_SRVR_x86: $(APP_OBJS)
	@echo "Building target" $@ "..."
	@$(COMPILO) -rdynamic -o $@ $(APP_OBJS) -m32 $(PROTOC_LIB_PATH) $(LIBS)
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 


# Start of Object file creation
$(OBJS)/zstack.pb-c.o: ../zstackpb/zstack.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/nwkmgr.pb-c.o: nwkmgr.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/server.pb-c.o: ../serverpb/server.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<		
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/api_client.o: ../srvwrapper/api_client.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/main.o: ../srvwrapper/main.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/api_server.o: ../srvwrapper/api_server.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/configparser.o: ../srvwrapper/configparser.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/serverep.o: ../srvepconfig/serverep.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $< 		
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/nwkmgrsrv.o: nwkmgrsrv.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $< 
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/nwkmgrdatabase.o: nwkmgrdatabase.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $< 
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/trace.o: ../srvwrapper/trace.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	
$(OBJS)/SimpleDB.o: ../sdb/SimpleDB.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/SimpleDBTxt.o: ../sdb/SimpleDBTxt.c 
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/nwkmgrservices.o: nwkmgrservices.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/nwkmgrp2p.o: nwkmgrp2p.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<	
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/zstack_pb_ctrl.o: ../zstackserverznp/zstack_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<		
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/nwkmgr_pb_ctrl.o: ../nwkmgr/nwkmgr_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<		
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/pb_utils.o: ../source/pb_utils.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<	
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/server_pb_ctrl.o: ../serverpb/server_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<		
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

