# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Kryptograficke utility - RSA kluce, podpisovanie, overovanie.

import hashlib
import secrets
import struct


def _miller_rabin(n, k=20):
    """Miller-Rabin test prvocisla."""
    if n < 2:
        return False
    if n == 2 or n == 3:
        return True
    if n % 2 == 0:
        return False

    r, d = 0, n - 1
    while d % 2 == 0:
        r += 1
        d //= 2

    for _ in range(k):
        a = secrets.randbelow(n - 3) + 2
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(r - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True


def _generate_prime(bits, rng=None):
    """Vygeneruje nahodne prvocislo s danym poctom bitov."""
    while True:
        if rng:
            p = int.from_bytes(rng(bits // 8), 'big')
        else:
            p = secrets.randbits(bits)
        p |= (1 << (bits - 1)) | 1  # nastav najvyssi bit a neparnost
        if _miller_rabin(p):
            return p


def sha256(data):
    """SHA-256 hash."""
    if isinstance(data, (list, bytearray)):
        data = bytes(data)
    return hashlib.sha256(data).digest()


class RSAKey:
    """RSA kluc (verejny alebo sukromny)."""

    def __init__(self, exponent, modulus):
        self.exponent = exponent
        self.modulus = modulus

    def _raw_encrypt(self, data_int):
        return pow(data_int, self.exponent, self.modulus)

    def sign(self, message):
        """Podpise spravu (hash + RSA sifrovanie sukromnym klucom)."""
        h = sha256(message)
        h_int = int.from_bytes(h, 'big')
        sig_int = self._raw_encrypt(h_int)
        byte_len = (self.modulus.bit_length() + 7) // 8
        return sig_int.to_bytes(byte_len, 'big')

    def verify_signature(self, message, signature):
        """Overi podpis (RSA desifrovanie verejnym klucom + porovnanie s hashom)."""
        if signature is None:
            return False
        try:
            h = sha256(message)
            h_int = int.from_bytes(h, 'big')
            sig_int = int.from_bytes(signature, 'big')
            decrypted = self._raw_encrypt(sig_int)
            return decrypted == h_int
        except Exception:
            return False

    def __eq__(self, other):
        if not isinstance(other, RSAKey):
            return False
        return self.exponent == other.exponent and self.modulus == other.modulus

    def __hash__(self):
        return hash((self.exponent, self.modulus))


class RSAKeyPair:
    """Generovanie RSA parov klucov."""

    E = 65537

    def __init__(self, bits=512, seed=None):
        """Vygeneruje RSA par klucov.

        Args:
            bits: pocet bitov pre kazdy faktor (celkovy modulus bude 2*bits)
            seed: seed pre deterministicke generovanie (bytes)
        """
        if seed is not None:
            import random
            r = random.Random()
            r.seed(int.from_bytes(seed, 'big'))
            rng = lambda n: bytes([r.randint(0, 255) for _ in range(n)])
        else:
            rng = None

        while True:
            p = _generate_prime(bits, rng)
            q = _generate_prime(bits, rng)
            if p == q:
                continue
            n = p * q
            phi = (p - 1) * (q - 1)
            from math import gcd
            if gcd(self.E, phi) != 1:
                continue
            d = pow(self.E, -1, phi)
            break

        self.public_key = RSAKey(self.E, n)
        self.private_key = RSAKey(d, n)

    def get_public_key(self):
        return self.public_key

    def get_private_key(self):
        return self.private_key
