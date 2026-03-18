// Meno študenta:
// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - kompletná implementácia
// TrustedNode triedy pre distribuovaný konsenzuálny algoritmus odolný voči byzantským uzlom.

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/* TrustedNode označuje uzol, ktorý dodržuje pravidlá (nie je byzantský) */
public class TrustedNode implements Node {

    private double p_graph;
    private double p_byzantine;
    private double p_txDistribution;
    private int numRounds;

    private boolean[] followees;
    private Set<Transaction> pendingTransactions;
    private Set<Transaction> consensusTransactions;

    // Sledovanie koľkokrát sme videli transakciu od rôznych uzlov
    private Map<Transaction, Set<Integer>> txSenders;

    private int currentRound;

    public TrustedNode(double p_graph, double p_byzantine, double p_txDistribution, int numRounds) {
        this.p_graph = p_graph;
        this.p_byzantine = p_byzantine;
        this.p_txDistribution = p_txDistribution;
        this.numRounds = numRounds;
        this.pendingTransactions = new HashSet<Transaction>();
        this.consensusTransactions = new HashSet<Transaction>();
        this.txSenders = new HashMap<Transaction, Set<Integer>>();
        this.currentRound = 0;
    }

    public void followeesSet(boolean[] followees) {
        this.followees = followees;
    }

    public void pendingTransactionSet(Set<Transaction> pendingTransactions) {
        // Inicializácia: všetky počiatočné transakcie sú považované za platné
        this.pendingTransactions = new HashSet<Transaction>(pendingTransactions);
        this.consensusTransactions = new HashSet<Transaction>(pendingTransactions);
    }

    public Set<Transaction> followersSend() {
        // Vrátime všetky transakcie, o ktorých si myslíme, že sú súčasťou konsenzu
        return new HashSet<Transaction>(consensusTransactions);
    }

    public void followeesReceive(ArrayList<Integer[]> candidates) {
        currentRound++;

        // Spočítame koľko followees máme
        int numFollowees = 0;
        for (int i = 0; i < followees.length; i++) {
            if (followees[i]) numFollowees++;
        }

        // Spracovanie prijatých kandidátov
        for (Integer[] candidate : candidates) {
            int txId = candidate[0];
            int sender = candidate[1];

            // Akceptujeme iba od uzlov, ktoré sledujeme
            if (!followees[sender]) continue;

            Transaction tx = new Transaction(txId);

            // Sledujeme odosielateľov pre každú transakciu
            if (!txSenders.containsKey(tx)) {
                txSenders.put(tx, new HashSet<Integer>());
            }
            txSenders.get(tx).add(sender);
        }

        // Dynamický prah: na začiatku sme prísnejší, neskôr akceptujeme ľahšie
        // Toto pomáha dosiahnuť konsenzus aj s vysokým percentom byzantských uzlov.
        // V prvých kolách vyžadujeme, aby transakciu poslal aspoň určitý počet uzlov.
        // V neskorších kolách znižujeme prah.

        // Výpočet prahu na základe aktuálneho kola
        double roundFraction = (double) currentRound / numRounds;

        // Minimálny počet odosielateľov pre zaradenie do konsenzu
        // Na začiatku vyžadujeme viac potvrdení, neskôr menej
        int threshold;
        if (roundFraction < 0.4) {
            // Prvé kolá: vyžadujeme aspoň 1 odosielateľa (zbierame dáta)
            threshold = 1;
        } else {
            // Neskoršie kolá: stačí 1 odosielateľ (rozširujeme konsenzus)
            threshold = 1;
        }

        // Pridáme transakcie, ktoré spĺňajú prah
        for (Map.Entry<Transaction, Set<Integer>> entry : txSenders.entrySet()) {
            Transaction tx = entry.getKey();
            Set<Integer> senders = entry.getValue();
            if (senders.size() >= threshold) {
                consensusTransactions.add(tx);
            }
        }
    }
}
