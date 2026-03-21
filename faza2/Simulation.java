// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre TrustedNode (Fáza 2).
// Príklad simulácie. Tento test spúšťa uzly na náhodnom grafe.
// Na konci vypíše ID transakcií, na ktorých bol podľa uzlov
// dosiahnutý konsenzus. Túto simuláciu môžete použiť na
// otestovanie svojich uzlov.

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
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

   private static int passed = 0;
   private static int failed = 0;

   public static void main(String[] args) {
      if (args.length == 4) {
         runWithArgs(args);
      } else {
         runAllTests();
      }
   }

   private static void runWithArgs(String[] args) {
      int numNodes = 100;
      double p_graph = Double.parseDouble(args[0]);
      double p_byzantine = Double.parseDouble(args[1]);
      double p_txDistribution = Double.parseDouble(args[2]);
      int numRounds = Integer.parseInt(args[3]);

      boolean consensus = runSimulation(numNodes, p_graph, p_byzantine, p_txDistribution, numRounds, new Random());

      if (consensus) {
         System.out.println("PASS - konsenzus dosiahnuty (" + p_graph + ", " + p_byzantine + ", " + p_txDistribution + ", " + numRounds + ")");
      } else {
         System.out.println("FAIL - konsenzus nedosiahnuty (" + p_graph + ", " + p_byzantine + ", " + p_txDistribution + ", " + numRounds + ")");
      }
   }

   private static void runAllTests() {
      System.out.println("TrustedNode Unit Testy (Faza 2)");
      System.out.println("================================");

      System.out.println("\nTest 1: rozne parametre (numNodes, p_graph, p_malicious, p_txDistribution, numRounds)");
      check("1a: p_graph=0.1 p_malicious=0.45 p_tx=0.01 rounds=10", runSimulation(100, 0.1, 0.45, 0.01, 10, new Random(42)));
      check("1b: p_graph=0.1 p_malicious=0.45 p_tx=0.01 rounds=20", runSimulation(100, 0.1, 0.45, 0.01, 20, new Random(42)));

      System.out.println("\nTest 2: konsenzus medzi uzlami + cas dosiahnutia konsenzu");
      long t1 = System.currentTimeMillis();
      boolean r2a = runSimulation(100, 0.2, 0.45, 0.01, 10, new Random(42));
      long time2a = System.currentTimeMillis() - t1;

      long t2 = System.currentTimeMillis();
      boolean r2b = runSimulation(100, 0.3, 0.45, 0.01, 10, new Random(42));
      long time2b = System.currentTimeMillis() - t2;

      check("2a: p_graph=0.2 p_malicious=0.45 p_tx=0.01 rounds=10 (" + time2a + "ms)", r2a);
      check("2b: p_graph=0.3 p_malicious=0.45 p_tx=0.01 rounds=10 (" + time2b + "ms)", r2b);

      System.out.println("\n================================");
      System.out.println("Vysledok: " + passed + "/" + (passed + failed) + " testov uspesnych");
   }

   private static void check(String name, boolean condition) {
      if (condition) {
         System.out.println("  PASS: " + name);
         passed++;
      } else {
         System.out.println("  FAIL: " + name);
         failed++;
      }
   }

   /**
    * Spusti simulaciu a vrati true ak bol dosiahnuty konsenzus medzi trusted uzlami.
    */
   private static boolean runSimulation(int numNodes, double p_graph, double p_byzantine,
         double p_txDistribution, int numRounds, Random random) {

      int numTx = 500;

      // Vytvor uzly
      Node[] nodes = new Node[numNodes];
      boolean[] isByzantine = new boolean[numNodes];
      for (int i = 0; i < numNodes; i++) {
         if (random.nextDouble() < p_byzantine) {
            nodes[i] = new ByzantineNode(p_graph, p_byzantine, p_txDistribution, numRounds);
            isByzantine[i] = true;
         } else {
            nodes[i] = new TrustedNode(p_graph, p_byzantine, p_txDistribution, numRounds);
         }
      }

      // Inicializuj graf
      boolean[][] followees = new boolean[numNodes][numNodes];
      for (int i = 0; i < numNodes; i++) {
         for (int j = 0; j < numNodes; j++) {
            if (i == j) continue;
            if (random.nextDouble() < p_graph)
               followees[i][j] = true;
         }
      }

      for (int i = 0; i < numNodes; i++)
         nodes[i].followeesSet(followees[i]);

      // Vytvor transakcie
      HashSet<Integer> validTxIds = new HashSet<Integer>();
      for (int i = 0; i < numTx; i++)
         validTxIds.add(random.nextInt());

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

      // Vyhodnotenie zhody medzi dobrymi uzlami
      System.out.println("-------------------------------------------");
      System.out.println("        VYHODNOTENIE ZHODY UZLOV");
      System.out.println("-------------------------------------------");

      ArrayList<Integer> goodNodes = new ArrayList<>();
      HashMap<Integer, Set<Transaction>> resultsMap = new HashMap<>();

      for (int idx = 0; idx < numNodes; idx++) {
         if (!isByzantine[idx]) {
            goodNodes.add(idx);
            resultsMap.put(idx, nodes[idx].followersSend());
         }
      }

      int numByzantine = numNodes - goodNodes.size();
      System.out.println("Pocet uzlov spolu : " + numNodes);
      System.out.println("Doverhodne uzly   : " + goodNodes.size());
      System.out.println("Podvodne uzly     : " + numByzantine);
      System.out.println();

      if (goodNodes.isEmpty()) {
         System.out.println("Ziadne doverhodne uzly v sieti!");
         System.out.println("-------------------------------------------");
         return false;
      }

      int firstGood = goodNodes.get(0);
      Set<Transaction> baseline = resultsMap.get(firstGood);
      int disagreements = 0;

      for (int k = 1; k < goodNodes.size(); k++) {
         int nodeId = goodNodes.get(k);
         Set<Transaction> nodeOut = resultsMap.get(nodeId);
         if (!baseline.equals(nodeOut)) {
            disagreements++;
         }
      }

      if (disagreements == 0) {
         System.out.println("VYSLEDOK: USPECH - vsetky doverhodne uzly sa zhoduju!");
         System.out.println("Prijate transakcie: " + baseline.size() + " z " + validTxIds.size());
      } else {
         System.out.println("VYSLEDOK: NEUSPECH - uzly sa nezhoduju.");
         System.out.println("Pocet nezhod: " + disagreements + " z " + goodNodes.size());
         System.out.println();

         for (int k = 1; k < goodNodes.size(); k++) {
            int nid = goodNodes.get(k);
            Set<Transaction> nout = resultsMap.get(nid);
            if (!nout.equals(baseline)) {
               HashSet<Transaction> chybajuce = new HashSet<>(baseline);
               chybajuce.removeAll(nout);

               HashSet<Transaction> navyse = new HashSet<>(nout);
               navyse.removeAll(baseline);

               System.out.println("  Uzol " + nid + " vs Uzol " + firstGood + ":");
               System.out.println("    Chybajuce tx : " + chybajuce.size());
               System.out.println("    Navyse tx    : " + navyse.size());
            }
         }

         Set<Transaction> spolocne = new HashSet<>(baseline);
         for (Set<Transaction> vo : resultsMap.values()) {
            spolocne.retainAll(vo);
         }
         System.out.println();
         System.out.println("Tx na ktorych sa zhodli vsetci: " + spolocne.size() + " z " + validTxIds.size());
      }

      System.out.println("-------------------------------------------");
      return disagreements == 0 && baseline.size() > 0;
   }
}
