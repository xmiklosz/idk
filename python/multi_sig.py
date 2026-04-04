# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# MultiSig - M-of-N multisig transakcie.

from transaction import Transaction
from utxo import UTXO, UTXOPool
from handle_txs import HandleTxs


class MultiSig:
    """M-of-N multisig penazenka."""

    def __init__(self, required_sigs, public_keys):
        if required_sigs <= 0 or required_sigs > len(public_keys):
            raise ValueError(
                f"required_sigs musi byt medzi 1 a {len(public_keys)}")
        self.required_sigs = required_sigs
        self.public_keys = list(public_keys)

    def get_required_sigs(self):
        return self.required_sigs

    def get_public_keys(self):
        return list(self.public_keys)

    def get_num_keys(self):
        return len(self.public_keys)

    def get_key(self, index):
        return self.public_keys[index]

    def get_primary_address(self):
        return self.public_keys[0]

    def verify(self, message, signatures):
        """Overi M-of-N podpisy."""
        if signatures is None or len(signatures) < self.required_sigs:
            return False

        used_keys = set()
        valid_count = 0

        for sig in signatures:
            if sig is None:
                continue
            for k, key in enumerate(self.public_keys):
                if k in used_keys:
                    continue
                if key.verify_signature(message, sig):
                    used_keys.add(k)
                    valid_count += 1
                    break

        return valid_count >= self.required_sigs


class MultiSigTransaction(Transaction):
    """Transakcia s podporou multisig."""

    def __init__(self):
        super().__init__()
        self.multi_signatures = {}  # input_index -> [signatures]
        self.multi_sig_outputs = {}  # output_index -> MultiSig

    def add_multi_sig_output(self, value, multi_sig):
        super().add_output(value, multi_sig.get_primary_address())
        index = self.num_outputs() - 1
        self.multi_sig_outputs[index] = multi_sig

    def add_multi_sig_signature(self, signature, input_index):
        if input_index not in self.multi_signatures:
            self.multi_signatures[input_index] = []
        self.multi_signatures[input_index].append(
            bytes(signature) if signature else None)

        # Prvy podpis pre kompatibilitu s finalize
        if len(self.multi_signatures[input_index]) == 1:
            super().add_signature(signature, input_index)

    def get_multi_signatures(self, input_index):
        return self.multi_signatures.get(input_index, [])

    def is_multi_sig_output(self, output_index):
        return output_index in self.multi_sig_outputs

    def get_multi_sig(self, output_index):
        return self.multi_sig_outputs.get(output_index)


class HandleMultiSigTxs(HandleTxs):
    """HandleTxs s podporou multisig validacie."""

    def __init__(self, utxo_pool):
        super().__init__(utxo_pool)
        self.multi_sig_registry = {}  # UTXO -> MultiSig

    def register_multi_sig(self, utxo, multi_sig):
        self.multi_sig_registry[utxo] = multi_sig

    def get_multi_sig(self, utxo):
        return self.multi_sig_registry.get(utxo)

    def tx_is_valid(self, tx):
        """Overi platnost transakcie vratane multisig."""
        pool = self.utxo_pool_get()
        claimed = set()
        input_sum = 0.0
        output_sum = 0.0

        for i in range(tx.num_inputs()):
            inp = tx.get_input(i)
            utxo = UTXO(inp.prev_tx_hash, inp.output_index)

            if not pool.contains(utxo):
                return False

            output = pool.get_tx_output(utxo)
            message = tx.get_data_to_sign(i)

            # Multisig alebo klasicky podpis
            ms = self.multi_sig_registry.get(utxo)
            if ms is not None:
                if not isinstance(tx, MultiSigTransaction):
                    return False
                sigs = tx.get_multi_signatures(i)
                if not ms.verify(message, sigs):
                    return False
            else:
                public_key = output.address
                sig = inp.signature
                if sig is None or not public_key.verify_signature(message, sig):
                    return False

            if utxo in claimed:
                return False
            claimed.add(utxo)
            input_sum += output.value

        for i in range(tx.num_outputs()):
            output = tx.get_output(i)
            if output.value < 0:
                return False
            output_sum += output.value

        if input_sum < output_sum:
            return False
        return True

    def handler(self, possible_txs):
        """Spracuje transakcie s podporou multisig."""
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

                    for j in range(tx.num_inputs()):
                        inp = tx.get_input(j)
                        utxo = UTXO(inp.prev_tx_hash, inp.output_index)
                        self.utxo_pool_get().remove_utxo(utxo)
                        self.multi_sig_registry.pop(utxo, None)

                    tx_hash = tx.get_hash()
                    for j in range(tx.num_outputs()):
                        utxo = UTXO(tx_hash, j)
                        self.utxo_pool_get().add_utxo(utxo, tx.get_output(j))

                        if isinstance(tx, MultiSigTransaction):
                            if tx.is_multi_sig_output(j):
                                self.multi_sig_registry[utxo] = tx.get_multi_sig(j)

                    txs[i] = None
                    changed = True

        return accepted
