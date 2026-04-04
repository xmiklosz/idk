# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Blockchain - stromova struktura s forkmi (Faza 3).

from utxo import UTXO, UTXOPool
from transaction_pool import TransactionPool
from handle_txs import HandleTxs


CUT_OFF_AGE = 12


class BlockNode:
    """Uzol v strome blockchainu."""

    def __init__(self, block, parent, utxo_pool):
        self.block = block
        self.parent = parent
        self.children = []
        self.utxo_pool = utxo_pool

        if parent is not None:
            self.height = parent.height + 1
            parent.children.append(self)
        else:
            self.height = 1

    def get_utxo_pool_copy(self):
        return UTXOPool(self.utxo_pool)


class Blockchain:
    """Blockchain s podporou forkov a CUT_OFF_AGE."""

    def __init__(self, genesis_block):
        self.block_map = {}
        self.tx_pool = TransactionPool()

        # UTXOPool pre genesis blok
        genesis_pool = UTXOPool()
        coinbase = genesis_block.get_coinbase()
        for i in range(coinbase.num_outputs()):
            utxo = UTXO(coinbase.get_hash(), i)
            genesis_pool.add_utxo(utxo, coinbase.get_output(i))

        genesis_node = BlockNode(genesis_block, None, genesis_pool)
        genesis_hash = genesis_block.get_hash()
        self.block_map[genesis_hash] = genesis_node
        self.max_height_node = genesis_node

    def get_block_at_max_height(self):
        return self.max_height_node.block

    def get_utxo_pool_at_max_height(self):
        return self.max_height_node.get_utxo_pool_copy()

    def get_transaction_pool(self):
        return self.tx_pool

    def block_add(self, block):
        """Prida blok do blockchainu ak je platny."""
        # Odmietni genesis bloky
        if block.get_prev_block_hash() is None:
            return False

        # Najdi rodicovsky blok
        prev_hash = block.get_prev_block_hash()
        parent_node = self.block_map.get(prev_hash)
        if parent_node is None:
            return False

        # Skontroluj vysku
        new_height = parent_node.height + 1
        if new_height <= self.max_height_node.height - CUT_OFF_AGE:
            return False

        # Vytvor UTXOPool z rodica
        parent_pool = parent_node.get_utxo_pool_copy()

        # Pridaj coinbase
        coinbase = block.get_coinbase()
        for i in range(coinbase.num_outputs()):
            utxo = UTXO(coinbase.get_hash(), i)
            parent_pool.add_utxo(utxo, coinbase.get_output(i))

        # Validuj transakcie
        handler = HandleTxs(parent_pool)
        block_txs = block.get_transactions()
        valid_txs = handler.handler(block_txs)

        if len(valid_txs) != len(block_txs):
            return False

        # Pridaj novy uzol
        new_pool = handler.utxo_pool_get()
        new_node = BlockNode(block, parent_node, new_pool)
        self.block_map[block.get_hash()] = new_node

        # Aktualizuj max height
        if new_height > self.max_height_node.height:
            self.max_height_node = new_node

        # Odstran transakcie z poolu
        for tx in block.get_transactions():
            self.tx_pool.remove_transaction(tx.get_hash())

        # Cleanup starych blokov
        to_remove = [h for h, node in self.block_map.items()
                     if node.height < self.max_height_node.height - CUT_OFF_AGE]
        for h in to_remove:
            del self.block_map[h]

        return True

    def transaction_add(self, tx):
        self.tx_pool.add_transaction(tx)
