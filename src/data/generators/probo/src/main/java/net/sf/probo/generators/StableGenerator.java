package net.sf.probo.generators;

import net.sf.tweety.arg.dung.DungTheory;
import net.sf.tweety.arg.dung.syntax.Argument;
import net.sf.tweety.arg.dung.syntax.Attack;
import net.sf.tweety.arg.dung.writer.ApxWriter;
import net.sf.tweety.arg.dung.writer.TgfWriter;
import net.sf.tweety.commons.util.RandomSubsetIterator;
import net.sf.tweety.commons.util.SubsetIterator;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;


/**
 * This is the generator for abstract argumentation graphs used
 * in ICCMA'15 for the second group of problems (testcases 4-6). It generates
 * graphs that (likely) possess many stable, preferred, and complete extensions.
 * It works roughly as follows:
 * 1.) A set of arguments is identified to form an acyclic subgraph, containing
 * the likely grounded extension
 * 2.) a subset of arguments is randomly selected and attacks are randomly added
 * from some arguments within this set to all arguments outside the set (except
 * to the arguments identified in 1.)
 * 3.) Step 2 is repeated until a number of desired stable extensions is reached *
 * For more details see the code.
 * <p>
 * This generator can be used by adapting the variables in the configuration block in the
 * beginning of the class. Once started, this generator generates graphs continuously
 * (it will not terminate on its own, the application has to be terminated forcefully).
 *
 * @author Matthias Thimm
 */
public class StableGenerator {

    /* Configure the following parameters to generate custom graphs */
    /* The default settings are the settings of ICCMA'15 */

    /* BEGIN configuration */


    /**
     * The maximum number of arguments the graphs will have.
     */
    public static int NumberOfArguments = 1000;

    /**
     * The min/max number of stable extensions
     * (note that it is not guaranteed that the final graph
     * will actually have a number of stable extensions in this interval,
     * these values are only used as heuristics).
     */
    public static int minNumExtensions = 1;
    public static int maxNumExtensions = 30;

    /**
     * The min/max sizes of stable extensions
     * (note that it is not guaranteed, these values are only used
     * as heuristics).
     */
    public static int minSizeOfExtensions = 1;
    public static int maxSizeOfExtensions = 40;

    /**
     * The min/max size of the grounded extension
     * (note that it is not guaranteed, these values are only used
     * as heuristics).
     */
    public static int minSizeOfGroundedExtension = 1;
    public static int maxSizeOfGroundedExtension = 40;

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
        List<Argument> ground;
        Random rand = new Random();
        Argument arg;
        int a, e, s, g;
        while (true) {
            try {
                // BEGIN actual graph generation
                // determine number of arguments in the graph
                a = NumberOfArguments;

                maxNumExtensions = a;
                maxSizeOfExtensions = a;
                maxSizeOfGroundedExtension = a;


                minNumExtensions = Math.max(Math.round(a / 3), minNumExtensions);
                minSizeOfExtensions = Math.max(Math.round(a / 3), minNumExtensions);
                minSizeOfGroundedExtension = Math.max(Math.round(a / 3), minNumExtensions);

                // give status update
                System.out.print("Generating graph #" + path + " with " + a + " arguments...");
                // sample parameters
                e = rand.nextInt(maxNumExtensions - minNumExtensions) + minNumExtensions;
                s = rand.nextInt(maxSizeOfExtensions - minSizeOfExtensions) + minSizeOfExtensions;
                g = rand.nextInt(maxSizeOfGroundedExtension - minSizeOfGroundedExtension) + minSizeOfGroundedExtension;
                // create graph
                theory = new DungTheory();
                all = new ArrayList<Argument>();
                // create arguments
                for (int i = 0; i < a; i++) {
                    arg = new Argument("a" + i);
                    theory.add(arg);
                    all.add(arg);
                }
                // in order to have some non-empty ground extension isolate some arguments which form
                // a acyclic subgraph
                ground = new ArrayList<Argument>();
                for (int j = 0; j < g; j++) {
                    ground.add(all.get(rand.nextInt(a)));
                    for (int k = 0; k < j; k++)
                        if (rand.nextDouble() < 0.2)
                            theory.add(new Attack(ground.get(j), ground.get(k)));
                }
                // sample subsets of arguments
                SubsetIterator<Argument> si = new RandomSubsetIterator<Argument>(new HashSet<Argument>(theory), false);
                List<Argument> ext;
                for (int j = 0; j < e; j++) {
                    do {
                        ext = new ArrayList<Argument>(si.next());
                    } while (ext.size() < s);
                    // add attacks from the sampled set to all other arguments
                    // except those in the isolated graph from before
                    for (Argument arg2 : theory) {
                        if (!ext.contains(arg2) && !ground.contains(arg2))
                            theory.add(new Attack(ext.get(rand.nextInt(ext.size())), arg2));
                    }
                }
                // END actual graph generation
                // give status update
                System.out.print("finished. ");

                apx.write(theory, new File(path + ".apx"));
                tgf.write(theory, new File(path + ".tgf"));
                // give status update
                System.out.print("Saved files.");

                return;

            } catch (Exception ex) {
                System.out.println("error");
                // if we encounter any problem, just continue with the next graph
                continue;
            }
        }
    }
}
