PROJ_DIR = .

LIBS = -L../../protobuf-c/lib/$(PLATFORM)

# Compilation output
OBJS = $(PROJ_DIR)/out

INCLUDES = -I$(PROJ_DIR) \
    -I../engines \
    -I../framework \
    -I../../nwkmgr \
    -I../../hagateway \
    -I../../otaserver \
    -I../../source \
    -I ../../protobuf-c/include/google/protobuf-c \
    -I ../../srvwrapper \
    -I ../../hal

COMPILO_FLAGS_x86 = "-c -Wall $(INCLUDES) -m32 -std=gnu99 -DSAMPLE_APP_BUILD -DSINGLE_THREADED_PROCESS -DSERVER_NAME=SMPLAPP"
COMPILO_FLAGS_arm = "-c -Wall $(INCLUDES) -std=gnu99 -DSAMPLE_APP_BUILD -DSINGLE_THREADED_PROCESS -DSERVER_NAME=SMPLAPP"

APP_OBJS = \
    $(OBJS)/main.o \
    $(OBJS)/polling.o \
    $(OBJS)/timer_utils.o \
    $(OBJS)/socket_interface.o \
    $(OBJS)/tcp_client.o \
    $(OBJS)/user_interface.o \
    $(OBJS)/data_structures.o \
    $(OBJS)/network_info_engine.o \
    $(OBJS)/device_list_engine.o \
    $(OBJS)/sensor_engine.o \
    $(OBJS)/state_reflector.o \
    $(OBJS)/attribute_engine.o \
    $(OBJS)/actions_engine.o \
    $(OBJS)/commissioning_engine.o \
    $(OBJS)/system_engine.o \
    $(OBJS)/ota_engine.o \
    $(OBJS)/nwkmgr.pb-c.o \
    $(OBJS)/gateway.pb-c.o \
    $(OBJS)/otasrvr.pb-c.o \
    $(OBJS)/group_scene_engine.o \
    $(OBJS)/macros.o \
    $(OBJS)/pb_utils.o \
    $(OBJS)/nwkmgr_pb_ctrl.o \
    $(OBJS)/gateway_pb_ctrl.o \
    $(OBJS)/otasrvr_pb_ctrl.o \
    $(OBJS)/trace.o \
    $(OBJS)/configparser.o

PROJECT_FOLDER = demo/project/

.PHONY: all clean lib create_output arch-all-x86 arch-all-arm clean_obj clean_obj2

all: \
    create_output \
    clean \
    arch-all-x86 \
    arch-all-arm \
    arch-all-arago

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

exec_app_x86: $(OBJS)/MAIN_x86

exec_app_arm: $(OBJS)/MAIN_arm

exec_app_arm_arago: $(OBJS)/MAIN_arago

$(OBJS)/MAIN_x86: $(APP_OBJS)
	@echo "Building main.bin for x86 ..."
	@$(COMPILO) -m32 $(LIBS) -rdynamic -o "main.bin" $(APP_OBJS)  -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/MAIN_arm: $(APP_OBJS)
	@echo "Building main.bin for arm ..."
	@$(COMPILO) $(LIBS) -rdynamic -o "main.bin" $(APP_OBJS)  -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

$(OBJS)/MAIN_arago: $(APP_OBJS)
	@echo "Building main.bin for arago ..."
	@$(COMPILO) $(LIBS) -rdynamic -o  "main.bin" $(APP_OBJS)  -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

# rule for cleaning files generated during compilations.
clean:
	@echo "********************************************************" 
	@echo "Cleaning up $(OBJS)/*"	
	-if test -d $(OBJS); then rm -rf $(OBJS); fi
	rm -f main.bin
	rm -f project_file_list.txt

# Start of Object file creation
$(OBJS)/main.o: main.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/polling.o: ../framework/polling.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/timer_utils.o: ../framework/timer_utils.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/socket_interface.o: ../framework/socket_interface.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/tcp_client.o: ../framework/tcp_client.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	
$(OBJS)/user_interface.o: ../framework/user_interface.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/data_structures.o: ../framework/data_structures.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/macros.o: ../framework/macros.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/network_info_engine.o: ../engines/network_info_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/device_list_engine.o: ../engines/device_list_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/sensor_engine.o: ../engines/sensor_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/state_reflector.o: ../engines/state_reflector.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/attribute_engine.o: ../engines/attribute_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/actions_engine.o: ../engines/actions_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/commissioning_engine.o: ../engines/commissioning_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/system_engine.o: ../engines/system_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/ota_engine.o: ../engines/ota_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/group_scene_engine.o: ../engines/group_scene_engine.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/nwkmgr.pb-c.o: ../../nwkmgr/nwkmgr.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/nwkmgr_pb_ctrl.o: ../../nwkmgr/nwkmgr_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/gateway.pb-c.o: ../../hagateway/gateway.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/gateway_pb_ctrl.o: ../../hagateway/gateway_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/otasrvr.pb-c.o: ../../otaserver/otasrvr.pb-c.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/otasrvr_pb_ctrl.o: ../../otaserver/otasrvr_pb_ctrl.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/pb_utils.o: ../../source/pb_utils.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/trace.o: ../../srvwrapper/trace.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

$(OBJS)/configparser.o: ../../srvwrapper/configparser.c
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi
