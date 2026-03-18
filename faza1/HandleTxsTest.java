// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre HandleTxs a MaxFeeHandleTxs.

import java.security.SignatureException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * JUnit testy pre HandleTxs (Fáza 1).
 * Pokrýva 15 testov pre HandleTxs a 3 testy pre MaxFeeHandleTxs.
 *
 * Keďže JUnit nemusí byť k dispozícii, testy sú implementované
 * ako statické metódy s assert logikou a main() metódou.
 */
public class HandleTxsTest {

    // Pomocná trieda na podpisovanie transakcií
    public static class Tx extends Transaction {
        public void signTx(RSAKey sk, int input) throws SignatureException {
            byte[] sig = sk.sign(this.getDataToSign(input));
            this.addSignature(sig, input);
            this.finalize();
        }
    }

    private static RSAKeyPair pk_alice;
    private static RSAKeyPair pk_bob;
    private static RSAKeyPair pk_cyril;
    private static RSAKeyPair pk_dave;

    private static int passed = 0;
    private static int failed = 0;

    public static void main(String[] args) throws Exception {
        // Inicializácia kľúčov
        byte[] key_alice = new byte[32];
        byte[] key_bob = new byte[32];
        byte[] key_cyril = new byte[32];
        byte[] key_dave = new byte[32];
        for (int i = 0; i < 32; i++) {
            key_alice[i] = (byte) 0;
            key_bob[i] = (byte) 1;
            key_cyril[i] = (byte) 2;
            key_dave[i] = (byte) 3;
        }
        pk_alice = new RSAKeyPair(new PRGen(key_alice), 265);
        pk_bob = new RSAKeyPair(new PRGen(key_bob), 265);
        pk_cyril = new RSAKeyPair(new PRGen(key_cyril), 265);
        pk_dave = new RSAKeyPair(new PRGen(key_dave), 265);

        System.out.println("=== HandleTxs Tests ===");

        // HandleTxs testy
        test1_txIsValid_validTx();
        test2_txIsValid_incorrectSignatureData();
        test3_txIsValid_invalidPrivateKey();
        test4_txIsValid_outputExceedsInput();
        test5_txIsValid_outputNotInPool();
        test6_txIsValid_doubleClaimUTXO();
        test7_txIsValid_negativeOutput();
        test8_handler_simpleValidTxs();
        test9_handler_someInvalidSignatures();
        test10_handler_inputLessThanOutput();
        test11_handler_doubleSpend();
        test12_handler_dependentTxs();
        test13_handler_nonExistentUTXO();
        test14_handler_complexTxs();
        test15_handler_calledTwice();

        System.out.println("\n=== MaxFeeHandleTxs Tests ===");

        // MaxFeeHandleTxs testy
        testMaxFee1_simpleValidTxs();
        testMaxFee2_doubleClaimOutput();
        testMaxFee3_complexMixed();

        System.out.println("\n=== Výsledky ===");
        System.out.println("Úspešných: " + passed + " / " + (passed + failed));
        System.out.println("Neúspešných: " + failed);
    }

    // Helper: vytvorí počiatočnú (genesis-like) transakciu s danou hodnotou pre daného príjemcu
    private static Transaction createInitialTx(double value, RSAKey address) {
        Tx tx = new Tx();
        tx.addOutput(value, address);
        tx.finalize();
        return tx;
    }

    private static UTXOPool createPoolWithTx(Transaction tx) {
        UTXOPool pool = new UTXOPool();
        for (int i = 0; i < tx.numOutputs(); i++) {
            UTXO utxo = new UTXO(tx.getHash(), i);
            pool.addUTXO(utxo, tx.getOutput(i));
        }
        return pool;
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

    // ==================== HandleTxs testy ====================

    /** Test 1: test txIsValid() s platnými transakciami */
    static void test1_txIsValid_validTx() throws Exception {
        System.out.println("Test 1: txIsValid s platnými transakciami");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addOutput(5.0, pk_bob.getPublicKey());
        tx.addOutput(4.0, pk_cyril.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);

        assertTest("Platná transakcia by mala byť platná", handler.txIsValid(tx));
    }

    /** Test 2: test txIsValid() s transakciami obsahujúcimi podpisy nekorektných dát */
    static void test2_txIsValid_incorrectSignatureData() throws Exception {
        System.out.println("Test 2: txIsValid s nekorektnými dátami podpisu");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        // Vytvoríme transakciu, podpíšeme ju, potom zmeníme výstupy (nekorektné dáta)
        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addOutput(5.0, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);

        // Vytvoríme novú tx s iným výstupom ale rovnakým podpisom
        Tx tx2 = new Tx();
        tx2.addInput(initial.getHash(), 0);
        tx2.addOutput(9.0, pk_cyril.getPublicKey()); // iný výstup
        // Použijeme podpis z prvej transakcie (nesprávne dáta)
        tx2.addSignature(tx.getInput(0).signature, 0);
        tx2.finalize();

        assertTest("Transakcia s nekorektným podpisom by nemala byť platná", !handler.txIsValid(tx2));
    }

    /** Test 3: test txIsValid() s transakciami obsahujúcimi podpisy použitím neplatných privátnych kľúčov */
    static void test3_txIsValid_invalidPrivateKey() throws Exception {
        System.out.println("Test 3: txIsValid s neplatným privátnym kľúčom");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addOutput(5.0, pk_bob.getPublicKey());
        // Podpísané Bobovým kľúčom namiesto Alicinho
        tx.signTx(pk_bob.getPrivateKey(), 0);

        assertTest("Transakcia podpísaná nesprávnym kľúčom by nemala byť platná", !handler.txIsValid(tx));
    }

    /** Test 4: test txIsValid() s transakciami, ktorých celkový output prekračuje celkový input */
    static void test4_txIsValid_outputExceedsInput() throws Exception {
        System.out.println("Test 4: txIsValid s output > input");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addOutput(11.0, pk_bob.getPublicKey()); // viac ako 10
        tx.signTx(pk_alice.getPrivateKey(), 0);

        assertTest("Transakcia kde output > input by nemala byť platná", !handler.txIsValid(tx));
    }

    /** Test 5: test txIsValid() s transakciami, ktoré deklarujú outputy mimo aktuálneho utxoPool */
    static void test5_txIsValid_outputNotInPool() throws Exception {
        System.out.println("Test 5: txIsValid s UTXO mimo poolu");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        // Vytvoríme falošný hash
        byte[] fakeHash = new byte[32];
        Arrays.fill(fakeHash, (byte) 99);

        Tx tx = new Tx();
        tx.addInput(fakeHash, 0);
        tx.addOutput(5.0, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);

        assertTest("Transakcia s neexistujúcim UTXO by nemala byť platná", !handler.txIsValid(tx));
    }

    /** Test 6: test txIsValid() s transakciami, ktoré deklarujú rovnaký UTXO viackrát */
    static void test6_txIsValid_doubleClaimUTXO() throws Exception {
        System.out.println("Test 6: txIsValid s double-claimed UTXO");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addInput(initial.getHash(), 0); // rovnaký UTXO dvakrát
        tx.addOutput(15.0, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);
        tx.signTx(pk_alice.getPrivateKey(), 1);

        assertTest("Transakcia s double-claimed UTXO by nemala byť platná", !handler.txIsValid(tx));
    }

    /** Test 7: test txIsValid() s transakciami, ktoré obsahujú zápornú output hodnotu */
    static void test7_txIsValid_negativeOutput() throws Exception {
        System.out.println("Test 7: txIsValid so záporným output");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx = new Tx();
        tx.addInput(initial.getHash(), 0);
        tx.addOutput(-5.0, pk_bob.getPublicKey());
        tx.addOutput(14.0, pk_cyril.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);

        assertTest("Transakcia so záporným output by nemala byť platná", !handler.txIsValid(tx));
    }

    /** Test 8: test handleTxs() s jednoduchými a platnými transakciami */
    static void test8_handler_simpleValidTxs() throws Exception {
        System.out.println("Test 8: handler s jednoduchými platnými tx");
        Transaction initial1 = createInitialTx(10.0, pk_alice.getPublicKey());
        Transaction initial2 = createInitialTx(20.0, pk_bob.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial1);
        // Pridaj aj initial2
        for (int i = 0; i < initial2.numOutputs(); i++) {
            pool.addUTXO(new UTXO(initial2.getHash(), i), initial2.getOutput(i));
        }
        HandleTxs handler = new HandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial1.getHash(), 0);
        tx1.addOutput(5.0, pk_bob.getPublicKey());
        tx1.addOutput(5.0, pk_cyril.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Tx tx2 = new Tx();
        tx2.addInput(initial2.getHash(), 0);
        tx2.addOutput(10.0, pk_alice.getPublicKey());
        tx2.addOutput(10.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Obe platné transakcie by mali byť prijaté", result.length == 2);
    }

    /** Test 9: test handleTxs() s neplatnými podpismi */
    static void test9_handler_someInvalidSignatures() throws Exception {
        System.out.println("Test 9: handler s neplatnými podpismi");
        Transaction initial1 = createInitialTx(10.0, pk_alice.getPublicKey());
        Transaction initial2 = createInitialTx(20.0, pk_bob.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial1);
        for (int i = 0; i < initial2.numOutputs(); i++) {
            pool.addUTXO(new UTXO(initial2.getHash(), i), initial2.getOutput(i));
        }
        HandleTxs handler = new HandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial1.getHash(), 0);
        tx1.addOutput(5.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0); // platná

        Tx tx2 = new Tx();
        tx2.addInput(initial2.getHash(), 0);
        tx2.addOutput(10.0, pk_alice.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0); // NEplatná - podpísaná Alicou, nie Bobom

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Len platná tx by mala byť prijatá", result.length == 1);
    }

    /** Test 10: test handleTxs() s input < output */
    static void test10_handler_inputLessThanOutput() throws Exception {
        System.out.println("Test 10: handler s input < output");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(5.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0); // platná

        Tx tx2 = new Tx();
        tx2.addInput(initial.getHash(), 0);
        tx2.addOutput(15.0, pk_bob.getPublicKey()); // output > input
        tx2.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Len tx s input >= output by mala byť prijatá", result.length == 1);
    }

    /** Test 11: test handleTxs() s double-spend */
    static void test11_handler_doubleSpend() throws Exception {
        System.out.println("Test 11: handler s double spend");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(10.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Tx tx2 = new Tx();
        tx2.addInput(initial.getHash(), 0); // rovnaký UTXO ako tx1
        tx2.addOutput(10.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Len jedna z double-spend tx by mala byť prijatá", result.length == 1);
    }

    /** Test 12: test handleTxs() s platnými tx, kde niektoré závisia od iných tx */
    static void test12_handler_dependentTxs() throws Exception {
        System.out.println("Test 12: handler so závislými tx");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        // tx1: Alice -> Bob (10)
        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(10.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        // tx2: Bob -> Cyril (závisí na tx1)
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(10.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        // Poradie: tx2 pred tx1 (neusporiadané)
        Transaction[] result = handler.handler(new Transaction[]{tx2, tx1});
        assertTest("Obe závislé tx by mali byť prijaté", result.length == 2);
    }

    /** Test 13: test handleTxs() s neexistujúcimi UTXO */
    static void test13_handler_nonExistentUTXO() throws Exception {
        System.out.println("Test 13: handler s neexistujúcim UTXO");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(10.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        byte[] fakeHash = new byte[32];
        Arrays.fill(fakeHash, (byte) 42);
        Tx tx2 = new Tx();
        tx2.addInput(fakeHash, 0);
        tx2.addOutput(5.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Len tx s existujúcim UTXO by mala byť prijatá", result.length == 1);
    }

    /** Test 14: test handleTxs() s komplexnými transakciami */
    static void test14_handler_complexTxs() throws Exception {
        System.out.println("Test 14: handler s komplexnými tx");
        Transaction initial1 = createInitialTx(10.0, pk_alice.getPublicKey());
        Transaction initial2 = createInitialTx(20.0, pk_bob.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial1);
        for (int i = 0; i < initial2.numOutputs(); i++) {
            pool.addUTXO(new UTXO(initial2.getHash(), i), initial2.getOutput(i));
        }
        HandleTxs handler = new HandleTxs(pool);

        // tx1: Alice -> Bob (5), Alice -> Cyril (5)
        Tx tx1 = new Tx();
        tx1.addInput(initial1.getHash(), 0);
        tx1.addOutput(5.0, pk_bob.getPublicKey());
        tx1.addOutput(5.0, pk_cyril.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        // tx2: Bob -> Dave (15) z initial2 a výstupu tx1
        Tx tx2 = new Tx();
        tx2.addInput(initial2.getHash(), 0);
        tx2.addInput(tx1.getHash(), 0); // závislosť na tx1
        tx2.addOutput(25.0, pk_dave.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        tx2.signTx(pk_bob.getPrivateKey(), 1);

        // tx3: Cyril -> Alice z výstupu tx1
        Tx tx3 = new Tx();
        tx3.addInput(tx1.getHash(), 1); // závislosť na tx1
        tx3.addOutput(4.0, pk_alice.getPublicKey());
        tx3.signTx(pk_cyril.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx3, tx2, tx1});
        assertTest("Všetky 3 komplexné tx by mali byť prijaté", result.length == 3);
    }

    /** Test 15: test handleTxs() zavolaný dvakrát na overenie zmien v poole */
    static void test15_handler_calledTwice() throws Exception {
        System.out.println("Test 15: handler zavolaný dvakrát");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        HandleTxs handler = new HandleTxs(pool);

        // Prvé volanie: Alice -> Bob (10)
        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(10.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result1 = handler.handler(new Transaction[]{tx1});
        assertTest("Prvé volanie: tx by mala byť prijatá", result1.length == 1);

        // Druhé volanie: Bob -> Cyril (10) - používa výstup z tx1
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(10.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        Transaction[] result2 = handler.handler(new Transaction[]{tx2});
        assertTest("Druhé volanie: tx používajúca nový UTXO by mala byť prijatá", result2.length == 1);

        // Tretie volanie: Pokus o double spend
        Tx tx3 = new Tx();
        tx3.addInput(initial.getHash(), 0); // už minutý UTXO
        tx3.addOutput(10.0, pk_dave.getPublicKey());
        tx3.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result3 = handler.handler(new Transaction[]{tx3});
        assertTest("Tretie volanie: double spend by nemal byť prijatý", result3.length == 0);
    }

    // ==================== MaxFeeHandleTxs testy ====================

    /** MaxFee Test 1: jednoduché platné transakcie */
    static void testMaxFee1_simpleValidTxs() throws Exception {
        System.out.println("MaxFee Test 1: jednoduché platné tx");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        MaxFeeHandleTxs handler = new MaxFeeHandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(8.0, pk_bob.getPublicKey()); // fee = 2
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1});
        assertTest("Platná tx by mala byť prijatá", result.length == 1);
    }

    /** MaxFee Test 2: dve transakcie deklarujú rovnaký output - mala by byť vybraná tá s vyšším fee */
    static void testMaxFee2_doubleClaimOutput() throws Exception {
        System.out.println("MaxFee Test 2: double claim, výber podľa fee");
        Transaction initial = createInitialTx(10.0, pk_alice.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial);
        MaxFeeHandleTxs handler = new MaxFeeHandleTxs(pool);

        Tx tx1 = new Tx();
        tx1.addInput(initial.getHash(), 0);
        tx1.addOutput(9.0, pk_bob.getPublicKey()); // fee = 1
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Tx tx2 = new Tx();
        tx2.addInput(initial.getHash(), 0);
        tx2.addOutput(7.0, pk_cyril.getPublicKey()); // fee = 3
        tx2.signTx(pk_alice.getPrivateKey(), 0);

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2});
        assertTest("Len jedna tx by mala byť prijatá", result.length == 1);
        // tx2 má vyšší fee, takže by mala byť vybraná
        boolean hasCyrilOutput = false;
        for (Transaction tx : result) {
            if (tx.getOutput(0).value == 7.0) {
                hasCyrilOutput = true;
            }
        }
        assertTest("Tx s vyšším fee by mala byť vybraná", hasCyrilOutput);
    }

    /** MaxFee Test 3: komplexné transakcie, niektoré platné, niektoré nie */
    static void testMaxFee3_complexMixed() throws Exception {
        System.out.println("MaxFee Test 3: komplexné tx, mix platných a neplatných");
        Transaction initial1 = createInitialTx(10.0, pk_alice.getPublicKey());
        Transaction initial2 = createInitialTx(20.0, pk_bob.getPublicKey());
        UTXOPool pool = createPoolWithTx(initial1);
        for (int i = 0; i < initial2.numOutputs(); i++) {
            pool.addUTXO(new UTXO(initial2.getHash(), i), initial2.getOutput(i));
        }
        MaxFeeHandleTxs handler = new MaxFeeHandleTxs(pool);

        // Platná tx1: Alice -> Bob, fee = 2
        Tx tx1 = new Tx();
        tx1.addInput(initial1.getHash(), 0);
        tx1.addOutput(8.0, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        // Platná tx2: Bob -> Cyril, fee = 5
        Tx tx2 = new Tx();
        tx2.addInput(initial2.getHash(), 0);
        tx2.addOutput(15.0, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        // Neplatná tx3: nesprávny podpis
        Tx tx3 = new Tx();
        tx3.addInput(initial1.getHash(), 0);
        tx3.addOutput(5.0, pk_dave.getPublicKey());
        tx3.signTx(pk_bob.getPrivateKey(), 0); // nesprávny kľúč

        Transaction[] result = handler.handler(new Transaction[]{tx1, tx2, tx3});
        // tx2 má vyšší fee, takže by mala byť vybraná prvá
        // tx1 alebo tx3 - tx3 je neplatná, tx1 je platná (ale double spend s tx3 na initial1)
        // tx1 a tx2 sú nezávislé, obidve by mali byť prijaté
        assertTest("Len platné tx by mali byť prijaté (2)", result.length == 2);
    }
}
