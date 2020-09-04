package AFGen;


import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

/**
 * Command line version of the Argumentation generator
 *
 * @author Billy D. Spelchan
 */
public class AFGen {
    /**
     * Simple utility that creates a file whose content is the provided string
     * @param filename name of file to save
     * @param contents contents of the file
     */
    public static void quickFile(String filename, String contents) {
        try {
            File saveFile = new File(filename);
            FileWriter fw = new FileWriter(saveFile);
            fw.write(contents);
            fw.flush();
            fw.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    // ****************
    // ***** MSIN *****
    // ****************
    public static void main(String[] args) {
        int len = args.length;
        // if no filename provided print error message
        if ( ( len > 6) || (len < 1) ) {
            System.out.println("Format: AFGen base-name [n] [p] [q] [r] [s]");
            System.out.println("  base-name name of file to generate (without extensions");
            System.out.println("  n = nnumber of arguments to generate (default 100)");
            System.out.println("  p = chance any pair of arguments will attack (default .1)");
            System.out.println("  q = chance attack is mutual (default .34)");
            System.out.println("  r = chance new attack will be added (default .01)");
            System.out.println("  s = chance to remove existing attack (default .01)");
            return;
        }

        // default parameters
        String baseName = args[0];
        int n = 100;
        double p = .1;
        double q = .34;
        double r = .01;
        double s = .1;

        // if parameters exist, replace the default with provided value
        if (len > 1) n = ParseUtil.parseInt(args[1], 2, Integer.MAX_VALUE);
        if (len > 2) p = ParseUtil.parseNormalizedDouble(args[2], p);
        if (len > 3) q = ParseUtil.parseNormalizedDouble(args[3], q);
        if (len > 4) r = ParseUtil.parseNormalizedDouble(args[4], r);
        if (len > 5) s = ParseUtil.parseNormalizedDouble(args[5], s);

        // generate the defeat graph and write the files
        DefeatGraph ag = new DefeatGraph();
        ag.generate(n, p, q);
        quickFile(baseName+".tfg", ag.toString());
        quickFile(baseName+".apx", ag.toAspartixString());

        // if r argument specified .apxm, tgfm
        if (len >= 5) {
            ag.generateModifications(r, s);
            quickFile(baseName+".tgfm", ag.toModifiedTGFFString());
            quickFile(baseName+".apxm", ag.toModifiedAPXFString());
        }
    }

}
