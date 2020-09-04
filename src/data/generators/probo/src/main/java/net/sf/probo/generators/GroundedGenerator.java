package net.sf.probo.generators;

import net.sf.tweety.arg.dung.DungTheory;
import net.sf.tweety.arg.dung.syntax.Argument;
import net.sf.tweety.arg.dung.syntax.Attack;
import net.sf.tweety.arg.dung.writer.ApxWriter;
import net.sf.tweety.arg.dung.writer.TgfWriter;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.Random;
import java.util.Set;


/**
 * This is the generator for abstract argumentation graphs used
 * in ICCMA'15 for the first group of problems (testcases 1-3). It generates
 * graphs that possess a large grounded extension and works roughly as follows:
 * 1.) The number of arguments "a" is randomly chosen (with a given maximum value)
 * 2.) Attacks are added between any argument "a_i" and "a_j" with a given probability,
 * but ONLY if i>j; the results graph is therefore cycle-free (and possibly unconnected)
 * 3) Random attacks are added between the not-yet connected arguments and the main component
 * For more details see the code.
 * <p>
 * This generator can be used by adapting the variables in the configuration block in the
 * beginning of the class. Once started, this generator generates graphs continuously
 * (it will not terminate on its own, the application has to be terminated forcefully).
 *
 * @author Matthias Thimm
 */
public class GroundedGenerator {

    /* Configure the following parameters to generate custom graphs */
    /* The default settings are the settings of ICCMA'15 */

    /* BEGIN configuration */

    /**
     * The maximum number of arguments the graphs will have.
     */
    public static int NumberOfArguments = 1000;

    /**
     * The probability of an attack in the initial dag structure.
     */
    public static double attackProbability = 0.02;

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
//		String filename;
        Random rand = new Random();
        Set<Argument> unconnected;
        int a, k;
//		int idx = 1;
        while (true) {
            try {
                // BEGIN actual graph generation
                // determine number of arguments in the graph
                a = NumberOfArguments;
                // give status update
                System.out.print("Generating graph #" + path + " with " + a + " arguments...");
                // create graph
                theory = new DungTheory();
                Argument arg;
                // we first generate "a" different arguments and add
                // attacks from new created arguments to only previously
                // created arguments; this gives us a possibly unconnected
                // graph without directed cycles (which probably has a huge
                // grounded extension)
                unconnected = new HashSet<Argument>();
                for (int i = 0; i < a; i++) {
                    arg = new Argument("a" + i);
                    unconnected.add(arg);
                    theory.add(arg);
                    for (int j = 0; j < i; j++) {
                        if (rand.nextDouble() <= GroundedGenerator.attackProbability) {
                            theory.add(new Attack(arg, new Argument("a" + j)));
                            unconnected.remove(arg);
                        }
                    }
                }
                // for each argument not yet connected to the main component
                // randomly add an attack from or to this argument from the
                // main component
                for (Argument b : unconnected) {
                    k = rand.nextInt(a);
                    if (rand.nextBoolean())
                        theory.add(new Attack(b, new Argument("a" + k)));
                    else theory.add(new Attack(new Argument("a" + k), b));
                }
                // END actual graph generation
                // give status update
                System.out.print("finished. ");

                apx.write(theory, new File(path + ".apx"));
                tgf.write(theory, new File(path + ".tgf"));
                // give status update
                System.out.print("Saved files.");

                return;

            } catch (Exception e) {
                System.out.println();
                // if we encounter any problem, just continue with the next graph
                continue;
            }
        }
    }
}
