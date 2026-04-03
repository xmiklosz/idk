// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - HandleMultiSigTxs pre bonus fázy 3.

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

/**
 * Rozšírenie HandleTxs s podporou multisig transakcií.
 * Spravuje registráciu multisig UTXO a validáciu M-of-N podpisov.
 */
public class HandleMultiSigTxs extends HandleTxs {

    /** Registrácia multisig informácií pre UTXO (UTXO -> MultiSig) */
    private HashMap<UTXO, MultiSig> multiSigRegistry;

    public HandleMultiSigTxs(UTXOPool utxoPool) {
        super(utxoPool);
        multiSigRegistry = new HashMap<UTXO, MultiSig>();
    }

    /**
     * Zaregistruje UTXO ako multisig výstup.
     */
    public void registerMultiSig(UTXO utxo, MultiSig multiSig) {
        multiSigRegistry.put(utxo, multiSig);
    }

    /**
     * Vráti MultiSig info pre dané UTXO, alebo null ak nie je multisig.
     */
    public MultiSig getMultiSig(UTXO utxo) {
        return multiSigRegistry.get(utxo);
    }

    /**
     * Overí platnosť transakcie vrátane multisig podpisov.
     */
    @Override
    public boolean txIsValid(Transaction tx) {
        UTXOPool pool = UTXOPoolGet();
        HashSet<UTXO> claimedUTXOs = new HashSet<UTXO>();
        double inputSum = 0;
        double outputSum = 0;

        for (int i = 0; i < tx.numInputs(); i++) {
            Transaction.Input input = tx.getInput(i);
            UTXO utxo = new UTXO(input.prevTxHash, input.outputIndex);

            // (1) UTXO musí existovať v poole
            if (!pool.contains(utxo)) {
                return false;
            }

            Transaction.Output correspondingOutput = pool.getTxOutput(utxo);
            byte[] message = tx.getDataToSign(i);

            // (2) Overenie podpisov - multisig alebo klasický
            MultiSig multiSig = multiSigRegistry.get(utxo);
            if (multiSig != null) {
                // Multisig overenie: potrebujeme M-of-N platných podpisov
                if (!(tx instanceof MultiSigTransaction)) {
                    return false;
                }
                MultiSigTransaction msTx = (MultiSigTransaction) tx;
                ArrayList<byte[]> signatures = msTx.getMultiSignatures(i);
                if (!multiSig.verify(message, signatures)) {
                    return false;
                }
            } else {
                // Klasické overenie jedným podpisom
                RSAKey publicKey = correspondingOutput.address;
                byte[] signature = input.signature;
                if (signature == null || !publicKey.verifySignature(message, signature)) {
                    return false;
                }
            }

            // (3) Žiadne UTXO nie je nárokované viackrát
            if (claimedUTXOs.contains(utxo)) {
                return false;
            }
            claimedUTXOs.add(utxo);

            inputSum += correspondingOutput.value;
        }

        // (4) Výstupné hodnoty musia byť nezáporné
        for (int i = 0; i < tx.numOutputs(); i++) {
            Transaction.Output output = tx.getOutput(i);
            if (output.value < 0) {
                return false;
            }
            outputSum += output.value;
        }

        // (5) Súčet vstupov >= súčet výstupov
        if (inputSum < outputSum) {
            return false;
        }

        return true;
    }

    /**
     * Spracovanie transakcií s podporou multisig.
     * Registruje nové multisig výstupy do registra.
     */
    @Override
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

                    // Odstráň spotrebované UTXO a ich multisig registrácie
                    for (int j = 0; j < tx.numInputs(); j++) {
                        Transaction.Input input = tx.getInput(j);
                        UTXO utxo = new UTXO(input.prevTxHash, input.outputIndex);
                        UTXOPoolGet().removeUTXO(utxo);
                        multiSigRegistry.remove(utxo);
                    }

                    // Pridaj nové UTXO
                    byte[] txHash = tx.getHash();
                    for (int j = 0; j < tx.numOutputs(); j++) {
                        UTXO utxo = new UTXO(txHash, j);
                        UTXOPoolGet().addUTXO(utxo, tx.getOutput(j));

                        // Registruj multisig ak je to MultiSigTransaction
                        if (tx instanceof MultiSigTransaction) {
                            MultiSigTransaction msTx = (MultiSigTransaction) tx;
                            if (msTx.isMultiSigOutput(j)) {
                                multiSigRegistry.put(utxo, msTx.getMultiSig(j));
                            }
                        }
                    }

                    possibleTxs[i] = null;
                    changed = true;
                }
            }
        }

        return acceptedTxs.toArray(new Transaction[acceptedTxs.size()]);
    }
}
