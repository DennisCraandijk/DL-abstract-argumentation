======================================================================

                         GlueMiniSat 2.2.7

      Hidetomo NABESHIMA *1,  Koji IWANUMA *1,  Katsumi INOUE *2

                *1 University of Yamanashi, JAPAN
               {nabesima,iwanuma}@yamanashi.ac.jp

           *2 National Institute of Informatics, JAPAN
                          inoue@nii.ac.jp

======================================================================

(1) BUILDING

   ./build.sh

(2) RUNNING

 For SAT+UNSAT or SAT track

   ./binary/glueminisat-simp -compe BENCHNAME

 For certified UNSAT track

   ./binary/glueminisat-cert-unsat.sh BENCHNAME
