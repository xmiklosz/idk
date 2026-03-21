// Meno študenta:
// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - kompletná implementácia
// TrustedNode triedy pre distribuovaný konsenzuálny algoritmus odolný voči byzantským uzlom.

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

/* TrustedNode označuje uzol, ktorý dodržuje pravidlá (nie je byzantský) */
public class TrustedNode implements Node {

    private double p_graph;
    private double p_byzantine;
    private double p_txDistribution;
    private int numRounds;

    private boolean[] followees;
    private Set<Transaction> consensusTransactions;

    public TrustedNode(double p_graph, double p_byzantine, double p_txDistribution, int numRounds) {
        this.p_graph = p_graph;
        this.p_byzantine = p_byzantine;
        this.p_txDistribution = p_txDistribution;
        this.numRounds = numRounds;
    }

    public void followeesSet(boolean[] followees) {
        this.followees = followees;
    }

    public void pendingTransactionSet(Set<Transaction> pendingTransactions) {
        this.consensusTransactions = new HashSet<Transaction>(pendingTransactions);
    }

    public Set<Transaction> followersSend() {
        return new HashSet<Transaction>(consensusTransactions);
    }

    public void followeesReceive(ArrayList<Integer[]> candidates) {
        for (Integer[] candidate : candidates) {
            int txId = candidate[0];
            int sender = candidate[1];

            if (!followees[sender]) continue;

            consensusTransactions.add(new Transaction(txId));
        }
    }
}
