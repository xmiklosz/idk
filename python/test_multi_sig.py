# AI: Unit testy vytvorene s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Testy pre MultiSig (bonus).

from crypto_utils import RSAKeyPair
from transaction import Transaction
from utxo import UTXO, UTXOPool
from block import Block
from multi_sig import MultiSig, MultiSigTransaction, HandleMultiSigTxs

passed = 0
failed = 0


def check(name, condition):
    global passed, failed
    if condition:
        print(f"  PASS: {name}")
        passed += 1
    else:
        print(f"  FAIL: {name}")
        failed += 1


pk_alice = RSAKeyPair(265, seed=b'\x0a' * 32)
pk_bob = RSAKeyPair(265, seed=b'\x0b' * 32)
pk_cyril = RSAKeyPair(265, seed=b'\x0c' * 32)
pk_dave = RSAKeyPair(265, seed=b'\x0d' * 32)


def test1_create_wallet():
    print("\nTest 1: Vytvorenie multisig penazenky")
    keys = [pk_alice.get_public_key(), pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(2, keys)

    check("requiredSigs == 2", wallet.get_required_sigs() == 2)
    check("numKeys == 3", wallet.get_num_keys() == 3)
    check("primaryAddress je Alice", wallet.get_primary_address() == pk_alice.get_public_key())

    thrown = False
    try:
        MultiSig(0, keys)
    except ValueError:
        thrown = True
    check("requiredSigs=0 vyhodi vynimku", thrown)

    thrown = False
    try:
        MultiSig(4, keys)
    except ValueError:
        thrown = True
    check("requiredSigs>N vyhodi vynimku", thrown)


def test2_sign_one_participant():
    print("\nTest 2: Podpis jednym ucastnikom (2-of-3)")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_alice.get_public_key(), pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(2, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    result = handler.handler([tx1])
    check("tx1 (odoslanie na multisig) platna", len(result) == 1)

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.5, pk_dave.get_public_key())
    sig_a = pk_alice.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_a, 0)
    tx2.finalize()

    check("Len 1 podpis z 2-of-3 je neplatny", not handler.tx_is_valid(tx2))


def test3_sign_minimum():
    print("\nTest 3: Podpis minimalnym poctom (2-of-3)")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_alice.get_public_key(), pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(2, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.5, pk_dave.get_public_key())
    sig_a = pk_alice.get_private_key().sign(tx2.get_data_to_sign(0))
    sig_b = pk_bob.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_a, 0)
    tx2.add_multi_sig_signature(sig_b, 0)
    tx2.finalize()

    check("2 podpisy z 2-of-3 su platne", handler.tx_is_valid(tx2))


def test4_sign_all():
    print("\nTest 4: Podpis vsetkymi (3-of-3)")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_alice.get_public_key(), pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(3, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.9, pk_dave.get_public_key())
    for pk in [pk_alice, pk_bob, pk_cyril]:
        s = pk.get_private_key().sign(tx2.get_data_to_sign(0))
        tx2.add_multi_sig_signature(s, 0)
    tx2.finalize()

    check("3 podpisy z 3-of-3 su platne", handler.tx_is_valid(tx2))


def test5_invalid_signer():
    print("\nTest 5: Neplatny podpisovatel")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_alice.get_public_key(), pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(2, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.5, pk_dave.get_public_key())
    sig_a = pk_alice.get_private_key().sign(tx2.get_data_to_sign(0))
    sig_d = pk_dave.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_a, 0)
    tx2.add_multi_sig_signature(sig_d, 0)
    tx2.finalize()

    check("Podpis neucastnikom (Dave) je neplatny", not handler.tx_is_valid(tx2))


def test6_send_to_multisig():
    print("\nTest 6: Odoslanie na multisig")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(1, keys)

    tx = MultiSigTransaction()
    tx.add_input(genesis.get_coinbase().get_hash(), 0)
    tx.add_multi_sig_output(2.0, wallet)
    tx.add_output(1.0, pk_alice.get_public_key())
    sig = pk_alice.get_private_key().sign(tx.get_data_to_sign(0))
    tx.add_signature(sig, 0)
    tx.finalize()

    handler = HandleMultiSigTxs(pool)
    result = handler.handler([tx])
    check("Odoslanie na multisig je platne", len(result) == 1)


def test7_spend_from_multisig():
    print("\nTest 7: Minanie z multisig (1-of-2)")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_bob.get_public_key(), pk_cyril.get_public_key()]
    wallet = MultiSig(1, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.5, pk_dave.get_public_key())
    sig_b = pk_bob.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_b, 0)
    tx2.finalize()

    result = handler.handler([tx2])
    check("1-of-2 multisig: Bob sam moze minutt", len(result) == 1)


def test8_double_spend_multisig():
    print("\nTest 8: Double-spend z multisig")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys = [pk_alice.get_public_key(), pk_bob.get_public_key()]
    wallet = MultiSig(1, keys)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo, wallet)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.0, pk_cyril.get_public_key())
    sig_a = pk_alice.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_a, 0)
    tx2.finalize()

    tx3 = MultiSigTransaction()
    tx3.add_input(tx1.get_hash(), 0)
    tx3.add_output(2.0, pk_dave.get_public_key())
    sig_b = pk_bob.get_private_key().sign(tx3.get_data_to_sign(0))
    tx3.add_multi_sig_signature(sig_b, 0)
    tx3.finalize()

    result = handler.handler([tx2, tx3])
    check("Double-spend z multisig: len 1 prijata", len(result) == 1)


def test9_chain_multisig():
    print("\nTest 9: Retazenie multisig transakcii")
    genesis = Block(None, pk_alice.get_public_key())
    genesis.finalize()

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_coinbase().get_hash(), 0),
                  genesis.get_coinbase().get_output(0))

    keys1 = [pk_alice.get_public_key(), pk_bob.get_public_key()]
    wallet1 = MultiSig(2, keys1)

    tx1 = MultiSigTransaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_multi_sig_output(3.0, wallet1)
    sig = pk_alice.get_private_key().sign(tx1.get_data_to_sign(0))
    tx1.add_signature(sig, 0)
    tx1.finalize()

    handler = HandleMultiSigTxs(pool)
    handler.handler([tx1])

    ms_utxo1 = UTXO(tx1.get_hash(), 0)
    handler.register_multi_sig(ms_utxo1, wallet1)

    keys2 = [pk_cyril.get_public_key(), pk_dave.get_public_key()]
    wallet2 = MultiSig(1, keys2)

    tx2 = MultiSigTransaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_multi_sig_output(2.5, wallet2)
    sig_a = pk_alice.get_private_key().sign(tx2.get_data_to_sign(0))
    sig_b = pk_bob.get_private_key().sign(tx2.get_data_to_sign(0))
    tx2.add_multi_sig_signature(sig_a, 0)
    tx2.add_multi_sig_signature(sig_b, 0)
    tx2.finalize()

    result = handler.handler([tx2])
    check("Presun z 2-of-2 na 1-of-2 multisig platny", len(result) == 1)


def main():
    print("=== MultiSig Tests (Bonus) ===")

    test1_create_wallet()
    test2_sign_one_participant()
    test3_sign_minimum()
    test4_sign_all()
    test5_invalid_signer()
    test6_send_to_multisig()
    test7_spend_from_multisig()
    test8_double_spend_multisig()
    test9_chain_multisig()

    print(f"\n=== Vysledky: {passed} PASSED, {failed} FAILED z 9 ===")


if __name__ == "__main__":
    main()
