     GlucoRed

     Siert Wieringa 
     siert.wieringa@aalto.fi
 (c) Aalto University 2012/2013


This is the README file for three different variants of GlucoRed
submitted to the SAT solver competition 2013:

1 - GlucoRed
2 - GlucoRed-multi
3 - GlucoRed+March

The file 'build.sh' should build all required binaries and place them 
in the directory 'binary'

*** GlucoRed ***

'GlucoRed' can be build by entering the 'code/glucored' or 'code/simp' 
directory and executing 'make'. All the standard MiniSAT make targets 
can be used, e.g. 'make r' for release, 'make rs' for statically 
compiled release.

The content of the subdirectories 'code/glucored' and 'code/reducer' 
provide the implementation of 'GlucoRed'. All other files are identical 
to those found in the directory 'sources/glucose' of the Glucose 2.1 
distribution, except:

- core/Solver.h  - The keyword 'virtual' has been added to 
                   the definition of the 'search' and 'solve_' 
                   functions.
- core/Solver.cc - Includes a bug fix for a problem that
                   occured when using '-ccmin-mode=1', and
                   several printf's were commented out.
- simp/*         - Modified to create simplying version of 
                   GlucoRed rather than MiniSAT

*** GlucoRed-multi ***

A simple program that runs multiple copies of GlucoRed in parallel
on the same input formula can be found in the directory 
'code/glucored-multi'. It has its own Makefile in that directory

*** GlucoRed+March ***

A simple program that decides whether to run GlucoRed or march_rw on
a given input formula can be found in the directory 
'code/glucored+march'. It has its own Makefile in that directory

*** march_rw **

The source code for march_rw has been download from:
  http://www.st.ewi.tudelft.nl/~marijn/sat/Sources/sat2011/march_rw.zip 

and can be found in the folder 'code/march_rw'. These sources are used
only for 'GlucoRed+March'.

