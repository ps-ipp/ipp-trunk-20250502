default: pantasks pantasks_client pantasks_server

include ../../../Makefile.System
HOME    =       $(ROOT)/src/opihi
SRC     =       $(HOME)/pantasks
BIN     =       $(HOME)/bin
LIB     =       $(HOME)/lib
INC     =       $(HOME)/include
DATA    =       $(DESTDATA)/pantasks
include ../Makefile.Common

# programs may add their own internal requirements here
LIBS1   =       -lbasiccmd -ldatacmd -lastrocmd -lshell -ldata 
LIBS2   =       -ldvo -lkapa -lFITS -lohana
FULL_CFLAGS   = $(BASE_CFLAGS)
FULL_CPPFLAGS = $(BASE_CPPFLAGS)
FULL_LDFLAGS  = $(LIBS1) $(LIBS2) $(BASE_LDFLAGS)

# sched user commands and support functions ########################

client = \
$(SRC)/pantasks_client.$(ARCH).o \
$(SRC)/client_shell.$(ARCH).o \
$(SRC)/invalid.$(ARCH).o \
$(SRC)/server_shutdown.$(ARCH).o \
$(SRC)/server_disconnect.$(ARCH).o \
$(SRC)/server_connect.$(ARCH).o \
$(SRC)/init_client.$(ARCH).o

single = \
$(SRC)/init.$(ARCH).o \
$(SRC)/run.$(ARCH).o \
$(SRC)/stop.$(ARCH).o \
$(SRC)/pantasks.$(ARCH).o \
$(SRC)/thread_locks.$(ARCH).o \
$(SRC)/jobs_and_tasks_thread.$(ARCH).o \
$(SRC)/controller_threads.$(ARCH).o 

server = \
$(SRC)/pantasks_server.$(ARCH).o \
$(SRC)/server_run.$(ARCH).o \
$(SRC)/ListenClients.$(ARCH).o \
$(SRC)/server.$(ARCH).o \
$(SRC)/status_server.$(ARCH).o \
$(SRC)/init_server.$(ARCH).o \
$(SRC)/CheckPassword.$(ARCH).o \
$(SRC)/thread_locks.$(ARCH).o \
$(SRC)/jobs_and_tasks_thread.$(ARCH).o \
$(SRC)/controller_threads.$(ARCH).o

funcs = \
$(SRC)/CheckJobs.$(ARCH).o \
$(SRC)/CheckController.$(ARCH).o \
$(SRC)/CheckTasks.$(ARCH).o \
$(SRC)/CheckTimeRanges.$(ARCH).o \
$(SRC)/ControllerOps.$(ARCH).o \
$(SRC)/LocalJob.$(ARCH).o \
$(SRC)/JobOps.$(ARCH).o \
$(SRC)/JobIDOps.$(ARCH).o \
$(SRC)/TaskOps.$(ARCH).o

cmds = \
$(SRC)/status.$(ARCH).o \
$(SRC)/flush.$(ARCH).o \
$(SRC)/showtask.$(ARCH).o \
$(SRC)/kill.$(ARCH).o \
$(SRC)/delete.$(ARCH).o \
$(SRC)/verbose.$(ARCH).o \
$(SRC)/controller.$(ARCH).o \
$(SRC)/controller_host.$(ARCH).o \
$(SRC)/controller_exit.$(ARCH).o \
$(SRC)/controller_check.$(ARCH).o \
$(SRC)/controller_status.$(ARCH).o \
$(SRC)/controller_jobstack.$(ARCH).o \
$(SRC)/controller_hoststack.$(ARCH).o \
$(SRC)/controller_machines.$(ARCH).o \
$(SRC)/controller_parameters.$(ARCH).o \
$(SRC)/controller_version.$(ARCH).o \
$(SRC)/controller_verbose.$(ARCH).o \
$(SRC)/controller_run.$(ARCH).o \
$(SRC)/controller_output.$(ARCH).o \
$(SRC)/controller_pulse.$(ARCH).o \
$(SRC)/task.$(ARCH).o \
$(SRC)/task_host.$(ARCH).o \
$(SRC)/task_nice.$(ARCH).o \
$(SRC)/task_nmax.$(ARCH).o \
$(SRC)/task_active.$(ARCH).o \
$(SRC)/task_macros.$(ARCH).o \
$(SRC)/task_trange.$(ARCH).o \
$(SRC)/task_stdout.$(ARCH).o \
$(SRC)/task_periods.$(ARCH).o \
$(SRC)/task_command.$(ARCH).o \
$(SRC)/task_options.$(ARCH).o \
$(SRC)/version.$(ARCH).o

libs = \
$(DESTLIB)/libshell.a \
$(DESTLIB)/libdata.a \
$(DESTLIB)/libbasiccmd.a \
$(DESTLIB)/libastrocmd.a \
$(DESTLIB)/libdatacmd.a

pantasks: $(BIN)/pantasks.$(ARCH)
$(SRC)/pantasks.$(ARCH).o : $(libs)
$(BIN)/pantasks.$(ARCH) : $(single) $(funcs) $(cmds)

pantasks_client: $(BIN)/pantasks_client.$(ARCH)
$(SRC)/pantasks_client.$(ARCH).o : $(libs)
$(BIN)/pantasks_client.$(ARCH) : $(client)

pantasks_server: $(BIN)/pantasks_server.$(ARCH)
$(SRC)/pantasks_server.$(ARCH).o : $(libs)
$(BIN)/pantasks_server.$(ARCH) : $(server) $(funcs) $(cmds)

pantasks_client.install: $(DESTBIN)/pantasks_client
pantasks_server.install: $(DESTBIN)/pantasks_server

$(single) $(client) $(server) $(funcs) $(cmds) : $(INC)/pantasks.h

install: $(DESTBIN)/pantasks $(DESTBIN)/pantasks_client $(DESTBIN)/pantasks_server help modules

help: clean-help cmd.basic.help cmd.data.help cmd.astro.help pantasks.help

modules: pantasks.modules

.PHONY: pantasks pantasks_client pantasks_server

.PRECIOUS: $(SRC)/pantasks.c
.PRECIOUS: $(SRC)/pantasks_client.c 
.PRECIOUS: $(SRC)/pantasks_server.c
