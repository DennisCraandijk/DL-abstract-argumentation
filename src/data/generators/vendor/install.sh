#!/bin/bash
echo 'Compiling Argumentation Framework Generators'
cd ./AFBenchGen2 || exit
mvn compile
mvn package
cd ./../AFGenBenchmarkGenerator || exit
ant
cd ./../probo || exit
cd ./lib || exit
wget -O commons-io-2.6.jar 'https://downloads.sourceforge.net/p/probo/code/HEAD/tree/trunk/lib/commons-io-2.6.jar?format=raw'
wget -O snakeyaml-1.14.jar 'https://downloads.sourceforge.net/p/probo/code/HEAD/tree/trunk/lib/snakeyaml-1.14.jar?format=raw'
wget -O tweety-snapshot-r825.jar 'https://downloads.sourceforge.net/p/probo/code/HEAD/tree/trunk/lib/tweety-snapshot-r825.jar?format=raw'
cd ./../ || exit
ant
