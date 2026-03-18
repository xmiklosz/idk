// Meno študenta:
// AI: Implementácia vytvorená s pomocou Claude AI (Anthropic) - kompletná implementácia
// Blockchain triedy s podporou forkov (stromová štruktúra), per-block UTXOPool,
// globálny TransactionPool a CUT_OFF_AGE limit.

// Blockchain by mal na naplnenie funkcií udržiavať iba obmedzené množstvo uzlov
// Nemali by ste mať všetky bloky pridané do blockchainu v pamäti
// pretože by to mohlo spôsobiť pretečenie pamäte.
import java.util.ArrayList;
import java.util.HashMap;

public class Blockchain {
    public static final int CUT_OFF_AGE = 12;

    // Všetky potrebné informácie na spracovanie bloku v reťazi blokov
    private class BlockNode {
        public Block b;
        public BlockNode parent;
        public ArrayList<BlockNode> children;
        public int height;
        // utxo pool na vytvorenie nového bloku na vrchu tohto bloku
        private UTXOPool uPool;

        public BlockNode(Block b, BlockNode parent, UTXOPool uPool) {
            this.b = b;
            this.parent = parent;
            children = new ArrayList<BlockNode>();
            this.uPool = uPool;
            if (parent != null) {
                height = parent.height + 1;
                parent.children.add(this);
            } else {
                height = 1;
            }
        }

        public UTXOPool getUTXOPoolCopy() {
            return new UTXOPool(uPool);
        }
    }

    // Mapovanie hash bloku na BlockNode pre rýchle vyhľadávanie
    private HashMap<ByteArrayWrapper, BlockNode> blockMap;
    // Globálny pool transakcií
    private TransactionPool txPool;
    // Najvyšší blok (najstarší ak je viac v rovnakej výške)
    private BlockNode maxHeightNode;

    /**
     * Vytvor prázdny blockchain iba s prvým (Genesis) blokom. Predpokladajme, že
     * {@code genesisBlock} je platný blok.
     */
    public Blockchain(Block genesisBlock) {
        blockMap = new HashMap<ByteArrayWrapper, BlockNode>();
        txPool = new TransactionPool();

        // Vytvorenie UTXOPool pre genesis blok - pridanie coinbase výstupu
        UTXOPool genesisPool = new UTXOPool();
        Transaction coinbase = genesisBlock.getCoinbase();
        for (int i = 0; i < coinbase.numOutputs(); i++) {
            UTXO utxo = new UTXO(coinbase.getHash(), i);
            genesisPool.addUTXO(utxo, coinbase.getOutput(i));
        }

        BlockNode genesisNode = new BlockNode(genesisBlock, null, genesisPool);
        ByteArrayWrapper genesisHash = new ByteArrayWrapper(genesisBlock.getHash());
        blockMap.put(genesisHash, genesisNode);
        maxHeightNode = genesisNode;
    }

    /** Získaj najvyšší (maximum height) blok */
    public Block getBlockAtMaxHeight() {
        return maxHeightNode.b;
    }

    /**
     * Získaj UTXOPool na ťaženie nového bloku na vrchu najvyššieho (max height) bloku
     */
    public UTXOPool getUTXOPoolAtMaxHeight() {
        return maxHeightNode.getUTXOPoolCopy();
    }

    /** Získaj pool transakcií na vyťaženie nového bloku */
    public TransactionPool getTransactionPool() {
        return txPool;
    }

    /**
     * Pridaj {@code block} do blockchainu, ak je platný. Kvôli platnosti by mali
     * byť všetky transakcie platné a blok by mal byť na
     * {@code height > (maxHeight - CUT_OFF_AGE)}.
     *
     * @return true, ak je blok úspešne pridaný
     */
    public boolean blockAdd(Block block) {
        // Odmietni genesis bloky (prevBlockHash je null)
        if (block.getPrevBlockHash() == null) {
            return false;
        }

        // Nájdi rodičovský blok
        ByteArrayWrapper prevHash = new ByteArrayWrapper(block.getPrevBlockHash());
        BlockNode parentNode = blockMap.get(prevHash);

        // Ak rodičovský blok neexistuje, odmietni
        if (parentNode == null) {
            return false;
        }

        // Skontroluj, či nový blok by bol na height > (maxHeight - CUT_OFF_AGE)
        int newHeight = parentNode.height + 1;
        if (newHeight <= maxHeightNode.height - CUT_OFF_AGE) {
            return false;
        }

        // Vytvor UTXOPool pre nový blok na základe rodičovského UTXOPool
        UTXOPool parentPool = parentNode.getUTXOPoolCopy();

        // Pridaj coinbase UTXO nového bloku do poolu
        Transaction coinbase = block.getCoinbase();
        for (int i = 0; i < coinbase.numOutputs(); i++) {
            UTXO utxo = new UTXO(coinbase.getHash(), i);
            parentPool.addUTXO(utxo, coinbase.getOutput(i));
        }

        // Skontroluj platnosť všetkých transakcií v bloku
        HandleTxs handler = new HandleTxs(parentPool);
        Transaction[] blockTxs = block.getTransactions().toArray(new Transaction[0]);
        Transaction[] validTxs = handler.handler(blockTxs);

        // Všetky transakcie v bloku musia byť platné
        if (validTxs.length != blockTxs.length) {
            return false;
        }

        // Získaj aktualizovaný UTXOPool po spracovaní transakcií
        UTXOPool newPool = handler.UTXOPoolGet();

        // Vytvor nový BlockNode
        BlockNode newNode = new BlockNode(block, parentNode, newPool);
        ByteArrayWrapper blockHash = new ByteArrayWrapper(block.getHash());
        blockMap.put(blockHash, newNode);

        // Aktualizuj maxHeightNode ak je nový blok vyšší
        if (newHeight > maxHeightNode.height) {
            maxHeightNode = newNode;
        }

        // Odstráň transakcie z poolu, ktoré boli zahrnuté do bloku
        for (Transaction tx : block.getTransactions()) {
            txPool.removeTransaction(tx.getHash());
        }

        // Odstráň staré bloky, ktoré sú príliš hlboko (pod CUT_OFF_AGE)
        // Pre jednoduchosť to robíme tak, že odstraňujeme bloky z mapy
        ArrayList<ByteArrayWrapper> toRemove = new ArrayList<ByteArrayWrapper>();
        for (HashMap.Entry<ByteArrayWrapper, BlockNode> entry : blockMap.entrySet()) {
            if (entry.getValue().height < maxHeightNode.height - CUT_OFF_AGE) {
                toRemove.add(entry.getKey());
            }
        }
        for (ByteArrayWrapper key : toRemove) {
            blockMap.remove(key);
        }

        return true;
    }

    /** Pridaj transakciu do transakčného poolu */
    public void transactionAdd(Transaction tx) {
        txPool.addTransaction(tx);
    }
}
