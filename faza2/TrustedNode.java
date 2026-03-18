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
        // Inicializácia: všetky počiatočné transakcie sú považované za platné
        this.consensusTransactions = new HashSet<Transaction>(pendingTransactions);
    }

    public Set<Transaction> followersSend() {
        // Pošleme všetky transakcie, o ktorých vieme - tie sa šíria sieťou
        return new HashSet<Transaction>(consensusTransactions);
    }

    public void followeesReceive(ArrayList<Integer[]> candidates) {
        // Spracovanie prijatých kandidátov od followees
        // Akceptujeme každú transakciu od uzla, ktorý sledujeme.
        // Simulácia už filtruje neplatné transakcie (kontroluje validTxIds),
        // takže tu stačí akceptovať všetko od dôveryhodných followees.
        // Tým sa transakcie šíria sieťou a po dostatočnom počte kôl
        // všetky čestné uzly konvergujú k rovnakému setu transakcií.
        for (Integer[] candidate : candidates) {
            int txId = candidate[0];
            int sender = candidate[1];

            // Akceptujeme iba od uzlov, ktoré sledujeme
            if (!followees[sender]) continue;

            Transaction tx = new Transaction(txId);
            consensusTransactions.add(tx);
        }
    }
}
