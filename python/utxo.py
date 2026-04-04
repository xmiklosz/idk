# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# UTXO a UTXOPool.


class UTXO:
    """Nepoužitý transakčný výstup (Unspent Transaction Output)."""

    def __init__(self, tx_hash, index):
        self.tx_hash = bytes(tx_hash) if tx_hash else None
        self.index = index

    def get_tx_hash(self):
        return self.tx_hash

    def get_index(self):
        return self.index

    def __eq__(self, other):
        if not isinstance(other, UTXO):
            return False
        return self.tx_hash == other.tx_hash and self.index == other.index

    def __hash__(self):
        return hash((self.tx_hash, self.index))

    def __lt__(self, other):
        if self.index != other.index:
            return self.index < other.index
        return self.tx_hash < other.tx_hash


class UTXOPool:
    """Mapovanie UTXO na ich vystupy."""

    def __init__(self, pool=None):
        if pool is not None:
            self._map = dict(pool._map)
        else:
            self._map = {}

    def add_utxo(self, utxo, tx_output):
        self._map[utxo] = tx_output

    def remove_utxo(self, utxo):
        self._map.pop(utxo, None)

    def get_tx_output(self, utxo):
        return self._map.get(utxo)

    def contains(self, utxo):
        return utxo in self._map

    def get_all_utxo(self):
        return list(self._map.keys())
