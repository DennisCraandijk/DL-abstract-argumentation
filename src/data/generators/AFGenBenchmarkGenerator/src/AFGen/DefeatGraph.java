package AFGen;

import java.util.ArrayList;
import java.util.Random;

/**
 * Defeat Graph with D(p,q,r) generation capabliity as well as support for
 * dynamic track generation.
 *
 * @author Billy D. Spelchan
 */
public class DefeatGraph {
    private Random rand;
    public ArrayList<ANode> vertice;

    /**
     * Constructor
     */
    public DefeatGraph(){
        rand = new Random();
        vertice = new ArrayList<ANode>();
    }

    /** D(n,p,q) generator */
    public void generate(int n, double p, double q) {
        ANode inode, jnode;
        double conflict, mutual;
        vertice.clear();
        // create the nodes
        for (int i = 0; i < (n); ++i) {
            inode = new ANode(""+(i));
            vertice.add(inode);
        }

        // generate attack edges
        for (int i = 0; i < (n-1); ++i) {
            inode = vertice.get(i);
            for (int j = i+1; j < n; ++j) {
                jnode = vertice.get(j);
                conflict = rand.nextDouble();
                if (conflict < p) {
                    mutual = rand.nextDouble();
                    if (mutual < q) {
                        inode.mutualAttack(jnode);
                        jnode.mutualAttack(inode);
                    } else {
                        if (rand.nextDouble() < .5) {
                            inode.addAttack(jnode);
                            jnode.addAttacker(inode);
                        } else {
                            inode.addAttacker(jnode);
                            jnode.addAttack(inode);
                        }
                    }
                }
            }
        }
    }

    /**
     * Creates an Aspartix string that can be written as a file
     * @return Aspartix file as a string
     */
    public String toAspartixString() {
        StringBuffer sb = new StringBuffer();
        for (ANode node : vertice) {
            sb.append("arg(a");
            sb.append(node.label);
            sb.append(").\n");
        }
        for (ANode node : vertice) {
            if (node.attacking.size() > 0)
                for(ANode target : node.attacking) {
                    sb.append("att(a");
                    sb.append(node.label);
                    sb.append(",a");
                    sb.append(target.label);
                    sb.append(").\n");
                }
        }
        return sb.toString();

    }

    /**
     * TGF files are returned as the default toString.
     * @return state of defeat graph in TGF file string
     */
    @Override
    public String toString() {
        StringBuffer sb = new StringBuffer();
        for (ANode node : vertice) {
            sb.append(node.label);
            sb.append("\n");
        }
        sb.append("#\n");
        for (ANode node : vertice) {
            if (node.attacking.size() > 0)
                for(ANode target : node.attacking) {
                    sb.append(node.label);
                    sb.append(' ');
                    sb.append(target.label);
                    sb.append("\n");
                }
        }
        return sb.toString();
    }

    /**
     * Create a dynamically modified file
     * @param r chance of new attack
     * @param s chance of removing attack
     */
    public void generateModifications(double r, double s) {
        ANode inode, jnode;
        int n = vertice.size();
        for (int i = 0; i < n; ++i) {
            inode = vertice.get(i);
            for (int j = 0; j < i; ++j) {
                jnode = vertice.get(j);
                if (inode.isAttacking(jnode)) {
                    if (rand.nextDouble() < s)
                        inode.trackAttackToModify(jnode);
                } else
                    if (rand.nextDouble() < r)
                        inode.trackAttackToModify(jnode);
            }
            for (int j = i; j < n; ++j) {
                jnode = vertice.get(j);
                if (inode.isAttacking(jnode)) {
                    if (rand.nextDouble() < s)
                        inode.trackAttackToModify(jnode);
                } else
                if (rand.nextDouble() < r)
                    inode.trackAttackToModify(jnode);
            }
        }
    }

    /**
     * Writes the TGFM file as a string
     * @return TGFM file as string
     */
    public String toModifiedTGFFString() {
        StringBuffer sb = new StringBuffer();
        for (ANode node : vertice) {
            ArrayList<ANode> mods = node.toBeModified;
            for (ANode target : mods) {
                if (node.isAttacking(target)) {
                    sb.append('-');
                } else {
                    sb.append('+');
                }
                sb.append(node.label);
                sb.append(' ');
                sb.append(target.label);
                sb.append("\n");
            }
        }
        return sb.toString();
    }

    /**
     * Write the APXM file as a string
     * @return APXM file as a string
     */
    public String toModifiedAPXFString() {
        StringBuffer sb = new StringBuffer();
        for (ANode node : vertice) {
            ArrayList<ANode> mods = node.toBeModified;
            for (ANode target : mods) {
                if (node.isAttacking(target)) {
                    sb.append("-att(a");
                } else {
                    sb.append("+att(a");
                }
                sb.append(node.label);
                sb.append(",a");
                sb.append(target.label);
                sb.append(").\n");
            }
        }
        return sb.toString();
    }

}
