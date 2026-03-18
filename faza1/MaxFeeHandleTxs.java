// Meno študenta:
// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - kompletná implementácia
// MaxFeeHandleTxs triedy s greedy-by-fee heuristikou pre maximalizáciu transakčných poplatkov.

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;

public class MaxFeeHandleTxs {

    private UTXOPool utxoPool;

    /**
     * Vytvorí verejný ledger, ktorého aktuálny UTXOPool je {@code utxoPool}.
     * Vytvorí bezpečnú kópiu utxoPool.
     */
    public MaxFeeHandleTxs(UTXOPool utxoPool) {
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
     * @return true, ak sú splnené všetky podmienky platnosti transakcie.
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
     * Vypočíta poplatok (fee) transakcie, čo je rozdiel medzi
     * súčtom vstupných a výstupných hodnôt.
     *
     * @return poplatok transakcie, alebo -1 ak transakcia nie je platná
     */
    private double calculateFee(Transaction tx) {
        double inputSum = 0;
        double outputSum = 0;

        for (int i = 0; i < tx.numInputs(); i++) {
            Transaction.Input input = tx.getInput(i);
            UTXO utxo = new UTXO(input.prevTxHash, input.outputIndex);
            if (!utxoPool.contains(utxo)) {
                return -1;
            }
            Transaction.Output correspondingOutput = utxoPool.getTxOutput(utxo);
            inputSum += correspondingOutput.value;
        }

        for (int i = 0; i < tx.numOutputs(); i++) {
            outputSum += tx.getOutput(i).value;
        }

        return inputSum - outputSum;
    }

    /**
     * Spracováva každú epochu s maximalizáciou celkových transakčných poplatkov.
     * Používa greedy-by-fee heuristiku: v každej iterácii vyberá platnú transakciu
     * s najvyšším poplatkom.
     */
    public Transaction[] handler(Transaction[] possibleTxs) {
        ArrayList<Transaction> acceptedTxs = new ArrayList<Transaction>();

        // Vytvoríme zoznam kandidátov
        ArrayList<Transaction> candidates = new ArrayList<Transaction>();
        for (Transaction tx : possibleTxs) {
            if (tx != null) {
                candidates.add(tx);
            }
        }

        // Iteratívny greedy prístup: v každom kole zoradíme podľa poplatku
        // a vyberieme najlepšiu platnú transakciu. Opakujeme, kým existujú platné.
        boolean changed = true;
        while (changed) {
            changed = false;

            // Zoradíme kandidátov podľa poplatku (zostupne)
            ArrayList<TransactionWithFee> validWithFees = new ArrayList<TransactionWithFee>();
            for (Transaction tx : candidates) {
                if (txIsValid(tx)) {
                    double fee = calculateFee(tx);
                    if (fee >= 0) {
                        validWithFees.add(new TransactionWithFee(tx, fee));
                    }
                }
            }

            // Zoradíme zostupne podľa poplatku
            Collections.sort(validWithFees, new Comparator<TransactionWithFee>() {
                public int compare(TransactionWithFee a, TransactionWithFee b) {
                    return Double.compare(b.fee, a.fee);
                }
            });

            // Vyberieme najlepšiu platnú transakciu
            for (TransactionWithFee twf : validWithFees) {
                Transaction tx = twf.tx;

                // Skontrolujeme ešte raz platnosť (mohla sa zmeniť po predchádzajúcej iterácii)
                if (txIsValid(tx)) {
                    acceptedTxs.add(tx);
                    candidates.remove(tx);

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

                    changed = true;
                    break; // Reštartujeme iteráciu s aktualizovaným poolom
                }
            }
        }

        return acceptedTxs.toArray(new Transaction[acceptedTxs.size()]);
    }

    /**
     * Pomocná trieda na ukladanie transakcie spolu s jej poplatkom.
     */
    private class TransactionWithFee {
        Transaction tx;
        double fee;

        TransactionWithFee(Transaction tx, double fee) {
            this.tx = tx;
            this.fee = fee;
        }
    }
}
