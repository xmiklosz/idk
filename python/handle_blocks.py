# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# HandleBlocks - spracovanie blokov (Faza 3).

from handle_txs import HandleTxs


class HandleBlocks:
    """Spracovanie blokov v blockchaine."""

    def __init__(self, blockchain):
        self.blockchain = blockchain

    def block_process(self, block):
        """Prida blok do blockchainu."""
        if block is None:
            return False
        return self.blockchain.block_add(block)

    def block_create(self, my_address):
        """Vytvori novy blok na vrchu najvyssieho bloku."""
        from block import Block

        parent = self.blockchain.get_block_at_max_height()
        new_block = Block(parent.get_hash(), my_address)

        pool = self.blockchain.get_utxo_pool_at_max_height()
        tx_pool = self.blockchain.get_transaction_pool()

        handler = HandleTxs(pool)
        valid_txs = handler.handler(tx_pool.get_transactions())

        for tx in valid_txs:
            new_block.transaction_add(tx)

        new_block.finalize()

        if self.blockchain.block_add(new_block):
            return new_block
        return None

    def tx_process(self, tx):
        """Prida transakciu do poolu."""
        self.blockchain.transaction_add(tx)
