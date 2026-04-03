// AI: Unit testy vytvorené s pomocou Claude AI (Anthropic) pre MultiSig bonus (Fáza 3).

import java.util.ArrayList;

/**
 * 9 unit testov pre MultiSig transakcie.
 * Každý test za 0,33 bodu, spolu max 3 body.
 */
public class MultiSigTest {

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
            key_alice[i] = (byte) 10;
            key_bob[i] = (byte) 11;
            key_cyril[i] = (byte) 12;
            key_dave[i] = (byte) 13;
        }
        pk_alice = new RSAKeyPair(new PRGen(key_alice), 265);
        pk_bob = new RSAKeyPair(new PRGen(key_bob), 265);
        pk_cyril = new RSAKeyPair(new PRGen(key_cyril), 265);
        pk_dave = new RSAKeyPair(new PRGen(key_dave), 265);

        System.out.println("=== MultiSig Tests (Bonus Fáza 3) ===");

        test1_createMultiSigWallet();
        test2_signWithOneParticipant();
        test3_signWithMinimumParticipants();
        test4_signWithAllParticipants();
        test5_invalidSignature();
        test6_sendToMultiSig();
        test7_spendFromMultiSig();
        test8_doubleSpendMultiSig();
        test9_chainMultiSigTransactions();

        System.out.println("\n=== Výsledky: " + passed + " PASSED, " + failed + " FAILED z 9 ===");
    }

    static void check(String name, boolean condition) {
        if (condition) {
            System.out.println("  PASS: " + name);
            passed++;
        } else {
            System.out.println("  FAIL: " + name);
            failed++;
        }
    }

    /**
     * Test 1: Vytvor multisig peňaženku (2-of-3).
     */
    static void test1_createMultiSigWallet() {
        System.out.println("\nTest 1: Vytvorenie multisig peňaženky");
        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());

        MultiSig wallet = new MultiSig(2, keys);

        check("2-of-3 multisig: requiredSigs == 2",
            wallet.getRequiredSigs() == 2);
        check("2-of-3 multisig: numKeys == 3",
            wallet.getNumKeys() == 3);
        check("2-of-3 multisig: primaryAddress je Alice",
            wallet.getPrimaryAddress().getExponent().equals(
                pk_alice.getPublicKey().getExponent()));

        // Overenie neplatných parametrov
        boolean thrown = false;
        try {
            new MultiSig(0, keys);
        } catch (IllegalArgumentException e) {
            thrown = true;
        }
        check("requiredSigs=0 vyhodí výnimku", thrown);

        thrown = false;
        try {
            new MultiSig(4, keys);
        } catch (IllegalArgumentException e) {
            thrown = true;
        }
        check("requiredSigs>N vyhodí výnimku", thrown);
    }

    /**
     * Test 2: Vytvor transakciu a podpíš ju iba jedným účastníkom (pre 2-of-3 musí zlyhať).
     */
    static void test2_signWithOneParticipant() throws Exception {
        System.out.println("\nTest 2: Podpis iba jedným účastníkom (2-of-3)");

        // Vytvor genesis blok s coinbase pre Alice
        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        // Alice pošle coiny na 2-of-3 multisig (Alice, Bob, Cyril)
        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(2, keys);

        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        // Spracuj tx1 - pošle na multisig
        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        Transaction[] result = handler.handler(new Transaction[]{tx1});
        check("tx1 (odoslanie na multisig) je platná", result.length == 1);

        // Teraz skús minúť z multisigu len s jedným podpisom (Alice)
        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.5, pk_dave.getPublicKey());
        byte[] sigAlice = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigAlice, 0);
        tx2.finalize();

        // Toto by malo zlyhať - len 1 podpis z 2 potrebných
        check("Len 1 podpis z 2-of-3 je neplatný",
            !handler.txIsValid(tx2));
    }

    /**
     * Test 3: Podpíš minimálnym počtom účastníkov (2 z 3).
     */
    static void test3_signWithMinimumParticipants() throws Exception {
        System.out.println("\nTest 3: Podpis minimálnym počtom účastníkov (2-of-3)");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(2, keys);

        // Odošli na multisig
        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        // Registruj multisig pre nový UTXO
        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        // Miň z multisigu s 2 podpismi (Alice + Bob)
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.5, pk_dave.getPublicKey());
        byte[] sigAlice = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        byte[] sigBob = pk_bob.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigAlice, 0);
        tx2.addMultiSigSignature(sigBob, 0);
        tx2.finalize();

        check("2 podpisy z 2-of-3 sú platné",
            handler.txIsValid(tx2));
    }

    /**
     * Test 4: Podpíš všetkými účastníkmi (3 z 3).
     */
    static void test4_signWithAllParticipants() throws Exception {
        System.out.println("\nTest 4: Podpis všetkými účastníkmi (3-of-3)");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        // 3-of-3 multisig
        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(3, keys);

        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        // Všetci 3 podpíšu
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.9, pk_dave.getPublicKey());
        byte[] sigA = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        byte[] sigB = pk_bob.getPrivateKey().sign(tx2.getDataToSign(0));
        byte[] sigC = pk_cyril.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigA, 0);
        tx2.addMultiSigSignature(sigB, 0);
        tx2.addMultiSigSignature(sigC, 0);
        tx2.finalize();

        check("3 podpisy z 3-of-3 sú platné",
            handler.txIsValid(tx2));
    }

    /**
     * Test 5: Neplatný podpis v multisig transakcii.
     */
    static void test5_invalidSignature() throws Exception {
        System.out.println("\nTest 5: Neplatný podpis v multisig");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(2, keys);

        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        // Dave nie je účastník multisigu - jeho podpis je neplatný
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.5, pk_dave.getPublicKey());
        byte[] sigAlice = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        byte[] sigDave = pk_dave.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigAlice, 0);
        tx2.addMultiSigSignature(sigDave, 0);
        tx2.finalize();

        check("Podpis neúčastníkom (Dave) je neplatný",
            !handler.txIsValid(tx2));
    }

    /**
     * Test 6: Odošli coiny na multisig adresu (klasická tx -> multisig výstup).
     */
    static void test6_sendToMultiSig() throws Exception {
        System.out.println("\nTest 6: Odoslanie coinov na multisig adresu");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(1, keys);

        // Alice posiela na 1-of-2 multisig
        MultiSigTransaction tx = new MultiSigTransaction();
        tx.addInput(genesis.getCoinbase().getHash(), 0);
        tx.addMultiSigOutput(2.0, wallet);
        tx.addOutput(1.0, pk_alice.getPublicKey()); // zvyšok späť Alice
        byte[] sig = pk_alice.getPrivateKey().sign(tx.getDataToSign(0));
        tx.addSignature(sig, 0);
        tx.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        Transaction[] result = handler.handler(new Transaction[]{tx});

        check("Odoslanie na multisig je platné", result.length == 1);
    }

    /**
     * Test 7: Miň z multisig peňaženky (1-of-2 - stačí jeden podpis).
     */
    static void test7_spendFromMultiSig() throws Exception {
        System.out.println("\nTest 7: Míňanie z multisig peňaženky (1-of-2)");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_bob.getPublicKey());
        keys.add(pk_cyril.getPublicKey());
        MultiSig wallet = new MultiSig(1, keys);

        // Odošli na multisig
        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        // Bob sám minie z 1-of-2
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.5, pk_dave.getPublicKey());
        byte[] sigBob = pk_bob.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigBob, 0);
        tx2.finalize();

        Transaction[] result = handler.handler(new Transaction[]{tx2});
        check("1-of-2 multisig: Bob sám môže minúť", result.length == 1);
    }

    /**
     * Test 8: Double-spend z multisig peňaženky.
     */
    static void test8_doubleSpendMultiSig() throws Exception {
        System.out.println("\nTest 8: Double-spend z multisig");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        ArrayList<RSAKey> keys = new ArrayList<RSAKey>();
        keys.add(pk_alice.getPublicKey());
        keys.add(pk_bob.getPublicKey());
        MultiSig wallet = new MultiSig(1, keys);

        // Odošli na multisig
        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        UTXO msUtxo = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo, wallet);

        // Prvá transakcia z multisigu
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addOutput(2.0, pk_cyril.getPublicKey());
        byte[] sigA = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigA, 0);
        tx2.finalize();

        // Double-spend - rovnaký vstup
        MultiSigTransaction tx3 = new MultiSigTransaction();
        tx3.addInput(tx1.getHash(), 0);
        tx3.addOutput(2.0, pk_dave.getPublicKey());
        byte[] sigB = pk_bob.getPrivateKey().sign(tx3.getDataToSign(0));
        tx3.addMultiSigSignature(sigB, 0);
        tx3.finalize();

        Transaction[] result = handler.handler(new Transaction[]{tx2, tx3});
        check("Double-spend z multisig: len 1 transakcia prijatá",
            result.length == 1);
    }

    /**
     * Test 9: Reťazenie multisig transakcií (multisig -> multisig).
     */
    static void test9_chainMultiSigTransactions() throws Exception {
        System.out.println("\nTest 9: Reťazenie multisig transakcií");

        Block genesis = new Block(null, pk_alice.getPublicKey());
        genesis.finalize();

        UTXOPool pool = new UTXOPool();
        UTXO coinbaseUtxo = new UTXO(genesis.getCoinbase().getHash(), 0);
        pool.addUTXO(coinbaseUtxo, genesis.getCoinbase().getOutput(0));

        // Prvý multisig: 2-of-2 (Alice, Bob)
        ArrayList<RSAKey> keys1 = new ArrayList<RSAKey>();
        keys1.add(pk_alice.getPublicKey());
        keys1.add(pk_bob.getPublicKey());
        MultiSig wallet1 = new MultiSig(2, keys1);

        // Odošli na prvý multisig
        MultiSigTransaction tx1 = new MultiSigTransaction();
        tx1.addInput(genesis.getCoinbase().getHash(), 0);
        tx1.addMultiSigOutput(3.0, wallet1);
        byte[] sig = pk_alice.getPrivateKey().sign(tx1.getDataToSign(0));
        tx1.addSignature(sig, 0);
        tx1.finalize();

        HandleMultiSigTxs handler = new HandleMultiSigTxs(pool);
        handler.handler(new Transaction[]{tx1});

        UTXO msUtxo1 = new UTXO(tx1.getHash(), 0);
        handler.registerMultiSig(msUtxo1, wallet1);

        // Druhý multisig: 1-of-2 (Cyril, Dave)
        ArrayList<RSAKey> keys2 = new ArrayList<RSAKey>();
        keys2.add(pk_cyril.getPublicKey());
        keys2.add(pk_dave.getPublicKey());
        MultiSig wallet2 = new MultiSig(1, keys2);

        // Presun z prvého multisigu na druhý
        MultiSigTransaction tx2 = new MultiSigTransaction();
        tx2.addInput(tx1.getHash(), 0);
        tx2.addMultiSigOutput(2.5, wallet2);
        byte[] sigA = pk_alice.getPrivateKey().sign(tx2.getDataToSign(0));
        byte[] sigB = pk_bob.getPrivateKey().sign(tx2.getDataToSign(0));
        tx2.addMultiSigSignature(sigA, 0);
        tx2.addMultiSigSignature(sigB, 0);
        tx2.finalize();

        Transaction[] result = handler.handler(new Transaction[]{tx2});
        check("Presun z 2-of-2 na 1-of-2 multisig je platný",
            result.length == 1);
    }
}
