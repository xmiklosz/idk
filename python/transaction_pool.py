# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# TransactionPool - pool nevyriesenych transakcii.


class TransactionPool:
    """Pool transakcii cakajucich na zaradenie do bloku."""

    def __init__(self, pool=None):
        if pool is not None:
            self._map = dict(pool._map)
        else:
            self._map = {}

    def add_transaction(self, tx):
        key = tx.get_hash()
        self._map[key] = tx

    def remove_transaction(self, tx_hash):
        self._map.pop(tx_hash, None)

    def get_transaction(self, tx_hash):
        return self._map.get(tx_hash)

    def get_transactions(self):
        return list(self._map.values())
