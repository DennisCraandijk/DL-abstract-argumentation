#!/bin/bash

if [[ $# -eq 0 ]] ; then
    echo 'c -------------------------------------'
    echo 'c | Open-WBO: a Modular MaxSAT Solver |'
    echo 'c -------------------------------------'
    echo 'c USAGE: ./compile.sh [sat solver] (options)'
    echo 'c Options: debug, release (default: release)'
    echo 'c SAT solvers: minisat2.0, minisat2.2, glucose2.3, glucose3.0, zenn, sinn, glueminisat, gluh, glue_bit, glucored'
    exit 0
fi

OPTIONS="rs"
MODE="release"

if [[ $# -eq 2 ]] ; then
    case "$2" in 
	debug) OPTIONS=""; MODE="debug"
   	esac
fi

case "$1" in 
    minisat2.0)
	echo "Compiling Open-WBO with minisat2.0 ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="Minisat2.0" SOLVERDIR=minisat2.0 NSPACE=Minisat $OPTIONS;;
    minisat2.2)
	echo "Compiling Open-WBO with minisat2.2 ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="Minisat2.2" SOLVERDIR=minisat2.2 NSPACE=Minisat $OPTIONS;;
    glucose3.0)
	echo "Compiling Open-WBO with glucose3.0 ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="Glucose3.0" SOLVERDIR=glucose3.0 NSPACE=Glucose $OPTIONS;;
    glucose2.3)
	echo "Compiling Open-WBO with glucose2.3 ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="Glucose2.3" SOLVERDIR=glucose2.3 NSPACE=Glucose $OPTIONS;;
    zenn)
	echo "Compiling Open-WBO with zenn ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="ZENN" SOLVERDIR=zenn NSPACE=Minisat $OPTIONS;;
    sinn)
	echo "Compiling Open-WBO with sinn ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="SINN" SOLVERDIR=sinn NSPACE=Minisat $OPTIONS;;
    glueminisat)
	echo "Compiling Open-WBO with glueminisat ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="Glueminisat" SOLVERDIR=glueminisat NSPACE=Minisat $OPTIONS;;
    gluh)
	echo "Compiling Open-WBO with gluh ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="GluH" SOLVERDIR=gluH NSPACE=Minisat $OPTIONS;;
    glue_bit)
	echo "Compiling Open-WBO with glue_bit ($MODE mode)"
	make clean && make VERSION=core SOLVERNAME="glue_bit" SOLVERDIR=glue_bit NSPACE=Minisat $OPTIONS;;
    glucored)
	echo "Compiling Open-WBO with glucored ($MODE mode)"
	make clean && make VERSION=simp SOLVERNAME="GlucoRed" SOLVERDIR=glucored NSPACE=Minisat $OPTIONS;;
esac
