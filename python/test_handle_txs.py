# AI: Unit testy vytvorene s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Testy pre HandleTxs a MaxFeeHandleTxs (Faza 1) - 18 testov.

from crypto_utils import RSAKeyPair
from transaction import Transaction
from utxo import UTXO, UTXOPool
from handle_txs import HandleTxs, MaxFeeHandleTxs

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


def make_keys():
    pk_alice = RSAKeyPair(265, seed=b'\x00' * 32)
    pk_bob = RSAKeyPair(265, seed=b'\x01' * 32)
    pk_cyril = RSAKeyPair(265, seed=b'\x02' * 32)
    return pk_alice, pk_bob, pk_cyril


def make_genesis(pk):
    """Vytvori coinbase transakciu (simulacia genesis)."""
    tx = Transaction(10.0, pk.get_public_key())
    return tx


def test1_valid_tx():
    print("\nTest 1: Platna transakcia")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Platna tx je akceptovana", handler.tx_is_valid(tx))


def test2_missing_utxo():
    print("\nTest 2: Chybajuce UTXO")
    pk_a, pk_b, _ = make_keys()

    pool = UTXOPool()
    tx = Transaction()
    tx.add_input(b'\x00' * 32, 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Tx s chybajucim UTXO je odmietnuty", not handler.tx_is_valid(tx))


def test3_invalid_signature():
    print("\nTest 3: Neplatny podpis")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_b.get_private_key(), 0)  # nespravny kluc

    handler = HandleTxs(pool)
    check("Tx s nespravnym podpisom je odmietnuty", not handler.tx_is_valid(tx))


def test4_double_spend():
    print("\nTest 4: Double spend")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_input(genesis.get_hash(), 0)  # rovnaky vstup dvakrat
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)
    tx.sign_tx(pk_a.get_private_key(), 1)

    handler = HandleTxs(pool)
    check("Double spend je odmietnuty", not handler.tx_is_valid(tx))


def test5_negative_output():
    print("\nTest 5: Zaporny vystup")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(-1.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Tx so zapornym vystupom je odmietnuty", not handler.tx_is_valid(tx))


def test6_output_exceeds_input():
    print("\nTest 6: Vystup prevysuje vstup")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(15.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Tx kde vystup > vstup je odmietnuty", not handler.tx_is_valid(tx))


def test7_handler_accepts_valid():
    print("\nTest 7: Handler akceptuje platne tx")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    result = handler.handler([tx])
    check("Handler akceptoval 1 platnu tx", len(result) == 1)


def test8_handler_rejects_invalid():
    print("\nTest 8: Handler odmietne neplatne tx")
    pk_a, pk_b, _ = make_keys()

    pool = UTXOPool()
    tx = Transaction()
    tx.add_input(b'\x00' * 32, 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    result = handler.handler([tx])
    check("Handler odmietol neplatnu tx", len(result) == 0)


def test9_dependent_txs():
    print("\nTest 9: Zavisle transakcie")
    pk_a, pk_b, pk_c = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx1 = Transaction()
    tx1.add_input(genesis.get_hash(), 0)
    tx1.add_output(5.0, pk_b.get_public_key())
    tx1.sign_tx(pk_a.get_private_key(), 0)

    tx2 = Transaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(3.0, pk_c.get_public_key())
    tx2.sign_tx(pk_b.get_private_key(), 0)

    handler = HandleTxs(pool)
    result = handler.handler([tx2, tx1])  # obratevy poradie
    check("Zavisle tx su obe akceptovane", len(result) == 2)


def test10_multiple_outputs():
    print("\nTest 10: Viacero vystupov")
    pk_a, pk_b, pk_c = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(3.0, pk_b.get_public_key())
    tx.add_output(3.0, pk_c.get_public_key())
    tx.add_output(4.0, pk_a.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Tx s viacerymi vystupmi je platna", handler.tx_is_valid(tx))


def test11_multiple_inputs():
    print("\nTest 11: Viacero vstupov")
    pk_a, pk_b, _ = make_keys()
    g1 = Transaction(5.0, pk_a.get_public_key())
    g2 = Transaction(4.0, pk_a.get_public_key())

    pool = UTXOPool()
    pool.add_utxo(UTXO(g1.get_hash(), 0), g1.get_output(0))
    pool.add_utxo(UTXO(g2.get_hash(), 0), g2.get_output(0))

    tx = Transaction()
    tx.add_input(g1.get_hash(), 0)
    tx.add_input(g2.get_hash(), 0)
    tx.add_output(9.0, pk_b.get_public_key())
    # Podpis oboch vstupov pred finalize
    sig0 = pk_a.get_private_key().sign(tx.get_data_to_sign(0))
    tx.add_signature(sig0, 0)
    sig1 = pk_a.get_private_key().sign(tx.get_data_to_sign(1))
    tx.add_signature(sig1, 1)
    tx.finalize()

    handler = HandleTxs(pool)
    check("Tx s viacerymi vstupmi je platna", handler.tx_is_valid(tx))


def test12_zero_output():
    print("\nTest 12: Nulovy vystup")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(0.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    check("Tx s nulovym vystupom je platna", handler.tx_is_valid(tx))


def test13_no_signature():
    print("\nTest 13: Chybajuci podpis")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.finalize()  # bez podpisu

    handler = HandleTxs(pool)
    check("Tx bez podpisu je odmietnuty", not handler.tx_is_valid(tx))


def test14_handler_double_spend():
    print("\nTest 14: Handler odmietne double spend medzi tx")
    pk_a, pk_b, pk_c = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx1 = Transaction()
    tx1.add_input(genesis.get_hash(), 0)
    tx1.add_output(5.0, pk_b.get_public_key())
    tx1.sign_tx(pk_a.get_private_key(), 0)

    tx2 = Transaction()
    tx2.add_input(genesis.get_hash(), 0)
    tx2.add_output(5.0, pk_c.get_public_key())
    tx2.sign_tx(pk_a.get_private_key(), 0)

    handler = HandleTxs(pool)
    result = handler.handler([tx1, tx2])
    check("Len 1 tx z double-spend je akceptovana", len(result) == 1)


def test15_handler_chain():
    print("\nTest 15: Retaz 3 transakcii")
    pk_a, pk_b, pk_c = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx1 = Transaction()
    tx1.add_input(genesis.get_hash(), 0)
    tx1.add_output(8.0, pk_b.get_public_key())
    tx1.sign_tx(pk_a.get_private_key(), 0)

    tx2 = Transaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(6.0, pk_c.get_public_key())
    tx2.sign_tx(pk_b.get_private_key(), 0)

    tx3 = Transaction()
    tx3.add_input(tx2.get_hash(), 0)
    tx3.add_output(4.0, pk_a.get_public_key())
    tx3.sign_tx(pk_c.get_private_key(), 0)

    handler = HandleTxs(pool)
    result = handler.handler([tx3, tx2, tx1])
    check("Vsetky 3 zavisle tx su akceptovane", len(result) == 3)


def test16_max_fee_basic():
    print("\nTest 16: MaxFee zakladny test")
    pk_a, pk_b, _ = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    tx = Transaction()
    tx.add_input(genesis.get_hash(), 0)
    tx.add_output(8.0, pk_b.get_public_key())  # fee = 2.0
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = MaxFeeHandleTxs(pool)
    result = handler.handler([tx])
    check("MaxFee akceptuje platnu tx", len(result) == 1)


def test17_max_fee_prefers_higher():
    print("\nTest 17: MaxFee preferuje vyssie poplatky")
    pk_a, pk_b, pk_c = make_keys()
    genesis = make_genesis(pk_a)

    pool = UTXOPool()
    pool.add_utxo(UTXO(genesis.get_hash(), 0), genesis.get_output(0))

    # tx1: fee = 1.0
    tx1 = Transaction()
    tx1.add_input(genesis.get_hash(), 0)
    tx1.add_output(9.0, pk_b.get_public_key())
    tx1.sign_tx(pk_a.get_private_key(), 0)

    # tx2: fee = 5.0 (konfliktna - rovnaky vstup)
    tx2 = Transaction()
    tx2.add_input(genesis.get_hash(), 0)
    tx2.add_output(5.0, pk_c.get_public_key())
    tx2.sign_tx(pk_a.get_private_key(), 0)

    handler = MaxFeeHandleTxs(pool)
    result = handler.handler([tx1, tx2])
    check("MaxFee vyberie tx s vyssim poplatkom", len(result) == 1)


def test18_max_fee_rejects_invalid():
    print("\nTest 18: MaxFee odmietne neplatne tx")
    pk_a, pk_b, _ = make_keys()

    pool = UTXOPool()
    tx = Transaction()
    tx.add_input(b'\x00' * 32, 0)
    tx.add_output(5.0, pk_b.get_public_key())
    tx.sign_tx(pk_a.get_private_key(), 0)

    handler = MaxFeeHandleTxs(pool)
    result = handler.handler([tx])
    check("MaxFee odmietne neplatnu tx", len(result) == 0)


def main():
    print("=== HandleTxs Tests (Faza 1) ===")

    test1_valid_tx()
    test2_missing_utxo()
    test3_invalid_signature()
    test4_double_spend()
    test5_negative_output()
    test6_output_exceeds_input()
    test7_handler_accepts_valid()
    test8_handler_rejects_invalid()
    test9_dependent_txs()
    test10_multiple_outputs()
    test11_multiple_inputs()
    test12_zero_output()
    test13_no_signature()
    test14_handler_double_spend()
    test15_handler_chain()
    test16_max_fee_basic()
    test17_max_fee_prefers_higher()
    test18_max_fee_rejects_invalid()

    print(f"\n=== Vysledky: {passed} PASSED, {failed} FAILED z 18 ===")


if __name__ == "__main__":
    main()
