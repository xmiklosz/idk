# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# HandleTxs a MaxFeeHandleTxs - Faza 1.

from utxo import UTXO, UTXOPool


class HandleTxs:
    """Validacia a spracovanie transakcii (Faza 1)."""

    def __init__(self, utxo_pool):
        self.utxo_pool = UTXOPool(utxo_pool)

    def utxo_pool_get(self):
        if self.utxo_pool is None:
            return UTXOPool()
        return self.utxo_pool

    def tx_is_valid(self, tx):
        """Overi platnost transakcie (5 podmienok)."""
        claimed = set()
        input_sum = 0.0
        output_sum = 0.0

        for i in range(tx.num_inputs()):
            inp = tx.get_input(i)
            utxo = UTXO(inp.prev_tx_hash, inp.output_index)

            # (1) UTXO existuje v poole
            if not self.utxo_pool.contains(utxo):
                return False

            output = self.utxo_pool.get_tx_output(utxo)

            # (2) Platny podpis
            public_key = output.address
            message = tx.get_data_to_sign(i)
            sig = inp.signature
            if sig is None or not public_key.verify_signature(message, sig):
                return False

            # (3) Ziadne UTXO nie je narokovane dvakrat
            if utxo in claimed:
                return False
            claimed.add(utxo)

            input_sum += output.value

        # (4) Vystupne hodnoty su nezaporne
        for i in range(tx.num_outputs()):
            output = tx.get_output(i)
            if output.value < 0:
                return False
            output_sum += output.value

        # (5) Sucet vstupov >= sucet vystupov
        if input_sum < output_sum:
            return False

        return True

    def handler(self, possible_txs):
        """Spracuje transakcie iterativnym greedy pristupom."""
        accepted = []
        txs = list(possible_txs)

        changed = True
        while changed:
            changed = False
            for i in range(len(txs)):
                tx = txs[i]
                if tx is None:
                    continue

                if self.tx_is_valid(tx):
                    accepted.append(tx)

                    # Odstran spotrebovane UTXO
                    for j in range(tx.num_inputs()):
                        inp = tx.get_input(j)
                        utxo = UTXO(inp.prev_tx_hash, inp.output_index)
                        self.utxo_pool.remove_utxo(utxo)

                    # Pridaj nove UTXO
                    tx_hash = tx.get_hash()
                    for j in range(tx.num_outputs()):
                        utxo = UTXO(tx_hash, j)
                        self.utxo_pool.add_utxo(utxo, tx.get_output(j))

                    txs[i] = None
                    changed = True

        return accepted


class MaxFeeHandleTxs(HandleTxs):
    """Maximalizacia poplatkov greedy-by-fee heuristikou."""

    def __init__(self, utxo_pool):
        super().__init__(utxo_pool)

    def _calc_fee(self, tx):
        """Vypocita poplatok transakcie."""
        input_sum = 0.0
        for i in range(tx.num_inputs()):
            inp = tx.get_input(i)
            utxo = UTXO(inp.prev_tx_hash, inp.output_index)
            output = self.utxo_pool.get_tx_output(utxo)
            if output is None:
                return 0.0
            input_sum += output.value
        output_sum = sum(tx.get_output(i).value for i in range(tx.num_outputs()))
        return input_sum - output_sum

    def handler(self, possible_txs):
        """Spracuje transakcie zoradenych podla poplatku (od najvyssieho)."""
        accepted = []
        txs = list(possible_txs)

        changed = True
        while changed:
            changed = False

            # Vypocitaj poplatky a zorad
            valid_with_fees = []
            for i, tx in enumerate(txs):
                if tx is None:
                    continue
                if self.tx_is_valid(tx):
                    fee = self._calc_fee(tx)
                    valid_with_fees.append((fee, i, tx))

            # Zorad podla poplatku zostupne
            valid_with_fees.sort(key=lambda x: -x[0])

            for fee, i, tx in valid_with_fees:
                if txs[i] is None:
                    continue
                if not self.tx_is_valid(tx):
                    continue

                accepted.append(tx)

                for j in range(tx.num_inputs()):
                    inp = tx.get_input(j)
                    utxo = UTXO(inp.prev_tx_hash, inp.output_index)
                    self.utxo_pool.remove_utxo(utxo)

                tx_hash = tx.get_hash()
                for j in range(tx.num_outputs()):
                    utxo = UTXO(tx_hash, j)
                    self.utxo_pool.add_utxo(utxo, tx.get_output(j))

                txs[i] = None
                changed = True

        return accepted
