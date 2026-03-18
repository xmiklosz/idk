import java.util.ArrayList;
import java.util.HashMap;
import java.util.Set;

public class UTXOPool {

    private HashMap<UTXO, Transaction.Output> H;

    public UTXOPool() {
        H = new HashMap<UTXO, Transaction.Output>();
    }

    public UTXOPool(UTXOPool uPool) {
        H = new HashMap<UTXO, Transaction.Output>(uPool.H);
    }

    public void addUTXO(UTXO utxo, Transaction.Output txOut) {
        H.put(utxo, txOut);
    }

    public void removeUTXO(UTXO utxo) {
        H.remove(utxo);
    }

    public Transaction.Output getTxOutput(UTXO ut) {
        return H.get(ut);
    }

    public boolean contains(UTXO utxo) {
        return H.containsKey(utxo);
    }

    public ArrayList<UTXO> getAllUTXO() {
        Set<UTXO> setUTXO = H.keySet();
        ArrayList<UTXO> allUTXO = new ArrayList<UTXO>();
        for (UTXO ut : setUTXO) {
            allUTXO.add(ut);
        }
        return allUTXO;
    }
}
