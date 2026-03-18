// Meno študenta:
// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - kompletná implementácia
// HandleTxs triedy pre Fázu 3 blockchainu.

import java.util.ArrayList;
import java.util.HashSet;

public class HandleTxs {

    private UTXOPool utxoPool;

    /**
     * Vytvorí verejný ledger, ktorého aktuálny UTXOPool je {@code utxoPool}.
     * Vytvorí bezpečnú kópiu utxoPool.
     */
    public HandleTxs(UTXOPool utxoPool) {
        this.utxoPool = new UTXOPool(utxoPool);
    }

    /**
     * @return aktuálny UTXO pool.
     * Ak nenájde žiadny aktuálny UTXO pool, tak vráti prázdny (nie nulový) objekt UTXOPool.
     */
    public UTXOPool UTXOPoolGet() {
        if (utxoPool == null) {
            return new UTXOPool();
        }
        return utxoPool;
    }

    /**
     * @return true ak sú splnené všetky podmienky platnosti transakcie.
     */
    public boolean txIsValid(Transaction tx) {
        HashSet<UTXO> claimedUTXOs = new HashSet<UTXO>();
        double inputSum = 0;
        double outputSum = 0;

        for (int i = 0; i < tx.numInputs(); i++) {
            Transaction.Input input = tx.getInput(i);
            UTXO utxo = new UTXO(input.prevTxHash, input.outputIndex);

            // (1) všetky výstupy nárokované tx sú v aktuálnom UTXO pool
            if (!utxoPool.contains(utxo)) {
                return false;
            }

            Transaction.Output correspondingOutput = utxoPool.getTxOutput(utxo);

            // (2) podpisy na každom vstupe tx sú platné
            RSAKey publicKey = correspondingOutput.address;
            byte[] message = tx.getDataToSign(i);
            byte[] signature = input.signature;
            if (signature == null || !publicKey.verifySignature(message, signature)) {
                return false;
            }

            // (3) žiadne UTXO nie je nárokované viackrát
            if (claimedUTXOs.contains(utxo)) {
                return false;
            }
            claimedUTXOs.add(utxo);

            inputSum += correspondingOutput.value;
        }

        // (4) všetky výstupné hodnoty tx sú nezáporné
        for (int i = 0; i < tx.numOutputs(); i++) {
            Transaction.Output output = tx.getOutput(i);
            if (output.value < 0) {
                return false;
            }
            outputSum += output.value;
        }

        // (5) súčet vstupných hodnôt tx >= súčet výstupných hodnôt
        if (inputSum < outputSum) {
            return false;
        }

        return true;
    }

    /**
     * Spracováva každú epochu prijímaním neusporiadaného radu navrhovaných transakcií.
     * Používa iteratívny greedy prístup na spracovanie závislých transakcií.
     */
    public Transaction[] handler(Transaction[] possibleTxs) {
        ArrayList<Transaction> acceptedTxs = new ArrayList<Transaction>();

        boolean changed = true;
        while (changed) {
            changed = false;
            for (int i = 0; i < possibleTxs.length; i++) {
                Transaction tx = possibleTxs[i];
                if (tx == null) continue;

                if (txIsValid(tx)) {
                    acceptedTxs.add(tx);

                    // Odstráň spotrebované UTXO
                    for (int j = 0; j < tx.numInputs(); j++) {
                        Transaction.Input input = tx.getInput(j);
                        UTXO utxo = new UTXO(input.prevTxHash, input.outputIndex);
                        utxoPool.removeUTXO(utxo);
                    }

                    // Pridaj nové UTXO
                    byte[] txHash = tx.getHash();
                    for (int j = 0; j < tx.numOutputs(); j++) {
                        UTXO utxo = new UTXO(txHash, j);
                        utxoPool.addUTXO(utxo, tx.getOutput(j));
                    }

                    possibleTxs[i] = null;
                    changed = true;
                }
            }
        }

        return acceptedTxs.toArray(new Transaction[acceptedTxs.size()]);
    }
}
