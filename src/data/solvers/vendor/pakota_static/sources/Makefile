CURR_DIR   = $(shell pwd)

VERSION    = core
SOLVERNAME = "Glucose3.0"
SOLVERDIR  = glucose3.0
NSPACE     = Glucose

#VERSION    = core
#SOLVERNAME = "Minisat2.2"
#SOLVERDIR  = minisat2.2
#NSPACE     = Minisat

SOURCE     = $(CURR_DIR)
OPENWBO    = $(CURR_DIR)/open-wbo
LMHS       = $(CURR_DIR)/LMHS
MINISAT    = $(CURR_DIR)/LMHS/minisat
GLUCOSE    = $(CURR_DIR)/open-wbo/solvers/$(SOLVERDIR)

PAKOTA_CFLAGS = -std=c++11 -O3 -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS
PAKOTA_OBJS = ArguFramework.o CredEnforcement.o Enforcement.o Enumeration.o Main.o SkeptEnforcement.o

MAXSAT_SOLVER ?= openwbo
SAT_SOLVER ?= openwbo

ifeq ($(SAT_SOLVER), minisat)
	SAT_DIR = $(MINISAT)
	SAT_LNFLAGS = -lz
	PAKOTA_CFLAGS += -DSAT_MINISAT -I$(MINISAT)
	PAKOTA_OBJS += MiniSATSolver.o
	SAT_OBJS = $(SAT_DIR)/build/release/minisat/core/Solver.o \
				$(SAT_DIR)/build/release/minisat/utils/System.o \
				$(SAT_DIR)/build/release/minisat/utils/Options.o
else ifeq ($(SAT_SOLVER), openwbo)
	SAT_DIR = $(OPENWBO)/solvers/$(SOLVERDIR)
	PAKOTA_CFLAGS += -DSAT_OPENWBO -DNSPACE=$(NSPACE) -DSOLVERNAME=$(SOLVERNAME) -DVERSION=$(VERSION) -I$(SAT_DIR)
	PAKOTA_OBJS += OpenWBOSATSolver.o
	SAT_OBJS = $(SAT_DIR)/core/Solver.or
else
	$(error No SAT solver specified.)
endif

ifeq ($(MAXSAT_SOLVER), openwbo)
	#cd $(OPENWBO) && ./compile.sh $(SOLVERDIR) && cd $(SOURCE)
	MAXSAT_DIR = $(OPENWBO)
	PAKOTA_CFLAGS += -DMAXSAT_OPENWBO -I$(OPENWBO)
	PAKOTA_OBJS += OpenWBOSolver.o
	MAXSAT_OBJS = $(OPENWBO)/Encoder.or $(OPENWBO)/MaxSAT.or $(OPENWBO)/algorithms/Alg_MSU3.or $(OPENWBO)/encodings/Enc_CNetworks.or $(OPENWBO)/encodings/Enc_Ladder.or $(OPENWBO)/encodings/Enc_MTotalizer.or $(OPENWBO)/encodings/Enc_SWC.or $(OPENWBO)/encodings/Enc_Totalizer.or $(OPENWBO)/encodings/Encodings.or
else ifeq ($(MAXSAT_SOLVER), lmhs)
	MAXSAT_DIR = $(LMHS)/src
	PAKOTA_CFLAGS += -DMAXSAT_LMHS -I$(MAXSAT_DIR) -L$(LMHS)/lib -lLMHS -Wl,-rpath,$(LMHS)/lib
	PAKOTA_OBJS += LMHSSolver.o
else
	$(error No MaxSAT solver specified.)
endif

.cpp.o:
	g++ $(PAKOTA_CFLAGS) -c $<

minisat:
	@echo "Compiling minisat:"
	@make -s -C $(SAT_DIR) config prefix=.
	@make -s -C $(SAT_DIR) build/release/minisat/core/Solver.o
	@make -s -C $(SAT_DIR) build/release/minisat/utils/System.o
	@make -s -C $(SAT_DIR) build/release/minisat/utils/Options.o
	@echo "Minisat compiled."

lmhs:
	@echo "Compiling LMHS:"
	cd $(LMHS) && make && make lib && cd $(SOURCE)
	@echo "LMHS compiled."

openwbo:
	cd $(OPENWBO) && ./compile.sh $(SOLVERDIR) && cd $(SOURCE)

pakota: $(PAKOTA_OBJS)
	g++ $(PAKOTA_CFLAGS) $(PAKOTA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS) -o pakota -static -static-libgcc -static-libstdc++

all:    $(SAT_SOLVER) $(MAXSAT_SOLVER) $(PAKOTA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS)
	g++ $(PAKOTA_CFLAGS) $(PAKOTA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS) -o pakota -static -static-libgcc -static-libstdc++

clean:
	rm *.o pakota
	cd $(OPENWBO) && ./clean.sh glucose3.0
