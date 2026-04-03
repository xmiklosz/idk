// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - MultiSigTransaction pre bonus fázy 3.

import java.util.ArrayList;
import java.util.HashMap;

/**
 * Rozšírenie transakcie s podporou multisig vstupov a výstupov.
 * Umožňuje viaceré podpisy na jednom vstupe a sleduje, ktoré výstupy sú multisig.
 */
public class MultiSigTransaction extends Transaction {

    /** Viaceré podpisy pre každý vstup (index vstupu -> zoznam podpisov) */
    private HashMap<Integer, ArrayList<byte[]>> multiSignatures;

    /** Multisig info pre výstupy (index výstupu -> MultiSig objekt) */
    private HashMap<Integer, MultiSig> multiSigOutputs;

    public MultiSigTransaction() {
        super();
        multiSignatures = new HashMap<Integer, ArrayList<byte[]>>();
        multiSigOutputs = new HashMap<Integer, MultiSig>();
    }

    /**
     * Pridá multisig výstup - hodnota sa pošle na multisig peňaženku.
     * Primárna adresa multisigu sa použije ako adresa v Transaction.Output.
     */
    public void addMultiSigOutput(double value, MultiSig multiSig) {
        super.addOutput(value, multiSig.getPrimaryAddress());
        int index = numOutputs() - 1;
        multiSigOutputs.put(index, multiSig);
    }

    /**
     * Pridá podpis pre vstup od jedného účastníka multisigu.
     * Viacerí účastníci volajú túto metódu pre rovnaký vstup.
     */
    public void addMultiSigSignature(byte[] signature, int inputIndex) {
        if (!multiSignatures.containsKey(inputIndex)) {
            multiSignatures.put(inputIndex, new ArrayList<byte[]>());
        }
        multiSignatures.get(inputIndex).add(
            signature != null ? signature.clone() : null);

        // Prvý podpis sa nastaví aj ako klasický podpis (pre kompatibilitu s finalize)
        if (multiSignatures.get(inputIndex).size() == 1) {
            super.addSignature(signature, inputIndex);
        }
    }

    /**
     * Vráti všetky podpisy pre daný vstup.
     */
    public ArrayList<byte[]> getMultiSignatures(int inputIndex) {
        if (multiSignatures.containsKey(inputIndex)) {
            return multiSignatures.get(inputIndex);
        }
        return new ArrayList<byte[]>();
    }

    /**
     * Vráti true ak daný výstup je multisig.
     */
    public boolean isMultiSigOutput(int outputIndex) {
        return multiSigOutputs.containsKey(outputIndex);
    }

    /**
     * Vráti MultiSig objekt pre daný výstup, alebo null.
     */
    public MultiSig getMultiSig(int outputIndex) {
        return multiSigOutputs.get(outputIndex);
    }

    /**
     * Vráti mapu všetkých multisig výstupov.
     */
    public HashMap<Integer, MultiSig> getMultiSigOutputs() {
        return new HashMap<Integer, MultiSig>(multiSigOutputs);
    }

    /**
     * Vráti true ak tento vstup má multisig podpisy.
     */
    public boolean hasMultiSigInput(int inputIndex) {
        return multiSignatures.containsKey(inputIndex)
            && multiSignatures.get(inputIndex).size() > 0;
    }
}
