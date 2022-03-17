#!/bin/bash

if [[ $# -eq 0 ]] ; then
    echo 'c -------------------------------------'
    echo 'c | Open-WBO: a Modular MaxSAT Solver |'
    echo 'c -------------------------------------'
    echo 'c USAGE: ./clean.sh [sat solver]'
    echo 'c SAT solvers: minisat2.0, minisat2.2, glucose2.3, glucose3.0, zenn, sinn, glueminisat, gluh, glue_bit, glucored, all'
    exit 0
fi

case "$1" in 
    minisat2.0)
	echo "Cleaning Open-WBO (with minisat2.0)"
	make VERSION=core SOLVERNAME="Minisat2.0" SOLVERDIR=minisat2.0 NSPACE=Minisat clean;;
    minisat2.2)
	echo "Cleaning Open-WBO (with minisat2.2)"
	make VERSION=core SOLVERNAME="Minisat2.2" SOLVERDIR=minisat2.2 NSPACE=Minisat clean;;
    glucose3.0)
	echo "Cleaning Open-WBO (with glucose3.0)"
	make VERSION=core SOLVERNAME="Glucose3.0" SOLVERDIR=glucose3.0 NSPACE=Glucose clean;;
    glucose2.3)
	echo "Cleaning Open-WBO (with glucose2.3)"
	make VERSION=core SOLVERNAME="Glucose2.3" SOLVERDIR=glucose2.3 NSPACE=Glucose clean;;
    zenn)
	echo "Cleaning Open-WBO (with zenn)"
	make VERSION=core SOLVERNAME="ZENN" SOLVERDIR=zenn NSPACE=Minisat clean;;
    sinn)
	echo "Cleaning Open-WBO (with sinn)"
	make VERSION=core SOLVERNAME="SINN" SOLVERDIR=sinn NSPACE=Minisat clean;;
    glueminisat)
	echo "Cleaning Open-WBO (with glueminisat)"
	make VERSION=core SOLVERNAME="Glueminisat" SOLVERDIR=glueminisat NSPACE=Minisat clean;;
    gluh)
	echo "Cleaning Open-WBO (with gluh)"
	make VERSION=core SOLVERNAME="GluH" SOLVERDIR=gluH NSPACE=Minisat clean;;
    glue_bit)
	echo "Cleaning Open-WBO (with glue_bit)"
	make VERSION=core SOLVERNAME="glue_bit" SOLVERDIR=glue_bit NSPACE=Minisat clean;;
    glucored)
	echo "Cleaning Open-WBO (with glucored)"
	make VERSION=simp SOLVERNAME="GlucoRed" SOLVERDIR=glucored NSPACE=Minisat clean;;
    all)
	echo "Cleaning Open-WBO (with all SAT solvers)"
	make VERSION=core SOLVERNAME="Minisat2.0" SOLVERDIR=minisat2.0 NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="Minisat2.2" SOLVERDIR=minisat2.2 NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="Glucose3.0" SOLVERDIR=glucose3.0 NSPACE=Glucose clean
	make VERSION=core SOLVERNAME="Glucose2.3" SOLVERDIR=glucose2.3 NSPACE=Glucose clean
	make VERSION=core SOLVERNAME="ZENN" SOLVERDIR=zenn NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="SINN" SOLVERDIR=sinn NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="Glueminisat" SOLVERDIR=glueminisat NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="GluH" SOLVERDIR=gluH NSPACE=Minisat clean
	make VERSION=core SOLVERNAME="glue_bit" SOLVERDIR=glue_bit NSPACE=Minisat clean
	make VERSION=simp SOLVERNAME="GlucoRed" SOLVERDIR=glucored NSPACE=Minisat clean;;
esac
