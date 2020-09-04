#!/bin/bash
# (c)2014 Federico Cerutti <federico.cerutti@acm.org> --- MIT LICENCE
# Generic script interface to probo http://sourceforge.net/projects/probo/
# Please feel freet to customize it for your own solver


# function for echoing on standard error
echoerr()
{
	# to remove standard error echoing, please comment the following line
	echo "$@" 1>&2; 
}

################################################################
# C O N F I G U R A T I O N
# 
# this script must be customized by defining:
# 1) procedure for printing author and version information of the solver
#	(function "information")
# 2) suitable procedures for invoking your solver (function "solver");
# 3) suitable procedures for parsing your solver's output 
#	(function "parse_output");
# 4) list of supported format (array "formats");
# 5) list of supported problems (array "problems").

# output information
function information
{
	# example for ArgSemSATv0.2
	echo "ArgSemSATv0.2"
	echo "Federico Cerutti <federico.cerutti@acm.org>"
	echo "Mauro Vallati <mauro.vallati@hud.ac.uk>"
}

# how to invoke your solver: this function must be customized
function solver
{
	fileinput=$1	# input file with correct path

	format=$2	# format of the input file (see below)

	problem=$3    	# problem to solve (see below)

	additional=$4	# additional information, i.e. name of an argument


	# dummy output
	echoerr "input file: $fileinput"
	echoerr "format: $format"
	echoerr "problem: $problem"
	echoerr "additional: $additional"

	# example for ArgSemSATv0.2
	if [ "$format" = "apx" -a "$problem" = "EE-PR" ];
	then
		./$(dirname $0)/ArgSemSAT $fileinput -ExtSAT GLUCOSE -sem preferred-df
	else
		echoerr "unsupported format or problem"
		exit 1
	fi
}


# how to parse the output of your solver in order to be compliant with probo:
# this function must be customized
# probo accepts solutions of the form:
#	[arg1,arg2,...,argN]       for 
#                                  1. some extension enumeration (SE)
#                                  2. enum. arguments credulously inferred (EC)
#                                  3. enum. arguments skeptically inferred (ES) 
#	[[arg1,arg2,...,argN],[...],...] for extension(s) enumeration
#	YES/NO                           for decision problems
function parse_output
{
	problem=$1
	output="$2"

	echoerr "original output: $output"

	#example of parsing for ArgSemSATv0.2, which returns "{arg1,arg2,...}\n{...}\n..."
	if [ "$problem" = "EE-PR" ];
	then
		echo -n "["
		echo $output | sed 's/{/[/g' | sed 's/}/]/g' | tr -d '\n' \
			| sed 's/}{/},{/g' | sed 's/\ /,/g'
		echo "]"
	else
		echoerr "unsupported format or problem"
		exit 1
	fi
}

# accepted formats: please comment those unsupported
formats[1]="apx" # "aspartix" format
formats[2]="cnf" # conjunctive normal form
formats[3]="tgf" # trivial graph format

# problems that can be accepted: please comment those unsupported

#+---------------------------------------------------------------------+
#|         I C C M A   '1 5   L I S T   O F   P R O B L E M S          |
#|                                                                     |
problems[1]="DC-CO" 	# Decide credulously according to Complete semantics
problems[2]="DC-GR" 	# Decide credulously according to Grounded semantics
problems[3]="DC-PR" 	# Decide credulously according to Preferred semantics
problems[4]="DC-ST" 	# Decide credulously according to Stable semantics
problems[5]="DS-CO" 	# Decide skeptically according to Complete semantics
problems[6]="DS-GR" 	# Decide skeptically according to Grounded semantics
problems[7]="DS-PR" 	# Decide skeptically according to Preferred semantics
problems[8]="DS-ST" 	# Decide skeptically according to Stable semantics
problems[9]="EE-CO" 	# Enumerate all the extensions according to Complete semantics
problems[10]="EE-GR" 	# Enumerate all the extensions according to Grounded semantics
problems[11]="EE-PR" 	# Enumerate all the extensions according to Preferred semantics
problems[12]="EE-ST" 	# Enumerate all the extensions according to Stable semantics
problems[13]="SE-CO" 	# Enumerate some extension according to Complete semantics
problems[14]="SE-GR" 	# Enumerate some extension according to Grounded semantics
problems[15]="SE-PR" 	# Enumerate some extension according to Preferred semantics
problems[16]="SE-ST" 	# Enumerate some extension according to Stable semantics
#|                                                                     |
#|  E N D   O F   I C C M A   '1 5   L I S T   O F   P R O B L E M S   |
#+---------------------------------------------------------------------+

problems[17]="DC-ADM" 	# Decide credulously according to admissiblity
problems[18]="DC-CF2" 	# Decide credulously according to CF2 semantics
problems[19]="DC-CF" 	# Decide credulously according to conflict-freeness
problems[20]="DC-ID" 	# Decide credulously according to Ideal semantics
problems[21]="DC-SST" 	# Decide credulously according to Semi-stable semantics
problems[22]="DC-STG" 	# Decide credulously according to Stage semantics
problems[23]="DS-ADM" 	# Decide skeptically according to admissiblity
problems[24]="DS-CF2" 	# Decide skeptically according to CF2 semantics
problems[25]="DS-CF" 	# Decide skeptically according to conflict-freeness
problems[26]="DS-ID" 	# Decide skeptically according to Ideal semantics
problems[27]="DS-SST" 	# Decide skeptically according to Semi-stable semantics
problems[28]="DS-STG" 	# Decide skeptically according to Stage semantics
problems[29]="EC-ADM" 	# Enumerate all the arguments credulously inferred according to admissiblity
problems[30]="EC-CF2" 	# Enumerate all the arguments credulously inferred according to CF2 semantics
problems[31]="EC-CF" 	# Enumerate all the arguments credulously inferred according to conflict-freeness
problems[32]="EC-CO" 	# Enumerate all the arguments credulously inferred according to Complete semantics
problems[33]="EC-GR" 	# Enumerate all the arguments credulously inferred according to Grounded semantics
problems[34]="EC-ID" 	# Enumerate all the arguments credulously inferred according to Ideal semantics
problems[35]="EC-PR" 	# Enumerate all the arguments credulously inferred according to Preferred semantics
problems[36]="EC-SST" 	# Enumerate all the arguments credulously inferred according to Semi-stable semantics
problems[37]="EC-STG" 	# Enumerate all the arguments credulously inferred according to Stage semantics
problems[38]="EC-ST" 	# Enumerate all the arguments credulously inferred according to Stable semantics
problems[39]="EE-ADM" 	# Enumerate all the extensions according to admissiblity
problems[40]="EE-CF2" 	# Enumerate all the extensions according to CF2 semantics
problems[41]="EE-CF" 	# Enumerate all the extensions according to conflict-freeness
problems[42]="EE-ID" 	# Enumerate all the extensions according to Ideal semantics
problems[43]="EE-SST" 	# Enumerate all the extensions according to Semi-stable semantics
problems[44]="EE-STG" 	# Enumerate all the extensions according to Stage semantics
problems[45]="ES-ADM" 	# Enumerate all the arguments skeptically inferred according to admissiblity
problems[46]="ES-CF2" 	# Enumerate all the arguments skeptically inferred according to CF2 semantics
problems[47]="ES-CF" 	# Enumerate all the arguments skeptically inferred according to conflict-freeness
problems[48]="ES-CO" 	# Enumerate all the arguments skeptically inferred according to Complete semantics
problems[49]="ES-GR" 	# Enumerate all the arguments skeptically inferred according to Grounded semantics
problems[50]="ES-ID" 	# Enumerate all the arguments skeptically inferred according to Ideal semantics
problems[51]="ES-PR" 	# Enumerate all the arguments skeptically inferred according to Preferred semantics
problems[52]="ES-SST" 	# Enumerate all the arguments skeptically inferred according to Semi-stable semantics
problems[53]="ES-STG" 	# Enumerate all the arguments skeptically inferred according to Stage semantics
problems[54]="ES-ST" 	# Enumerate all the arguments skeptically inferred according to Stable semantics
problems[55]="SE-ADM" 	# Enumerate some extension according to admissiblity
problems[56]="SE-CF2" 	# Enumerate some extension according to CF2 semantics
problems[57]="SE-CF" 	# Enumerate some extension according to conflict-freeness
problems[58]="SE-ID" 	# Enumerate some extension according to Ideal semantics
problems[59]="SE-SST" 	# Enumerate some extension according to Semi-stable semantics
problems[60]="SE-STG" 	# Enumerate some extension according to Stage semantics

# E N D   O F   C O N F I G U R A T I O N    S E C T I O N
#################################################################

function list_output
{
	declare -a arr=("${!1}")
	check_something_printed=false
	echo -n '['
	for i in ${arr[@]}; 
	do
		if [ "$check_something_printed" = true ];
		then
			echo -n ", "
		fi
		echo -n $i
		check_something_printed=true
	done
        echo ']'
}

function main
{
	if [ "$#" = "0" ]
	then
		information
		exit 0
	fi

	local local_problem=""
	local local_fileinput=""
	local local_format=""
	local local_additional=""

	while [ "$1" != "" ]; do
	    case $1 in
	      "--formats")
		list_output formats[@]
		exit 0
		;;
	      "--problems")
		list_output problems[@]
		exit 0
		;;
	      "-p")
		shift
		local_problem=$1
		;;
	      "-f")
		shift
		local_fileinput=$1
		;;
	      "-fo")
		shift
		local_format=$1
		;;
	      "-a")
		shift
		local_additional=$1
		;;
	    esac
	    shift
	done

	res=$(solver $local_fileinput $local_format $local_problem $local_additional)

	parse_output $local_problem "$res"
}

main $@
exit 0
