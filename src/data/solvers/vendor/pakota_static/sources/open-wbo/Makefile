# VERSION    = core or simp 
# SOLVERNAME = name of the SAT solver
# SOLVERDIR  = subdirectory of the SAT solver
# NSPACE     = namespace of the SAT solver
# 
# e.g. minisat compilation with core version:
#
# VERSION    = core
# SOLVERNAME = "Minisat"
# SOLVERDIR  = minisat
# NSPACE     = Minisat
#
VERSION    = core
SOLVERNAME = "Glucose3.0"
SOLVERDIR  = glucose3.0
NSPACE     = Glucose
# THE REMAINING OF THE MAKEFILE SHOULD BE LEFT UNCHANGED
EXEC       = open-wbo
DEPDIR     = mtl utils core 
DEPDIR     +=  ../../encodings ../../algorithms
MROOT      = $(PWD)/solvers/$(SOLVERDIR)
CFLAGS     = -Wall -Wno-parentheses -DNSPACE=$(NSPACE) -DSOLVERNAME=$(SOLVERNAME) -DVERSION=$(VERSION)
ifeq ($(VERSION),simp)
DEPDIR     += simp
CFLAGS     += -DSIMP=1 
ifeq ($(SOLVERDIR),glucored)
LFLAGS     += -pthread
CFLAGS     += -DGLUCORED
DEPDIR     += reducer glucored
endif
endif
include $(MROOT)/mtl/template.mk
