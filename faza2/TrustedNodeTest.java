// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre TrustedNode (Fáza 2).

import java.util.ArrayList;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Set;
import java.util.Random;

/**
 * Unit testy pre TrustedNode (Fáza 2).
 * Pokrýva 4 testovacie scenáre pre byzantský konsenzuálny algoritmus.
 */
public class TrustedNodeTest {

    private static int passed = 0;
    private static int failed = 0;

    public static void main(String[] args) {
        System.out.println("=== TrustedNode Tests (Fáza 2) ===");

        test1_consensusLowByzantine();
        test2_consensusMediumByzantine();
        test3_consensusHighByzantine();
        test4_consensusSetNonEmpty();

        System.out.println("\n=== Výsledky ===");
        System.out.println("Úspešných: " + passed + " / " + (passed + failed));
        System.out.println("Neúspešných: " + failed);
    }

    private static void assertTest(String name, boolean condition) {
        if (condition) {
            System.out.println("  PASS: " + name);
            passed++;
        } else {
            System.out.println("  FAIL: " + name);
            failed++;
        }
    }

    /**
     * Spustí simuláciu s danými parametrami a vráti true ak všetky
     * trusted uzly dosiahli rovnaký konsenzus (rovnaký set transakcií).
     * Tiež vráti veľkosť konsenzuálneho setu cez pole consensusSize.
     */
    private static boolean runSimulation(double p_graph, double p_byzantine,
            double p_txDistribution, int numRounds, int[] consensusSize) {

        int numNodes = 100;
        int numTx = 500;
        Random random = new Random(42); // fixný seed pre reprodukovateľnosť

        // Vytvor uzly
        Node[] nodes = new Node[numNodes];
        boolean[] isByzantine = new boolean[numNodes];
        for (int i = 0; i < numNodes; i++) {
            if (random.nextDouble() < p_byzantine) {
                nodes[i] = new ByzantineNode(p_graph, p_byzantine, p_txDistribution, numRounds);
                isByzantine[i] = true;
            } else {
                nodes[i] = new TrustedNode(p_graph, p_byzantine, p_txDistribution, numRounds);
                isByzantine[i] = false;
            }
        }

        // Inicializuj náhodný graf
        boolean[][] followees = new boolean[numNodes][numNodes];
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < numNodes; j++) {
                if (i == j) continue;
                if (random.nextDouble() < p_graph) {
                    followees[i][j] = true;
                }
            }
        }

        // Upozorni uzly o followees
        for (int i = 0; i < numNodes; i++)
            nodes[i].followeesSet(followees[i]);

        // Vytvor platné transakcie
        HashSet<Integer> validTxIds = new HashSet<Integer>();
        for (int i = 0; i < numTx; i++) {
            validTxIds.add(random.nextInt());
        }

        // Distribuuj transakcie uzlom
        for (int i = 0; i < numNodes; i++) {
            HashSet<Transaction> pendingTransactions = new HashSet<Transaction>();
            for (Integer txID : validTxIds) {
                if (random.nextDouble() < p_txDistribution)
                    pendingTransactions.add(new Transaction(txID));
            }
            nodes[i].pendingTransactionSet(pendingTransactions);
        }

        // Simuluj koly
        for (int round = 0; round < numRounds; round++) {
            HashMap<Integer, ArrayList<Integer[]>> allProposals = new HashMap<>();

            for (int i = 0; i < numNodes; i++) {
                Set<Transaction> proposals = nodes[i].followersSend();
                for (Transaction tx : proposals) {
                    if (!validTxIds.contains(tx.id)) continue;

                    for (int j = 0; j < numNodes; j++) {
                        if (!followees[j][i]) continue;

                        if (allProposals.containsKey(j)) {
                            Integer[] candidate = new Integer[]{tx.id, i};
                            allProposals.get(j).add(candidate);
                        } else {
                            ArrayList<Integer[]> candidates = new ArrayList<>();
                            candidates.add(new Integer[]{tx.id, i});
                            allProposals.put(j, candidates);
                        }
                    }
                }
            }

            for (int i = 0; i < numNodes; i++) {
                if (allProposals.containsKey(i))
                    nodes[i].followeesReceive(allProposals.get(i));
            }
        }

        // Skontroluj konsenzus medzi trusted uzlami
        Set<Transaction> referenceSet = null;
        boolean allAgree = true;
        int consensusCount = 0;

        for (int i = 0; i < numNodes; i++) {
            if (isByzantine[i]) continue;

            Set<Transaction> nodeResult = nodes[i].followersSend();
            if (referenceSet == null) {
                referenceSet = nodeResult;
                consensusCount = nodeResult.size();
            } else {
                if (!referenceSet.equals(nodeResult)) {
                    allAgree = false;
                }
            }
        }

        if (consensusSize != null && consensusSize.length > 0) {
            consensusSize[0] = consensusCount;
        }

        return allAgree;
    }

    /** Test 1: konsenzus s nízkou mierou byzantských uzlov (15%) */
    static void test1_consensusLowByzantine() {
        System.out.println("Test 1: konsenzus s p_byzantine=0.15, p_graph=0.2, numRounds=10");
        int[] consensusSize = new int[1];
        boolean consensus = runSimulation(0.2, 0.15, 0.05, 10, consensusSize);
        assertTest("Všetky trusted uzly dosiahli konsenzus (15% byzantských)", consensus);
    }

    /** Test 2: konsenzus so strednou mierou byzantských uzlov (30%) */
    static void test2_consensusMediumByzantine() {
        System.out.println("Test 2: konsenzus s p_byzantine=0.30, p_graph=0.2, numRounds=10");
        int[] consensusSize = new int[1];
        boolean consensus = runSimulation(0.2, 0.30, 0.05, 10, consensusSize);
        assertTest("Všetky trusted uzly dosiahli konsenzus (30% byzantských)", consensus);
    }

    /** Test 3: konsenzus s vysokou mierou byzantských uzlov (45%) */
    static void test3_consensusHighByzantine() {
        System.out.println("Test 3: konsenzus s p_byzantine=0.45, p_graph=0.3, numRounds=20");
        int[] consensusSize = new int[1];
        boolean consensus = runSimulation(0.3, 0.45, 0.10, 20, consensusSize);
        assertTest("Všetky trusted uzly dosiahli konsenzus (45% byzantských)", consensus);
    }

    /** Test 4: konsenzuálny set nie je prázdny */
    static void test4_consensusSetNonEmpty() {
        System.out.println("Test 4: konsenzuálny set nie je prázdny");
        int[] consensusSize = new int[1];
        boolean consensus = runSimulation(0.3, 0.15, 0.10, 20, consensusSize);
        assertTest("Konsenzuálny set obsahuje transakcie",
            consensus && consensusSize[0] > 0);
    }
}
