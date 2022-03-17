package AFGen;

import java.util.ArrayList;

/**
 * Attack graph nodes hold the list of nodes they are attacking as well as
 * the nodes that are attacking them. This will make the solver easier to
 * implement.
 *
 * @author Billy D. Spelchan
 */
public class ANode {
    public String label;
    public ArrayList<ANode> attacking;
    public ArrayList<ANode> attackers;
    public ArrayList<ANode> toBeModified;

    /**
     * Constructor
     * @param s label to attach to this node
     */
    public ANode(String s) {
        label = s;
        attacking = new ArrayList<ANode>();
        attackers = new ArrayList<ANode>();
        toBeModified = new ArrayList<ANode>();
    }

    /**
     * Adds a node to the list of nodes to attack.
     * @param target node to attack
     */
    public void addAttack(ANode target) {
        attacking.add(target);
    }

    /**
     * Adds a node to the list of nodes attacking this node.
     * @param attacker node attacking this node
     */
    public void addAttacker(ANode attacker) {
        attackers.add(attacker);
    }

    /**
     * A mutual attack is one in which we attack our attaker back
     * @param target node attacking us who we are going to attack.
     */
    public void mutualAttack(ANode target) {
        attackers.add(target);
        attacking.add(target);
    }

    /**
     * Checks to see if we are attacking the indicated target
     * @param target who to check
     * @return true if attacking, false otherwise
     */
    public boolean isAttacking(ANode target) {
        return attacking.contains(target);
    }

    /**
     * Generator support for tracking which modifications are being made
     * @param target
     */
    public void trackAttackToModify(ANode target) {
        toBeModified.add(target);
    }
}
