# Makefile for Zstack Server

PROTOC_LIB_PATH = -L../protobuf-c/lib/$(PLATFORM)

COMPONENTS = ../../../../Components
PROJECTS = ../../../../Projects

# Compilation outputl
OBJS = out

# Library to use
LIBS= -lpthread -lprotobuf-c

INCLUDE = -I . \
	-I $(PROJECTS)/zstack/linux/zstackserverznp \
	-I $(PROJECTS)/zstack/linux/zstackpb \
	-I $(PROJECTS)/zstack/linux/srvwrapper \
	-I $(PROJECTS)/zstack/linux/linux_osal/include \
	-I $(PROJECTS)/zstack/linux/hal \
	-I $(PROJECTS)/zstack/linux/source \
	-I $(PROJECTS)/zstack/linux/zmac \
	-I $(COMPONENTS)/hal/include \
	-I $(COMPONENTS)/stack/gp \
	-I $(COMPONENTS)/stack/nwk \
	-I $(COMPONENTS)/stack/zcl \
	-I $(COMPONENTS)/stack/sec \
	-I $(COMPONENTS)/stack/af \
	-I $(COMPONENTS)/stack/sys \
	-I $(COMPONENTS)/stack/zdo \
	-I $(COMPONENTS)/zmac \
	-I $(COMPONENTS)/mt \
	-I $(COMPONENTS)/mac/include \
	-I $(PROJECTS)/zstack/linux/protobuf-c/include/google/protobuf-c \
	-I $(PROJECTS)/zstack/linux/demo/framework \
	-I $(PROJECTS)/zstack/linux/source

DEFINES = -D_GNU_SOURCE -DLINUX_ZNP -DZDO_COORDINATOR -D__ZNP_DEBUG__ -DLCD_SUPPORTED  \
	-DAPI_CLIENT_8BIT_LEN -DSERVER_NAME=Z_STACK \
	-D__APP_UI__

COMPILO_FLAGS_x86 = "-O0 -Wall $(INCLUDE) $(DEFINES) -m32 -include ../hal/hal_types.h -include f8wConfig.h"
COMPILO_FLAGS_arm = "-O0 -Wall -Wno-unused-but-set-variable $(INCLUDE) $(DEFINES) -include ../hal/hal_types.h -include f8wConfig.h -mcpu=cortex-a8 -march=armv7-a"

APP_OBJS = \
    $(OBJS)/osal_zstack_server_znp.o \
	$(OBJS)/zmain.o \
	$(OBJS)/znp_af.o \
	$(OBJS)/znp_misc.o \
	$(OBJS)/znp_zdo.o \
	$(OBJS)/zstack.pb-c.o \
	$(OBJS)/zstackpb.o \
	$(OBJS)/api_client.o \
	$(OBJS)/trace.o \
	$(OBJS)/main.o \
	$(OBJS)/api_server.o \
	$(OBJS)/configparser.o \
	$(OBJS)/OSAL_Memory.o \
	$(OBJS)/OSAL_PwrMgr.o \
	$(OBJS)/OSAL_Timers.o \
	$(OBJS)/OSAL.o \
	$(OBJS)/saddr.o \
	$(OBJS)/OnBoard.o \
	$(OBJS)/hal_timer.o \
	$(OBJS)/pb_utils.o \
	$(OBJS)/zstack_pb_ctrl.o
	
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

exec_app_x86: $(OBJS)/ZLSZNP_x86

exec_app_arm: $(OBJS)/ZLSZNP_arm

exec_app_arago: $(OBJS)/ZLSZNP_arago

clean:
	@echo "Cleaning up $(OBJS)/*"	
	@/bin/rm -rf $(OBJS)
	@rm -f project_file_list.txt
	@rm -f actual_standard_project_file_list.txt
	@rm -f actual_specific_project_file_list.txt

$(OBJS)/ZLSZNP_arm: $(APP_OBJS)
	@echo "Building target ZLSZNP_arm ..."
	@$(COMPILO) $(PROTOC_LIB_PATH) -rdynamic -o $@ $(APP_OBJS) $(LIBS)
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/ZLSZNP_arago: $(APP_OBJS)
	@echo "Building target ZLSZNP_arago ..."
	@$(COMPILO) $(PROTOC_LIB_PATH) -rdynamic -o $@ $(APP_OBJS) $(LIBS)
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/ZLSZNP_x86: $(APP_OBJS)
	@echo "Building target ZLSZNP_x86 ..."
	@$(COMPILO) $(PROTOC_LIB_PATH) -rdynamic -m32 -o $@ $(APP_OBJS) $(LIBS)
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 
	
# Start of Object file creation
$(OBJS)/osal_zstack_server_znp.o: osal_zstack_server_znp.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/zmain.o: ../source/zmain.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/znp_af.o: znp_af.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/znp_misc.o: znp_misc.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/znp_zdo.o: znp_zdo.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/zstack.pb-c.o: ../zstackpb/zstack.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/zstackpb.o: ../zstackpb/zstackpb.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/api_client.o: ../srvwrapper/api_client.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/trace.o: ../srvwrapper/trace.c
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

$(OBJS)/OSAL_Memory.o: ../linux_osal/common/OSAL_Memory.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/OSAL_PwrMgr.o: ../linux_osal/common/OSAL_PwrMgr.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/OSAL_Timers.o: ../linux_osal/common/OSAL_Timers.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/OSAL.o: ../linux_osal/common/OSAL.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/saddr.o: $(COMPONENTS)/services/saddr/saddr.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/OnBoard.o: ../source/OnBoard.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/hal_timer.o: ../hal/hal_timer.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/pb_utils.o: ../source/pb_utils.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/zstack_pb_ctrl.o: ../zstackserverznp/zstack_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
