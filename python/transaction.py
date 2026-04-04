# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Transakcia - vstupy, vystupy, podpisy, hash.

import struct
from crypto_utils import sha256


class Transaction:
    """Transakcia v UTXO modeli."""

    class Input:
        def __init__(self, prev_tx_hash, output_index):
            self.prev_tx_hash = bytes(prev_tx_hash) if prev_tx_hash else None
            self.output_index = output_index
            self.signature = None

        def add_signature(self, sig):
            self.signature = bytes(sig) if sig else None

        def __eq__(self, other):
            if not isinstance(other, Transaction.Input):
                return False
            return (self.prev_tx_hash == other.prev_tx_hash and
                    self.output_index == other.output_index and
                    self.signature == other.signature)

        def __hash__(self):
            return hash((self.prev_tx_hash, self.output_index,
                         self.signature if self.signature else b''))

    class Output:
        def __init__(self, value, address):
            self.value = value
            self.address = address  # RSAKey

        def __eq__(self, other):
            if not isinstance(other, Transaction.Output):
                return False
            return (self.value == other.value and
                    self.address == other.address)

        def __hash__(self):
            return hash((self.value, self.address))

    def __init__(self, coin=None, address=None):
        self.hash = None
        self.inputs = []
        self.outputs = []
        self.coinbase = False

        if coin is not None and address is not None:
            self.coinbase = True
            self.add_output(coin, address)
            self.finalize()

    def copy(self):
        """Vytvori kopiu transakcie."""
        tx = Transaction()
        tx.hash = bytes(self.hash) if self.hash else None
        tx.inputs = list(self.inputs)
        tx.outputs = list(self.outputs)
        tx.coinbase = self.coinbase
        return tx

    def is_coinbase(self):
        return self.coinbase

    def add_input(self, prev_tx_hash, output_index):
        inp = self.Input(prev_tx_hash, output_index)
        self.inputs.append(inp)

    def add_output(self, value, address):
        out = self.Output(value, address)
        self.outputs.append(out)

    def remove_input(self, index):
        del self.inputs[index]

    def get_data_to_sign(self, index):
        """Serializuje data na podpisanie pre dany vstup."""
        if index >= len(self.inputs):
            return None
        sig_data = bytearray()
        inp = self.inputs[index]
        if inp.prev_tx_hash:
            sig_data.extend(inp.prev_tx_hash)
        sig_data.extend(struct.pack('>i', inp.output_index))
        for out in self.outputs:
            sig_data.extend(struct.pack('>d', out.value))
            exp_bytes = out.address.exponent.to_bytes(
                (out.address.exponent.bit_length() + 7) // 8, 'big')
            mod_bytes = out.address.modulus.to_bytes(
                (out.address.modulus.bit_length() + 7) // 8, 'big')
            sig_data.extend(exp_bytes)
            sig_data.extend(mod_bytes)
        return bytes(sig_data)

    def add_signature(self, signature, index):
        self.inputs[index].add_signature(signature)

    def get_tx(self):
        """Serializuje celu transakciu na bajty."""
        raw = bytearray()
        for inp in self.inputs:
            if inp.prev_tx_hash:
                raw.extend(inp.prev_tx_hash)
            raw.extend(struct.pack('>i', inp.output_index))
            if inp.signature:
                raw.extend(inp.signature)
        for out in self.outputs:
            raw.extend(struct.pack('>d', out.value))
            exp_bytes = out.address.exponent.to_bytes(
                (out.address.exponent.bit_length() + 7) // 8, 'big')
            mod_bytes = out.address.modulus.to_bytes(
                (out.address.modulus.bit_length() + 7) // 8, 'big')
            raw.extend(exp_bytes)
            raw.extend(mod_bytes)
        return bytes(raw)

    def finalize(self):
        """Vypocita SHA-256 hash transakcie."""
        self.hash = sha256(self.get_tx())

    def get_hash(self):
        return self.hash

    def get_input(self, index):
        return self.inputs[index] if index < len(self.inputs) else None

    def get_output(self, index):
        return self.outputs[index] if index < len(self.outputs) else None

    def num_inputs(self):
        return len(self.inputs)

    def num_outputs(self):
        return len(self.outputs)

    def sign_tx(self, private_key, input_index):
        """Pomocna metoda - podpise vstup a finalizuje."""
        sig = private_key.sign(self.get_data_to_sign(input_index))
        self.add_signature(sig, input_index)
        self.finalize()
