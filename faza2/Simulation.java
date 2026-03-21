// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre TrustedNode (Fáza 2).
// Príklad simulácie. Tento test spúšťa uzly na náhodnom grafe.
// Na konci vypíše ID transakcií, na ktorých bol podľa uzlov
// dosiahnutý konsenzus. Túto simuláciu môžete použiť na
// otestovanie svojich uzlov.

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Random;
import java.util.Set;
import java.util.HashMap;

/**
 * Simulácia + Unit testy pre TrustedNode (Fáza 2).
 *
 * Test 1: niekoľko testov s rôznymi parametrami: numNodes, p_graph, p_malicious,
 *         p_txDistribution, numRounds
 * Test 2: testuje konsenzus medzi uzlami s rôznymi parametrami a čas koľko trvá
 *         dosiahnuť konsenzus
 *
 * Spustenie: java Simulation
 */
public class Simulation {

   private static int passed = 0;
   private static int failed = 0;

   // ANSI farby pre prehľadný výstup
   private static final String GREEN = "\u001B[32m";
   private static final String RED = "\u001B[31m";
   private static final String YELLOW = "\u001B[33m";
   private static final String CYAN = "\u001B[36m";
   private static final String BOLD = "\u001B[1m";
   private static final String RESET = "\u001B[0m";

   /**
    * Výsledok simulácie - obsahuje všetky dôležité info.
    */
   private static class SimResult {
      boolean consensus;
      int consensusSize;
      long elapsedMs;
      int trustedCount;
      int byzantineCount;
   }

   public static void main(String[] args) {
      System.out.println(BOLD + "╔════════════════════════════════════════════════════════════╗");
      System.out.println("║          TrustedNode Unit Testy (Fáza 2)                   ║");
      System.out.println("╚════════════════════════════════════════════════════════════╝" + RESET);

      System.out.println("\n" + BOLD + "--- Test 1: rôzne parametre (numNodes, p_graph, p_malicious, p_txDistribution, numRounds) ---" + RESET);
      test1a_variousParams_lowByzantine();
      test1b_variousParams_highByzantine();

      System.out.println("\n" + BOLD + "--- Test 2: konsenzus medzi uzlami + čas dosiahnutia konsenzu ---" + RESET);
      test2a_consensusTime_fewRounds();
      test2b_consensusTime_moreRounds();

      System.out.println();
      System.out.println(BOLD + "╔════════════════════════════════════════════════════════════╗");
      if (failed == 0) {
         System.out.println("║  " + GREEN + "VŠETKY TESTY ÚSPEŠNÉ: " + passed + "/" + (passed + failed) + RESET + BOLD + "                              ║");
      } else {
         System.out.println("║  " + RED + "NEÚSPEŠNÉ: " + failed + " z " + (passed + failed) + RESET + BOLD + "                                      ║");
      }
      System.out.println("╚════════════════════════════════════════════════════════════╝" + RESET);
   }

   private static void assertTest(String name, boolean condition) {
      if (condition) {
         System.out.println("  " + GREEN + "✔ PASS" + RESET + ": " + name);
         passed++;
      } else {
         System.out.println("  " + RED + "✘ FAIL" + RESET + ": " + name);
         failed++;
      }
   }

   /**
    * Spustí simuláciu s danými parametrami a vráti detailný výsledok.
    */
   private static SimResult runSimulation(int numNodes, double p_graph, double p_byzantine,
         double p_txDistribution, int numRounds) {

      int numTx = 500;
      Random random = new Random(42);

      long startTime = System.currentTimeMillis();

      // Vytvor uzly - byzantské a trusted
      Node[] nodes = new Node[numNodes];
      boolean[] isByzantine = new boolean[numNodes];
      int byzantineCount = 0;
      for (int i = 0; i < numNodes; i++) {
         if (random.nextDouble() < p_byzantine) {
            nodes[i] = new ByzantineNode(p_graph, p_byzantine, p_txDistribution, numRounds);
            isByzantine[i] = true;
            byzantineCount++;
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
                     allProposals.get(j).add(new Integer[]{tx.id, i});
                  } else {
                     ArrayList<Integer[]> candidates = new ArrayList<>();
                     candidates.add(new Integer[]{tx.id, i});
                     allProposals.put(j, candidates);
                  }
               }
            }
         }

         // Distribuuje návrhy k ich zamýšľaným príjemcom
         for (int i = 0; i < numNodes; i++) {
            if (allProposals.containsKey(i))
               nodes[i].followeesReceive(allProposals.get(i));
         }
      }

      long elapsed = System.currentTimeMillis() - startTime;

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

      SimResult result = new SimResult();
      result.consensus = allAgree;
      result.consensusSize = consensusCount;
      result.elapsedMs = elapsed;
      result.trustedCount = numNodes - byzantineCount;
      result.byzantineCount = byzantineCount;
      return result;
   }

   /**
    * Vypíše prehľadné info o parametroch a výsledku simulácie.
    */
   private static void printSimInfo(String label, int numNodes, double p_graph,
         double p_byzantine, double p_txDistribution, int numRounds, SimResult r) {
      System.out.println(CYAN + "  [" + label + "]" + RESET);
      System.out.println("    Parametre: numNodes=" + numNodes
         + ", p_graph=" + p_graph
         + ", p_malicious=" + p_byzantine
         + ", p_txDistribution=" + p_txDistribution
         + ", numRounds=" + numRounds);
      System.out.println("    Uzly: " + r.trustedCount + " trusted, " + r.byzantineCount + " byzantských");
      System.out.println("    Konsenzus: " + (r.consensus ? GREEN + "DOSIAHNUTÝ" + RESET : RED + "NEDOSIAHNUTÝ" + RESET));
      System.out.println("    Veľkosť konsenzuálneho setu: " + r.consensusSize + " transakcií");
      System.out.println("    Čas simulácie: " + r.elapsedMs + " ms");
   }

   // ==================== Test 1: rôzne parametre ====================

   /**
    * Test 1a: ľahšia kombinácia parametrov
    * numNodes=100, p_graph=0.1, p_malicious=0.15, p_txDistribution=0.01, numRounds=10
    */
   static void test1a_variousParams_lowByzantine() {
      int numNodes = 100;
      double p_graph = 0.1, p_byz = 0.15, p_tx = 0.01;
      int rounds = 10;
      SimResult r = runSimulation(numNodes, p_graph, p_byz, p_tx, rounds);
      printSimInfo("Ľahká kombinácia", numNodes, p_graph, p_byz, p_tx, rounds, r);
      assertTest("Konsenzus s ľahkými parametrami (p_malicious=0.15, p_graph=0.1)", r.consensus && r.consensusSize > 0);
   }

   /**
    * Test 1b: najťažšia kombinácia parametrov
    * numNodes=50, p_graph=0.1, p_malicious=0.45, p_txDistribution=0.10, numRounds=20
    */
   static void test1b_variousParams_highByzantine() {
      int numNodes = 50;
      double p_graph = 0.1, p_byz = 0.45, p_tx = 0.10;
      int rounds = 20;
      SimResult r = runSimulation(numNodes, p_graph, p_byz, p_tx, rounds);
      printSimInfo("Najťažšia kombinácia", numNodes, p_graph, p_byz, p_tx, rounds, r);
      if (!r.consensus) {
         System.out.println("    " + YELLOW + "⚠ POZOR: Pri najťažších parametroch konsenzus nebol dosiahnutý!" + RESET);
         System.out.println("    " + YELLOW + "  Skúste zvýšiť numRounds alebo vylepšiť TrustedNode algoritmus." + RESET);
      }
      assertTest("Konsenzus s najťažšími parametrami (p_malicious=0.45, p_graph=0.1)", r.consensus && r.consensusSize > 0);
   }

   // ==================== Test 2: konsenzus a čas ====================

   /**
    * Test 2a: meria čas dosiahnutia konsenzu s 10 kolami
    */
   static void test2a_consensusTime_fewRounds() {
      int numNodes = 100;
      double p_graph = 0.2, p_byz = 0.30, p_tx = 0.05;
      int rounds = 10;
      SimResult r = runSimulation(numNodes, p_graph, p_byz, p_tx, rounds);
      printSimInfo("Konsenzus 10 kôl", numNodes, p_graph, p_byz, p_tx, rounds, r);
      assertTest("Konsenzus dosiahnutý za " + r.elapsedMs + " ms (10 kôl)", r.consensus && r.consensusSize > 0);
   }

   /**
    * Test 2b: porovnáva 10 vs 20 kôl - viac kôl => lepší/rovnaký konsenzus
    */
   static void test2b_consensusTime_moreRounds() {
      int numNodes = 100;
      double p_graph = 0.2, p_byz = 0.30, p_tx = 0.10;

      SimResult r10 = runSimulation(numNodes, p_graph, p_byz, p_tx, 10);
      SimResult r20 = runSimulation(numNodes, p_graph, p_byz, p_tx, 20);

      printSimInfo("10 kôl", numNodes, p_graph, p_byz, p_tx, 10, r10);
      printSimInfo("20 kôl", numNodes, p_graph, p_byz, p_tx, 20, r20);

      System.out.println(CYAN + "  [Porovnanie]" + RESET);
      System.out.println("    10 kôl: " + r10.consensusSize + " tx, " + r10.elapsedMs + " ms"
         + (r10.consensus ? GREEN + " ✔" + RESET : RED + " ✘" + RESET));
      System.out.println("    20 kôl: " + r20.consensusSize + " tx, " + r20.elapsedMs + " ms"
         + (r20.consensus ? GREEN + " ✔" + RESET : RED + " ✘" + RESET));
      System.out.println("    Rozdiel v čase: +" + (r20.elapsedMs - r10.elapsedMs) + " ms za ďalších 10 kôl");

      assertTest("20 kôl dosiahne konsenzus s >= transakciami ako 10 kôl ("
         + r20.consensusSize + " >= " + r10.consensusSize + ")",
         r20.consensus && r20.consensusSize >= r10.consensusSize);
   }
}
