// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - MultiSig trieda pre bonus fázy 3.

import java.util.ArrayList;
import java.util.HashSet;

/**
 * Reprezentuje M-of-N multisig peňaženku.
 * Vyžaduje minimálne {@code requiredSigs} podpisov z {@code publicKeys} na autorizáciu transakcie.
 */
public class MultiSig {

    private int requiredSigs;
    private ArrayList<RSAKey> publicKeys;

    /**
     * Vytvorí multisig peňaženku s prahom M a zoznamom N verejných kľúčov.
     * @param requiredSigs minimálny počet podpisov (M)
     * @param publicKeys zoznam verejných kľúčov účastníkov (N)
     */
    public MultiSig(int requiredSigs, ArrayList<RSAKey> publicKeys) {
        if (requiredSigs <= 0 || requiredSigs > publicKeys.size()) {
            throw new IllegalArgumentException(
                "requiredSigs musí byť medzi 1 a " + publicKeys.size());
        }
        this.requiredSigs = requiredSigs;
        this.publicKeys = new ArrayList<RSAKey>(publicKeys);
    }

    public int getRequiredSigs() {
        return requiredSigs;
    }

    public ArrayList<RSAKey> getPublicKeys() {
        return new ArrayList<RSAKey>(publicKeys);
    }

    public int getNumKeys() {
        return publicKeys.size();
    }

    public RSAKey getKey(int index) {
        return publicKeys.get(index);
    }

    /**
     * Overí, či poskytnuté podpisy spĺňajú M-of-N požiadavku.
     * Každý podpis sa priradí ku kľúču, ktorý ho dokáže overiť.
     * Jeden kľúč môže byť použitý iba raz.
     *
     * @param message správa, ktorá bola podpísaná
     * @param signatures zoznam podpisov na overenie
     * @return true ak aspoň M podpisov je platných rôznymi kľúčmi
     */
    public boolean verify(byte[] message, ArrayList<byte[]> signatures) {
        if (signatures == null || signatures.size() < requiredSigs) {
            return false;
        }

        HashSet<Integer> usedKeys = new HashSet<Integer>();
        int validCount = 0;

        for (byte[] sig : signatures) {
            if (sig == null) continue;
            for (int k = 0; k < publicKeys.size(); k++) {
                if (usedKeys.contains(k)) continue;
                if (publicKeys.get(k).verifySignature(message, sig)) {
                    usedKeys.add(k);
                    validCount++;
                    break;
                }
            }
        }

        return validCount >= requiredSigs;
    }

    /**
     * Vráti prvý verejný kľúč ako "primárnu adresu" multisigu.
     * Používa sa pre kompatibilitu s Transaction.Output.
     */
    public RSAKey getPrimaryAddress() {
        return publicKeys.get(0);
    }
}
