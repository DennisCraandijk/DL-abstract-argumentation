package AFGen;

/**
 * String parsing utilities that are shared between both the GUI and the CLI
 *
 * @author Billy D. Spelchan
 */
public class ParseUtil {
    /**
     * Parses a string that should be an int making sure it is within the
     * inidicated range with invalid input defaulting to the minimum value.
     * @param s string to parse
     * @param min minimal value
     * @param max maximum value
     * @return parsed value between range.
     */
    public static int parseInt(String s, int min, int max) {
        int result;
        try {
            result = Integer.parseInt(s);
        } catch (NumberFormatException nfe) {
            result = min;
        }
        if (result < min)
            return min;
        if (result > max)
            return max;
        return result;
    }

    /**
     * Parses a double that should have a range between 0 and 1 with the default
     * value used if the passed string is invalid.
     * @param s string to parse
     * @param def default value for invalid strings
     * @return double between 0 and 1
     */
    public static double parseNormalizedDouble(String s, double def) {
        double result;
        try {
            result = Double.parseDouble(s);
        } catch (NumberFormatException nfe) {
            result = def;
        }
        if (result < 0)
            return 0;
        if (result > 1.0)
            return 1.0;
        return result;
    }

}
