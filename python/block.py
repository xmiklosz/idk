# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Block - blok blockchainu (Faza 3).

from transaction import Transaction
from crypto_utils import sha256


COINBASE = 3.125


class Block:
    """Blok v blockchaine."""

    def __init__(self, prev_hash, address):
        """
        Args:
            prev_hash: hash predchadzajuceho bloku (None pre genesis)
            address: RSAKey adresa pre coinbase transakciu
        """
        self.prev_block_hash = bytes(prev_hash) if prev_hash else None
        self.coinbase = Transaction(COINBASE, address)
        self.txs = []
        self.hash = None

    def get_coinbase(self):
        return self.coinbase

    def get_hash(self):
        return self.hash

    def get_prev_block_hash(self):
        return self.prev_block_hash

    def get_transactions(self):
        return list(self.txs)

    def get_transaction(self, index):
        return self.txs[index]

    def transaction_add(self, tx):
        self.txs.append(tx)

    def get_block(self):
        """Serializuje blok na bajty."""
        raw = bytearray()
        if self.prev_block_hash:
            raw.extend(self.prev_block_hash)
        for tx in self.txs:
            raw.extend(tx.get_tx())
        return bytes(raw)

    def finalize(self):
        """Vypocita SHA-256 hash bloku."""
        self.hash = sha256(self.get_block())
