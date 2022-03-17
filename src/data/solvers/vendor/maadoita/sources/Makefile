CURR_DIR   = $(shell pwd)

VERSION    = core
SOLVERNAME = "Glucose4.1"
SOLVERDIR  = glucose4.1
NSPACE     = Glucose

#VERSION    = core
#SOLVERNAME = "Minisat2.2"
#SOLVERDIR  = minisat2.2
#NSPACE     = Minisat

SOURCE     = $(CURR_DIR)
OPENWBO    = $(CURR_DIR)/open-wbo
GLUCOSE    = $(CURR_DIR)/open-wbo/solvers/$(SOLVERDIR)

MAADOITA_CFLAGS = -std=c++11 -O3 -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS
MAADOITA_OBJS = ArguFramework.o Enforcement.o Grounded.o Main.o

MAXSAT_SOLVER ?= openwbo
SAT_SOLVER ?= openwbo

ifeq ($(SAT_SOLVER), openwbo)
	SAT_DIR = $(OPENWBO)/solvers/$(SOLVERDIR)
	MAADOITA_CFLAGS += -DSAT_OPENWBO -DNSPACE=$(NSPACE) -DSOLVERNAME=$(SOLVERNAME) -DVERSION=$(VERSION) -I$(SAT_DIR)
	MAADOITA_OBJS += OpenWBOSATSolver.o
	SAT_OBJS = $(SAT_DIR)/core/Solver.or
else
	$(error No SAT solver specified.)
endif

ifeq ($(MAXSAT_SOLVER), openwbo)
	#cd $(OPENWBO) && ./compile.sh $(SOLVERDIR) && cd $(SOURCE)
	MAXSAT_DIR = $(OPENWBO)
	MAADOITA_CFLAGS += -DMAXSAT_OPENWBO -I$(OPENWBO)
	MAADOITA_OBJS += OpenWBOSolver.o
	MAXSAT_OBJS = $(OPENWBO)/Encoder.or $(OPENWBO)/MaxSAT.or $(OPENWBO)/algorithms/Alg_MSU3.or $(OPENWBO)/encodings/Enc_CNetworks.or $(OPENWBO)/encodings/Enc_GTE.or $(OPENWBO)/encodings/Enc_Ladder.or $(OPENWBO)/encodings/Enc_MTotalizer.or $(OPENWBO)/encodings/Enc_SWC.or $(OPENWBO)/encodings/Enc_Totalizer.or $(OPENWBO)/encodings/Encodings.or $(OPENWBO)/MaxSATFormula.or
else
	$(error No MaxSAT solver specified.)
endif

.cpp.o:
	g++ $(MAADOITA_CFLAGS) -c $<

openwbo:
	cd $(OPENWBO) && make rs && cd $(SOURCE)

maadoita: $(MAADOITA_OBJS)
	g++ $(MAADOITA_CFLAGS) $(MAADOITA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS) -o maadoita

all:    $(SAT_SOLVER) $(MAXSAT_SOLVER) $(MAADOITA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS)
	g++ $(MAADOITA_CFLAGS) $(MAADOITA_OBJS) $(MAXSAT_OBJS) $(SAT_OBJS) -o maadoita

clean:
	rm *.o maadoita
	cd $(OPENWBO) && make clean
