package net.sf.probo.generators;

import net.sf.tweety.arg.dung.DungTheory;
import net.sf.tweety.arg.dung.syntax.Argument;
import net.sf.tweety.arg.dung.syntax.Attack;
import net.sf.tweety.arg.dung.writer.ApxWriter;
import net.sf.tweety.arg.dung.writer.TgfWriter;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.Random;


/**
 * This is the generator for abstract argumentation graphs used
 * in ICCMA'15 for the third group of problems (testcases 7-9). It generates
 * graphs that (likely) possess many strongly connected components and
 * are thus easily decomposable. It works roughly as follows:
 * 1.) Arguments are generated and partitioned into a given number of components n,
 * 2.) Within each components attacks are added with high probability (thus likely
 * forming a strongly connected component
 * 3.) In-between components attacks are added with less probability, but only between
 * components i to j with i>j (in order to avoid having one large SCC)
 * <p>
 * This generator can be used by adapting the variables in the configuration block in the
 * beginning of the class. Once started, this generator generates graphs continuously
 * (it will not terminate on its own, the application has to be terminated forcefully).
 *
 * @author Matthias Thimm
 */
public class SccGenerator {

    /* Configure the following parameters to generate custom graphs */
    /* The default settings are the settings of ICCMA'15 */

    /* BEGIN configuration */

    /**
     * The maximum number of arguments the graphs will have.
     */
    public static int NumberOfArguments = 500;

    /**
     * The max number of strongly connected components
     * (note that it is not guaranteed that the final graph
     * will actually have this number of SCCs,
     * this value is only used as a heuristic).
     */
    public static int maxNumSccs = 50;

    /**
     * The probability that there is an attack between two arguments within
     * an SCC.
     */
    public static double innerAttackProbability = 0.5;

    /**
     * The probability that there is an attack between two arguments from one
     * SCC to another one.
     */
    public static double outerAttackProbability = 0.1;

    /* END configuration */


    /**
     * Main method
     *
     * @param args not needed
     * @throws IOException if something went wrong with saving files etc.
     */
    public static void main(String[] args) throws IOException {
        String path = args[0];
        NumberOfArguments = Integer.valueOf(args[1]);

        // initialize file writers for the formats APX and TGF
        ApxWriter apx = new ApxWriter();
        TgfWriter tgf = new TgfWriter();
        // Some variables needed for generation
        DungTheory theory;
        List<Argument> all;
        Random rand = new Random();
        Argument arg;
        int a, scc;
        while (true) {
            try {
                // BEGIN actual graph generation
                // sample number of arguments in the graph
                // and number of SCCs
                a = NumberOfArguments;
                scc = rand.nextInt(maxNumSccs) + 1;
                // give status update
                System.out.print("Generating graph #" + path + " with " + a + " arguments...");
                // create graph
                theory = new DungTheory();
                all = new ArrayList<Argument>();
                // generate arguments
                for (int i = 0; i < a; i++) {
                    arg = new Argument("a" + i);
                    theory.add(arg);
                    all.add(arg);
                }
                // associate arguments to SCCs
                List<List<Argument>> sccs = new ArrayList<List<Argument>>();
                for (int i = 0; i < scc; i++)
                    sccs.add(new LinkedList<Argument>());
                for (Argument ar : all)
                    sccs.get(rand.nextInt(scc)).add(ar);
                //within each SCC add lots of attacks
                for (int i = 0; i < scc; i++) {
                    for (Argument a1 : sccs.get(i))
                        for (Argument a2 : sccs.get(i))
                            if (rand.nextDouble() < innerAttackProbability)
                                theory.add(new Attack(a1, a2));
                }
                // in between SCCs add some attacks obeying the directionality
                for (int i = 0; i < scc - 1; i++)
                    for (int j = i + 1; j < scc; j++) {
                        if (rand.nextDouble() < 0.3) {
                            for (Argument a1 : sccs.get(i))
                                for (Argument a2 : sccs.get(j))
                                    if (rand.nextDouble() < outerAttackProbability)
                                        theory.add(new Attack(a1, a2));
                        }
                    }
                // END actual graph generation
                // give status update
                System.out.print("finished. ");
                // if no solutions are required, write graph as APX and TGF file
                apx.write(theory, new File(path + ".apx"));
                tgf.write(theory, new File(path + ".tgf"));
                // give status update
                System.out.print("Saved files.");
                return;

            } catch (Exception ex) {
                System.out.println();
                // if we encounter any problem, just continue with the next graph
                continue;
            }
        }
    }
}
