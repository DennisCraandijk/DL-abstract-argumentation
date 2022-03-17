package AFGen;

public class QStar {

    static double findQStar(double p) {
        double qStar = 0;
        double qStarHigh = 1.0;
        double qStarLow = 0;;
        double temp;

        for (int cntr = 0; cntr < 100; ++cntr) {
            qStar = (qStarHigh - qStarLow) / 2.0 + qStarLow;
            temp = (4 * qStar) / ((1+qStar) * (1+qStar));
            if (temp < p)
                qStarLow = qStar;
            else if (temp > p)
                qStarHigh = qStar;
            else
                break;

//            System.out.println("q* = " + qStar +  "(" + qStarLow + "-" + qStarHigh + ") " + temp );
        }

        return qStar;
    }

    // ****************
    // ***** MSIN *****
    // ****************
    public static void main(String[] args) {
        int len = args.length;
        // if no filename provided print error message
        if ( (len < 1) ) {
            System.out.println("Format: QStar p");
            System.out.println("  p = chance any pair of arguments will attack (default .1)");
            System.out.println();
            System.out.println("Result is q* for the provided p. See documentation for details");
            return;
        }

        // default parameters
        String baseName = args[0];
        int n = 100;
        double p =  ParseUtil.parseNormalizedDouble(args[0], .1);


        System.out.println("q* = " +  QStar.findQStar(p));
    }
}

