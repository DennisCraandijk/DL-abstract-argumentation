#!/bin/bash
echo 'Compiling Argumentation Framework Generators'
cd ./AFBenchGen2 || exit
mvn compile
cd ./../AFGenBenchmarkGenerator || exit
ant
cd ./../probo || exit
ant
