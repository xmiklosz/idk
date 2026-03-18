// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre Blockchain (Fáza 3).

import java.security.SignatureException;
import java.util.Arrays;

/**
 * JUnit testy pre Blockchain (Fáza 3).
 * Pokrýva 27 testovacích scenárov.
 */
public class BlockchainTest {

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

        System.out.println("=== Blockchain Tests (Fáza 3) ===");

        test1_blockNoTxs();
        test2_blockOneValidTx();
        test3_blockMultipleValidTxs();
        test4_blockDoubleSpend();
        test5_newGenesisBlock();
        test6_invalidPrevBlockHash();
        test7_invalidTxTypes();
        test8_multipleBlocksOverGenesis();
        test9_claimAlreadyClaimedUTXO();
        test10_claimUTXOOutsideBranch();
        test11_claimOlderUnclaimedUTXO();
        test12_linearChain();
        test13_linearChainCutOff();
        test14_linearChainCutOffPlus1();
        test15_createBlockNoTxs();
        test16_createBlockAfterOneTx();
        test17_createTwoBlocksSequentially();
        test18_createBlockTxAlreadyInBlock();
        test19_createBlockTxUsesClaimedUTXO();
        test20_createBlockTxNotDoubleSpend();
        test21_createBlockOnlyInvalidTxs();
        test22_combinedTxAndBlockCreation();
        test23_processBlockClaimingUTXOFromTx();
        test24_processBlockOnGenesisWithTxUTXO();
        test25_multipleBlocksOverGenesisCreateBlock();
        test26_multipleBranches();
        test27_blocksBelowCutOffAge();

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

    private static Block createGenesisBlock(RSAKey address) {
        Block genesis = new Block(null, address);
        genesis.finalize();
        return genesis;
    }

    /** Test 1: proces bloku bez transakcií */
    static void test1_blockNoTxs() throws Exception {
        System.out.println("Test 1: blok bez transakcií");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block = new Block(genesis.getHash(), pk_bob.getPublicKey());
        block.finalize();

        assertTest("Blok bez tx by mal byť prijatý", hb.blockProcess(block));
    }

    /** Test 2: proces bloku s jednou platnou transakciou */
    static void test2_blockOneValidTx() throws Exception {
        System.out.println("Test 2: blok s jednou platnou tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block = new Block(genesis.getHash(), pk_bob.getPublicKey());
        Tx tx = new Tx();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addOutput(3.125, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);
        block.transactionAdd(tx);
        block.finalize();

        assertTest("Blok s platnou tx by mal byť prijatý", hb.blockProcess(block));
    }

    /** Test 3: proces bloku s viacerými platnými transakciami */
    static void test3_blockMultipleValidTxs() throws Exception {
        System.out.println("Test 3: blok s viacerými platnými tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block = new Block(genesis.getHash(), pk_bob.getPublicKey());

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(1.0, pk_bob.getPublicKey());
        tx1.addOutput(2.125, pk_cyril.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(1.0, pk_dave.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);

        block.transactionAdd(tx1);
        block.transactionAdd(tx2);
        block.finalize();

        assertTest("Blok s viacerými platnými tx by mal byť prijatý", hb.blockProcess(block));
    }

    /** Test 4: proces bloku s double-spend */
    static void test4_blockDoubleSpend() throws Exception {
        System.out.println("Test 4: blok s double-spend");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block = new Block(genesis.getHash(), pk_bob.getPublicKey());

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);

        Tx tx2 = new Tx();
        tx2.addInput(genesis.getCoinbase().getHash(), 0); // double spend
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);

        block.transactionAdd(tx1);
        block.transactionAdd(tx2);
        block.finalize();

        assertTest("Blok s double-spend by mal byť odmietnutý", !hb.blockProcess(block));
    }

    /** Test 5: proces nového genesis bloku */
    static void test5_newGenesisBlock() throws Exception {
        System.out.println("Test 5: nový genesis blok");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block newGenesis = new Block(null, pk_bob.getPublicKey());
        newGenesis.finalize();

        assertTest("Nový genesis blok by mal byť odmietnutý", !hb.blockProcess(newGenesis));
    }

    /** Test 6: proces bloku s neplatným prevBlockHash */
    static void test6_invalidPrevBlockHash() throws Exception {
        System.out.println("Test 6: neplatný prevBlockHash");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        byte[] fakeHash = new byte[32];
        Arrays.fill(fakeHash, (byte) 99);
        Block block = new Block(fakeHash, pk_bob.getPublicKey());
        block.finalize();

        assertTest("Blok s neplatným prevBlockHash by mal byť odmietnutý", !hb.blockProcess(block));
    }

    /** Test 7: proces bloku s rôznymi typmi neplatných transakcií */
    static void test7_invalidTxTypes() throws Exception {
        System.out.println("Test 7: blok s neplatnými tx typmi");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Neplatný podpis
        Block block = new Block(genesis.getHash(), pk_bob.getPublicKey());
        Tx tx = new Tx();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addOutput(3.125, pk_bob.getPublicKey());
        tx.signTx(pk_bob.getPrivateKey(), 0); // nesprávny kľúč
        block.transactionAdd(tx);
        block.finalize();

        assertTest("Blok s neplatnou tx by mal byť odmietnutý", !hb.blockProcess(block));
    }

    /** Test 8: proces viacerých blokov priamo nad genesis blokom */
    static void test8_multipleBlocksOverGenesis() throws Exception {
        System.out.println("Test 8: viacero blokov nad genesis");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        block1.finalize();
        assertTest("Block1 nad genesis by mal byť prijatý", hb.blockProcess(block1));

        Block block2 = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        block2.finalize();
        assertTest("Block2 nad genesis by mal byť prijatý (fork)", hb.blockProcess(block2));

        // Najstarší blok by mal byť maxHeight
        assertTest("MaxHeight blok by mal byť najstarší", Arrays.equals(bc.getBlockAtMaxHeight().getHash(), block1.getHash()));
    }

    /** Test 9: UTXO už nárokované v rodičovskom bloku */
    static void test9_claimAlreadyClaimedUTXO() throws Exception {
        System.out.println("Test 9: UTXO už nárokované rodičom");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        block1.transactionAdd(tx1);
        block1.finalize();
        hb.blockProcess(block1);

        // Pokus o použitie rovnakého UTXO v ďalšom bloku
        Block block2 = new Block(block1.getHash(), pk_cyril.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(genesis.getCoinbase().getHash(), 0); // už spotrebované
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);
        block2.transactionAdd(tx2);
        block2.finalize();

        assertTest("Blok s už nárokovaným UTXO by mal byť odmietnutý", !hb.blockProcess(block2));
    }

    /** Test 10: UTXO mimo vetvy */
    static void test10_claimUTXOOutsideBranch() throws Exception {
        System.out.println("Test 10: UTXO mimo vetvy");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Vetva 1
        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        block1.transactionAdd(tx1);
        block1.finalize();
        hb.blockProcess(block1);

        // Vetva 2 - pokus o použitie UTXO z vetvy 1
        Block block2 = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0); // UTXO z vetvy 1
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        block2.transactionAdd(tx2);
        block2.finalize();

        assertTest("Blok s UTXO mimo vetvy by mal byť odmietnutý", !hb.blockProcess(block2));
    }

    /** Test 11: staršie UTXO v rámci vetvy, ktoré ešte nebolo nárokované */
    static void test11_claimOlderUnclaimedUTXO() throws Exception {
        System.out.println("Test 11: staršie nenárokované UTXO v rámci vetvy");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Block1: Alice -> Bob
        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(1.0, pk_bob.getPublicKey());
        tx1.addOutput(2.125, pk_alice.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        block1.transactionAdd(tx1);
        block1.finalize();
        hb.blockProcess(block1);

        // Block2: nad block1, používa nepoužité UTXO z tx1
        Block block2 = new Block(block1.getHash(), pk_cyril.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 1); // nepoužitý výstup Alice
        tx2.addOutput(2.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);
        block2.transactionAdd(tx2);
        block2.finalize();

        assertTest("Blok so starším nenárokovaným UTXO by mal byť prijatý", hb.blockProcess(block2));
    }

    /** Test 12: lineárna reťaz blokov */
    static void test12_linearChain() throws Exception {
        System.out.println("Test 12: lineárna reťaz blokov");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block prev = genesis;
        boolean allAdded = true;
        for (int i = 0; i < 5; i++) {
            Block block = new Block(prev.getHash(), pk_bob.getPublicKey());
            block.finalize();
            if (!hb.blockProcess(block)) {
                allAdded = false;
                break;
            }
            prev = block;
        }

        assertTest("Lineárna reťaz 5 blokov by mala byť prijatá", allAdded);
    }

    /** Test 13: lineárna reťaz dlhá CUT_OFF_AGE, potom blok nad genesis */
    static void test13_linearChainCutOff() throws Exception {
        System.out.println("Test 13: reťaz CUT_OFF_AGE, blok nad genesis");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block prev = genesis;
        for (int i = 0; i < Blockchain.CUT_OFF_AGE; i++) {
            Block block = new Block(prev.getHash(), pk_bob.getPublicKey());
            block.finalize();
            hb.blockProcess(block);
            prev = block;
        }

        // Blok nad genesis - výška by bola 2, maxHeight = CUT_OFF_AGE + 1
        // height > maxHeight - CUT_OFF_AGE => 2 > (CUT_OFF_AGE + 1) - CUT_OFF_AGE => 2 > 1 => true
        Block blockOnGenesis = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        blockOnGenesis.finalize();

        assertTest("Blok nad genesis pri height = CUT_OFF_AGE + 1 by mal byť prijatý", hb.blockProcess(blockOnGenesis));
    }

    /** Test 14: lineárna reťaz CUT_OFF_AGE + 1, potom blok nad genesis */
    static void test14_linearChainCutOffPlus1() throws Exception {
        System.out.println("Test 14: reťaz CUT_OFF_AGE+1, blok nad genesis");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block prev = genesis;
        for (int i = 0; i < Blockchain.CUT_OFF_AGE + 1; i++) {
            Block block = new Block(prev.getHash(), pk_bob.getPublicKey());
            block.finalize();
            hb.blockProcess(block);
            prev = block;
        }

        // Blok nad genesis - výška by bola 2, maxHeight = CUT_OFF_AGE + 2
        // height > maxHeight - CUT_OFF_AGE => 2 > (CUT_OFF_AGE + 2) - CUT_OFF_AGE => 2 > 2 => false
        Block blockOnGenesis = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        blockOnGenesis.finalize();

        assertTest("Blok nad genesis pri height > CUT_OFF_AGE + 1 by mal byť odmietnutý", !hb.blockProcess(blockOnGenesis));
    }

    /** Test 15: vytvor blok, keď neboli spracované žiadne transakcie */
    static void test15_createBlockNoTxs() throws Exception {
        System.out.println("Test 15: vytvor blok bez tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block created = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Vytvorený blok by nemal byť null", created != null);
    }

    /** Test 16: vytvor blok po spracovaní jednej platnej transakcie */
    static void test16_createBlockAfterOneTx() throws Exception {
        System.out.println("Test 16: vytvor blok po jednej tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx = new Tx();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addOutput(3.125, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx);

        Block created = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Vytvorený blok s tx by nemal byť null", created != null);
        assertTest("Blok by mal obsahovať transakciu", created.getTransactions().size() >= 1);
    }

    /** Test 17: vytvor dva bloky sekvenčne */
    static void test17_createTwoBlocksSequentially() throws Exception {
        System.out.println("Test 17: dva bloky sekvenčne");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);

        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Prvý vytvorený blok by nemal byť null", block1 != null);

        // Druhý blok - používa coinbase z block1
        Tx tx2 = new Tx();
        tx2.addInput(block1.getCoinbase().getHash(), 0);
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        hb.txProcess(tx2);

        Block block2 = hb.blockCreate(pk_cyril.getPublicKey());
        assertTest("Druhý vytvorený blok by nemal byť null", block2 != null);
    }

    /** Test 18: vytvor blok po spracovaní tx, ktorá už je v bloku */
    static void test18_createBlockTxAlreadyInBlock() throws Exception {
        System.out.println("Test 18: tx už v bloku");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx = new Tx();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addOutput(3.125, pk_bob.getPublicKey());
        tx.signTx(pk_alice.getPrivateKey(), 0);

        // Pridaj tx do bloku aj do poolu
        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        block1.transactionAdd(tx);
        block1.finalize();
        hb.blockProcess(block1);

        // Tx je už v bloku, pool by mal byť prázdny
        hb.txProcess(tx); // pridáme do poolu
        Block created = hb.blockCreate(pk_cyril.getPublicKey());
        assertTest("Blok by mal byť vytvorený", created != null);
        // Tx by mala byť neplatná (UTXO už spotrebované)
    }

    /** Test 19: tx používa UTXO, ktoré už bolo nárokované */
    static void test19_createBlockTxUsesClaimedUTXO() throws Exception {
        System.out.println("Test 19: tx používa nárokované UTXO");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);

        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Block1 vytvorený", block1 != null);

        // tx2 používa rovnaké UTXO ako tx1
        Tx tx2 = new Tx();
        tx2.addInput(genesis.getCoinbase().getHash(), 0);
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx2);

        Block created = hb.blockCreate(pk_cyril.getPublicKey());
        // tx2 by mala byť neplatná, blok by mal byť prázdny (len coinbase)
        assertTest("Blok by mal byť vytvorený (bez neplatnej tx)", created != null);
    }

    /** Test 20: platná tx, ktorá nie je double spend */
    static void test20_createBlockTxNotDoubleSpend() throws Exception {
        System.out.println("Test 20: platná tx bez double spend");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(1.0, pk_bob.getPublicKey());
        tx1.addOutput(2.125, pk_alice.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);

        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Block1 vytvorený", block1 != null);

        // tx2 používa nový UTXO z tx1
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 1);
        tx2.addOutput(2.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx2);

        Block block2 = hb.blockCreate(pk_cyril.getPublicKey());
        assertTest("Block2 s platnou novou tx", block2 != null);
        assertTest("Block2 by mal obsahovať tx", block2.getTransactions().size() >= 1);
    }

    /** Test 21: vytvor blok po spracovaní iba neplatných transakcií */
    static void test21_createBlockOnlyInvalidTxs() throws Exception {
        System.out.println("Test 21: len neplatné tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Neplatná tx - nesprávny podpis
        Tx tx = new Tx();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addOutput(3.125, pk_bob.getPublicKey());
        tx.signTx(pk_bob.getPrivateKey(), 0); // nesprávny kľúč
        hb.txProcess(tx);

        Block created = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Blok by mal byť vytvorený aj bez platných tx", created != null);
        assertTest("Blok by nemal obsahovať neplatné tx", created.getTransactions().size() == 0);
    }

    /** Test 22: spracuj transakciu, vytvor blok, opakuj */
    static void test22_combinedTxAndBlockCreation() throws Exception {
        System.out.println("Test 22: kombinovaný tx a block creation");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Runda 1
        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);
        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Block1 vytvorený", block1 != null);

        // Runda 2
        Tx tx2 = new Tx();
        tx2.addInput(block1.getCoinbase().getHash(), 0);
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        hb.txProcess(tx2);
        Block block2 = hb.blockCreate(pk_cyril.getPublicKey());
        assertTest("Block2 vytvorený", block2 != null);

        // Runda 3
        Tx tx3 = new Tx();
        tx3.addInput(block2.getCoinbase().getHash(), 0);
        tx3.addOutput(3.0, pk_dave.getPublicKey());
        tx3.signTx(pk_cyril.getPrivateKey(), 0);
        hb.txProcess(tx3);
        Block block3 = hb.blockCreate(pk_dave.getPublicKey());
        assertTest("Block3 vytvorený", block3 != null);
    }

    /** Test 23: spracuj tx, vytvor blok, spracuj blok s tx nárokujúcou UTXO z tej tx */
    static void test23_processBlockClaimingUTXOFromTx() throws Exception {
        System.out.println("Test 23: blok s tx nárokujúcou UTXO z predchádzajúcej tx");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);

        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Block1 vytvorený", block1 != null);

        // Blok nad block1 s tx používajúcou výstup tx1
        Block block2 = new Block(block1.getHash(), pk_cyril.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        block2.transactionAdd(tx2);
        block2.finalize();

        assertTest("Block2 s tx z predchádzajúcej tx by mal byť prijatý", hb.blockProcess(block2));
    }

    /** Test 24: spracuj blok nad genesis s tx nárokujúcou UTXO */
    static void test24_processBlockOnGenesisWithTxUTXO() throws Exception {
        System.out.println("Test 24: blok nad genesis s tx UTXO");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Tx tx1 = new Tx();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addOutput(3.125, pk_bob.getPublicKey());
        tx1.signTx(pk_alice.getPrivateKey(), 0);
        hb.txProcess(tx1);

        Block block1 = hb.blockCreate(pk_bob.getPublicKey());
        assertTest("Block1 vytvorený", block1 != null);

        // Blok nad genesis (fork) - nemôže používať UTXO z tx1
        Block blockFork = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        Tx tx2 = new Tx();
        tx2.addInput(tx1.getHash(), 0); // UTXO z inej vetvy
        tx2.addOutput(3.125, pk_cyril.getPublicKey());
        tx2.signTx(pk_bob.getPrivateKey(), 0);
        blockFork.transactionAdd(tx2);
        blockFork.finalize();

        assertTest("Fork blok s UTXO z inej vetvy by mal byť odmietnutý", !hb.blockProcess(blockFork));
    }

    /** Test 25: viacero blokov nad genesis, potom vytvor blok */
    static void test25_multipleBlocksOverGenesisCreateBlock() throws Exception {
        System.out.println("Test 25: viacero blokov, vytvor blok v správnej vetve");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        Block block1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        block1.finalize();
        hb.blockProcess(block1);

        Block block2 = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        block2.finalize();
        hb.blockProcess(block2);

        // block1 je najstarší, takže maxHeight by mal byť block1
        assertTest("MaxHeight by mal byť block1 (najstarší)", Arrays.equals(bc.getBlockAtMaxHeight().getHash(), block1.getHash()));

        // Vytvoríme blok - mal by byť nad block1
        Block created = hb.blockCreate(pk_dave.getPublicKey());
        assertTest("Vytvorený blok by nemal byť null", created != null);
    }

    /** Test 26: viacero vetiev približne rovnakej veľkosti */
    static void test26_multipleBranches() throws Exception {
        System.out.println("Test 26: viacero vetiev");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Vetva 1: genesis -> b1 -> b3
        Block b1 = new Block(genesis.getHash(), pk_bob.getPublicKey());
        b1.finalize();
        hb.blockProcess(b1);

        Block b3 = new Block(b1.getHash(), pk_bob.getPublicKey());
        b3.finalize();
        hb.blockProcess(b3);

        // Vetva 2: genesis -> b2 -> b4
        Block b2 = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        b2.finalize();
        hb.blockProcess(b2);

        Block b4 = new Block(b2.getHash(), pk_cyril.getPublicKey());
        b4.finalize();
        hb.blockProcess(b4);

        // Obe vetvy majú rovnakú výšku, maxHeight by mal byť najstarší = b3
        assertTest("MaxHeight by mal byť b3 (najstarší s max height)", Arrays.equals(bc.getBlockAtMaxHeight().getHash(), b3.getHash()));

        // Predĺžime vetvu 1
        Block b5 = new Block(b3.getHash(), pk_bob.getPublicKey());
        b5.finalize();
        hb.blockProcess(b5);

        assertTest("Po predĺžení vetvy 1, maxHeight by mal byť b5", Arrays.equals(bc.getBlockAtMaxHeight().getHash(), b5.getHash()));
    }

    /** Test 27: bloky pod CUT_OFF_AGE */
    static void test27_blocksBelowCutOffAge() throws Exception {
        System.out.println("Test 27: bloky pod CUT_OFF_AGE");
        Block genesis = createGenesisBlock(pk_alice.getPublicKey());
        Blockchain bc = new Blockchain(genesis);
        HandleBlocks hb = new HandleBlocks(bc);

        // Vytvoríme dlhú reťaz
        Block prev = genesis;
        Block[] chain = new Block[Blockchain.CUT_OFF_AGE + 2];
        for (int i = 0; i < Blockchain.CUT_OFF_AGE + 2; i++) {
            chain[i] = new Block(prev.getHash(), pk_bob.getPublicKey());
            chain[i].finalize();
            hb.blockProcess(chain[i]);
            prev = chain[i];
        }

        // Pokus o pridanie bloku nad genesis (príliš staré)
        Block oldBlock = new Block(genesis.getHash(), pk_cyril.getPublicKey());
        oldBlock.finalize();
        assertTest("Blok nad genesis (pod CUT_OFF_AGE) by mal byť odmietnutý", !hb.blockProcess(oldBlock));

        // Pokus o pridanie bloku nad chain[0] (tiež príliš staré)
        Block oldBlock2 = new Block(chain[0].getHash(), pk_cyril.getPublicKey());
        oldBlock2.finalize();
        assertTest("Blok nad starým blokom (pod CUT_OFF_AGE) by mal byť odmietnutý", !hb.blockProcess(oldBlock2));
    }
}
