PROJ_DIR = .

OBJS = out

INCLUDE = -I$(PROJ_DIR)
PROTOC_LIB_PATH = -L../../protobuf-c/lib/$(PLATFORM)

COMPILO_FLAGS_x86= "-c -Wall -m32 -std=gnu99"
COMPILO_FLAGS_arm= "-c -Wall -std=gnu99"

VPATH = $(PROJ_DIR)

APP_OBJS = \
	$(OBJS)/usbreset.o

.PHONY: all clean create_output
all: \
    create_output \
    clean \
    arch-all-x86 \
    arch-all-arm \
    arch-all-arago


create_output:
	@echo "VPATH is $(VPATH)"
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

exec_app_x86: RESET_x86

exec_app_arm: RESET_arm

exec_app_arm_arago: RESET_arago

RESET_x86: $(APP_OBJS)
	@echo "Building usbreset.bin for x86 ..."
	@$(COMPILO) -m32 $(LIBS) -rdynamic -o "usbreset.bin" $(APP_OBJS) $(PROTOC_LIB_PATH) -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

RESET_arm: $(APP_OBJS)
	@echo "Building usbreset.bin for arm ..."
	@$(COMPILO) $(LIBS) -rdynamic -o "usbreset.bin" $(APP_OBJS) $(PROTOC_LIB_PATH) -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

RESET_arago: $(APP_OBJS)
	@echo "Building usbreset.bin for arago ..."
	@$(COMPILO) $(LIBS) -rdynamic -o "usbreset.bin" $(APP_OBJS) $(PROTOC_LIB_PATH) -lprotobuf-c 
	@if [ -n "$(EXTRACT_FILES)" ]; then echo $(MAKEFILE_LIST) | $(EXTRACT_FILES) >> project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep -v /usr/ >actual_specific_project_file_list.txt; fi
	@if [ -n "$(EXTRACT_FILES)" ]; then sort -u -V project_file_list.txt | grep /usr/ >actual_standard_project_file_list.txt; fi
	@echo "********************************************************" 

# Start of Object file creation
$(OBJS)/usbreset.o: ../source/usbreset.c
	@pwd
	@echo "Compiling" $< "..."
	@$(COMPILO) -c -o $@ $(COMPILO_FLAGS) $<
	@if [ -n "$(EXTRACT_FILES)" ]; then $(COMPILO) -M $< $(COMPILO_FLAGS) | $(EXTRACT_FILES) >> project_file_list.txt; fi

# rule for cleaning files generated during compilations.
clean:
	@echo "********************************************************" 
	@echo "Cleaning up $(OBJS)/*"	
	-if test -d $(OBJS); then rm -rf $(OBJS); fi
	/bin/rm -f usbreset.bin $(OBJS)/*
	/bin/rm -f *project_file_list.txt

