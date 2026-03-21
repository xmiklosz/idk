// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre TrustedNode (Fáza 2).
// Príklad simulácie. Tento test spúšťa uzly na náhodnom grafe.
// Na konci vypíše ID transakcií, na ktorých bol podľa uzlov
// dosiahnutý konsenzus. Túto simuláciu môžete použiť na
// otestovanie svojich uzlov. Budete chcieť vyskúšať vytvoriť nejaké podvodné uzly a
// zmiešať ich v sieti na úplné otestovanie.

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
 * Spustenie s argumentmi: java Simulation p_graph p_byzantine p_txDistribution numRounds
 * Spustenie všetkých testov: java Simulation
 */
public class Simulation {

   // ANSI farby pre prehľadný výstup
   private static final String GREEN = "\u001B[32m";
   private static final String RED = "\u001B[31m";
   private static final String YELLOW = "\u001B[33m";
   private static final String CYAN = "\u001B[36m";
   private static final String BOLD = "\u001B[1m";
   private static final String RESET = "\u001B[0m";

   private static int passed = 0;
   private static int failed = 0;

   public static void main(String[] args) {
      if (args.length == 4) {
         // Spustenie s argumentmi: java Simulation p_graph p_byzantine p_txDistribution numRounds
         runWithArgs(args);
      } else {
         // Spustenie všetkých testov
         runAllTests();
      }
   }

   /**
    * Spustenie simulácie s argumentmi z príkazového riadku.
    * Sú štyri požadované argumenty: p_graph (.1, .2, .3),
    * p_byzantine (.15, .30, .45), p_txDistribution (.01, .05, .10),
    * a numRounds (10, 20).
    */
   private static void runWithArgs(String[] args) {
      int numNodes = 100;
      double p_graph = Double.parseDouble(args[0]);
      double p_byzantine = Double.parseDouble(args[1]);
      double p_txDistribution = Double.parseDouble(args[2]);
      int numRounds = Integer.parseInt(args[3]);

      System.out.println(BOLD + "╔════════════════════════════════════════════════════════════╗");
      System.out.println("║          TrustedNode Simulácia (Fáza 2)                    ║");
      System.out.println("╚════════════════════════════════════════════════════════════╝" + RESET);
      System.out.println("  Parametre: p_graph=" + p_graph
         + ", p_malicious=" + p_byzantine
         + ", p_txDistribution=" + p_txDistribution
         + ", numRounds=" + numRounds);

      long startTime = System.currentTimeMillis();

      // vyberte, ktoré uzly sú byzantské a ktorým dôverujeme
      Node[] nodes = new Node[numNodes];
      boolean[] isByzantine = new boolean[numNodes];
      int byzantineCount = 0;
      for (int i = 0; i < numNodes; i++) {
         if (Math.random() < p_byzantine) {
            nodes[i] = new ByzantineNode(p_graph, p_byzantine, p_txDistribution, numRounds);
            isByzantine[i] = true;
            byzantineCount++;
         } else {
            nodes[i] = new TrustedNode(p_graph, p_byzantine, p_txDistribution, numRounds);
         }
      }

      System.out.println("  Uzly: " + (numNodes - byzantineCount) + " trusted, " + byzantineCount + " byzantských");

      // inicializovať náhodné sledovanie grafu
      boolean[][] followees = new boolean[numNodes][numNodes];
      for (int i = 0; i < numNodes; i++) {
         for (int j = 0; j < numNodes; j++) {
            if (i == j) continue;
            if (Math.random() < p_graph) {
               followees[i][j] = true;
            }
         }
      }

      // upozorni všetky uzly o ich nasledovníkoch
      for (int i = 0; i < numNodes; i++)
         nodes[i].followeesSet(followees[i]);

      // inicializuj set 500 platných transakcií s náhodnými id
      int numTx = 500;
      HashSet<Integer> validTxIds = new HashSet<Integer>();
      Random random = new Random();
      for (int i = 0; i < numTx; i++) {
         int r = random.nextInt();
         validTxIds.add(r);
      }

      // distribuuje 500 transakcií do všetkých uzlov
      for (int i = 0; i < numNodes; i++) {
         HashSet<Transaction> pendingTransactions = new HashSet<Transaction>();
         for (Integer txID : validTxIds) {
            if (Math.random() < p_txDistribution)
               pendingTransactions.add(new Transaction(txID));
         }
         nodes[i].pendingTransactionSet(pendingTransactions);
      }

      // Simuluj numRounds-krát
      for (int round = 0; round < numRounds; round++) {
         HashMap<Integer, ArrayList<Integer[]>> allProposals = new HashMap<>();

         for (int i = 0; i < numNodes; i++) {
            Set<Transaction> proposals = nodes[i].followersSend();
            for (Transaction tx : proposals) {
               if (!validTxIds.contains(tx.id)) continue;

               for (int j = 0; j < numNodes; j++) {
                  if (!followees[j][i]) continue;

                  if (allProposals.containsKey(j)) {
                     Integer[] candidate = new Integer[2];
                     candidate[0] = tx.id;
                     candidate[1] = i;
                     allProposals.get(j).add(candidate);
                  } else {
                     ArrayList<Integer[]> candidates = new ArrayList<Integer[]>();
                     Integer[] candidate = new Integer[2];
                     candidate[0] = tx.id;
                     candidate[1] = i;
                     candidates.add(candidate);
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

      long elapsed = System.currentTimeMillis() - startTime;

      // Skontroluj konsenzus medzi trusted uzlami
      Set<Transaction> referenceSet = null;
      boolean allAgree = true;

      for (int i = 0; i < numNodes; i++) {
         if (isByzantine[i]) continue;

         Set<Transaction> nodeResult = nodes[i].followersSend();
         if (referenceSet == null) {
            referenceSet = nodeResult;
         } else {
            if (!referenceSet.equals(nodeResult)) {
               allAgree = false;
            }
         }
      }

      int consensusSize = (referenceSet != null) ? referenceSet.size() : 0;

      System.out.println("  Veľkosť konsenzuálneho setu: " + consensusSize + " transakcií");
      System.out.println("  Čas simulácie: " + elapsed + " ms");

      if (allAgree && consensusSize > 0) {
         System.out.println("\n  " + GREEN + "✔ PASS" + RESET + " - Konsenzus dosiahnutý medzi všetkými trusted uzlami");
      } else {
         System.out.println("\n  " + RED + "✘ FAIL" + RESET + " - Konsenzus nedosiahnutý");
      }
   }

   // ==================== Spustenie všetkých testov ====================

   private static void runAllTests() {
      System.out.println(BOLD + "╔════════════════════════════════════════════════════════════╗");
      System.out.println("║          TrustedNode Unit Testy (Fáza 2)                   ║");
      System.out.println("╚════════════════════════════════════════════════════════════╝" + RESET);

      System.out.println("\n" + BOLD + "--- Test 1: rôzne parametre (numNodes, p_graph, p_malicious, p_txDistribution, numRounds) ---" + RESET);
      runTest("1a", 100, 0.1, 0.45, 0.01, 10);
      runTest("1b", 100, 0.1, 0.45, 0.01, 20);

      System.out.println("\n" + BOLD + "--- Test 2: konsenzus medzi uzlami + čas dosiahnutia konsenzu ---" + RESET);
      runTest("2a", 100, 0.2, 0.45, 0.01, 10);
      runTest("2b", 100, 0.3, 0.45, 0.01, 10);

      System.out.println();
      System.out.println(BOLD + "╔════════════════════════════════════════════════════════════╗");
      if (failed == 0) {
         System.out.println("║  " + GREEN + "VŠETKY TESTY ÚSPEŠNÉ: " + passed + "/" + (passed + failed) + RESET + BOLD + "                              ║");
      } else {
         System.out.println("║  " + RED + "NEÚSPEŠNÉ: " + failed + " z " + (passed + failed) + RESET + BOLD + "                                      ║");
      }
      System.out.println("╚════════════════════════════════════════════════════════════╝" + RESET);
   }

   /**
    * Spustí jeden test so seedovaným randomom pre reprodukovateľnosť.
    */
   private static void runTest(String label, int numNodes, double p_graph, double p_byzantine,
         double p_txDistribution, int numRounds) {

      int numTx = 500;
      Random random = new Random(42);

      long startTime = System.currentTimeMillis();

      // Vytvor uzly
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
         }
      }

      // Inicializuj graf
      boolean[][] followees = new boolean[numNodes][numNodes];
      for (int i = 0; i < numNodes; i++) {
         for (int j = 0; j < numNodes; j++) {
            if (i == j) continue;
            if (random.nextDouble() < p_graph) {
               followees[i][j] = true;
            }
         }
      }

      for (int i = 0; i < numNodes; i++)
         nodes[i].followeesSet(followees[i]);

      // Vytvor transakcie
      HashSet<Integer> validTxIds = new HashSet<Integer>();
      for (int i = 0; i < numTx; i++) {
         validTxIds.add(random.nextInt());
      }

      // Distribuuj transakcie
      for (int i = 0; i < numNodes; i++) {
         HashSet<Transaction> pendingTransactions = new HashSet<Transaction>();
         for (Integer txID : validTxIds) {
            if (random.nextDouble() < p_txDistribution)
               pendingTransactions.add(new Transaction(txID));
         }
         nodes[i].pendingTransactionSet(pendingTransactions);
      }

      // Simuluj
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

         for (int i = 0; i < numNodes; i++) {
            if (allProposals.containsKey(i))
               nodes[i].followeesReceive(allProposals.get(i));
         }
      }

      long elapsed = System.currentTimeMillis() - startTime;

      // Skontroluj konsenzus
      Set<Transaction> referenceSet = null;
      boolean allAgree = true;

      for (int i = 0; i < numNodes; i++) {
         if (isByzantine[i]) continue;

         Set<Transaction> nodeResult = nodes[i].followersSend();
         if (referenceSet == null) {
            referenceSet = nodeResult;
         } else {
            if (!referenceSet.equals(nodeResult)) {
               allAgree = false;
            }
         }
      }

      int consensusSize = (referenceSet != null) ? referenceSet.size() : 0;
      boolean testPassed = allAgree && consensusSize > 0;

      System.out.println(CYAN + "  [Test " + label + "]" + RESET);
      System.out.println("    Parametre: p_graph=" + p_graph
         + ", p_malicious=" + p_byzantine
         + ", p_txDistribution=" + p_txDistribution
         + ", numRounds=" + numRounds);
      System.out.println("    Uzly: " + (numNodes - byzantineCount) + " trusted, " + byzantineCount + " byzantských");
      System.out.println("    Konsenzus: " + (allAgree ? GREEN + "DOSIAHNUTÝ" + RESET : RED + "NEDOSIAHNUTÝ" + RESET));
      System.out.println("    Veľkosť konsenzuálneho setu: " + consensusSize + " transakcií");
      System.out.println("    Čas simulácie: " + elapsed + " ms");

      if (testPassed) {
         System.out.println("  " + GREEN + "✔ PASS" + RESET);
         passed++;
      } else {
         System.out.println("  " + RED + "✘ FAIL" + RESET);
         failed++;
      }
   }
}
