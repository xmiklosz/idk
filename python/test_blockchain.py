# AI: Unit testy vytvorene s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Testy pre Blockchain (Faza 3).

from crypto_utils import RSAKeyPair
from transaction import Transaction
from utxo import UTXO, UTXOPool
from block import Block
from blockchain import Blockchain
from handle_blocks import HandleBlocks

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


pk_alice = RSAKeyPair(265, seed=b'\x00' * 32)
pk_bob = RSAKeyPair(265, seed=b'\x01' * 32)
pk_cyril = RSAKeyPair(265, seed=b'\x02' * 32)
pk_dave = RSAKeyPair(265, seed=b'\x03' * 32)


def new_chain():
    genesis = Block(None, pk_bob.get_public_key())
    genesis.finalize()
    bc = Blockchain(genesis)
    hb = HandleBlocks(bc)
    return genesis, bc, hb


def test1_block_no_txs():
    print("\nTest 1: Blok bez transakcii")
    genesis, bc, hb = new_chain()
    b = Block(genesis.get_hash(), pk_alice.get_public_key())
    b.finalize()
    check("Prazdny blok je platny", hb.block_process(b))


def test2_block_valid_tx():
    print("\nTest 2: Blok s platnou tx")
    genesis, bc, hb = new_chain()
    b = Block(genesis.get_hash(), pk_alice.get_public_key())
    tx = Transaction()
    tx.add_input(genesis.get_coinbase().get_hash(), 0)
    tx.add_output(2.0, pk_alice.get_public_key())
    tx.sign_tx(pk_bob.get_private_key(), 0)
    b.transaction_add(tx)
    b.finalize()
    check("Blok s platnou tx je akceptovany", hb.block_process(b))


def test3_double_spend():
    print("\nTest 3: Double spend v bloku")
    genesis, bc, hb = new_chain()
    b = Block(genesis.get_hash(), pk_alice.get_public_key())

    tx1 = Transaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_output(2.0, pk_alice.get_public_key())
    tx1.sign_tx(pk_bob.get_private_key(), 0)

    tx2 = Transaction()
    tx2.add_input(genesis.get_coinbase().get_hash(), 0)
    tx2.add_output(2.0, pk_cyril.get_public_key())
    tx2.sign_tx(pk_bob.get_private_key(), 0)

    b.transaction_add(tx1)
    b.transaction_add(tx2)
    b.finalize()
    check("Blok s double-spend je odmietnuty", not hb.block_process(b))


def test4_new_genesis():
    print("\nTest 4: Novy genesis blok")
    genesis, bc, hb = new_chain()
    b = Block(None, pk_alice.get_public_key())
    b.finalize()
    check("Novy genesis je odmietnuty", not hb.block_process(b))


def test5_fork():
    print("\nTest 5: Fork")
    genesis, bc, hb = new_chain()

    b1 = Block(genesis.get_hash(), pk_alice.get_public_key())
    b1.finalize()
    check("Block 1 pridany", hb.block_process(b1))

    b2 = Block(genesis.get_hash(), pk_bob.get_public_key())
    b2.finalize()
    check("Fork block pridany", hb.block_process(b2))


def test6_longest_chain():
    print("\nTest 6: Najdlhsi retazec")
    genesis, bc, hb = new_chain()

    b1 = Block(genesis.get_hash(), pk_alice.get_public_key())
    b1.finalize()
    hb.block_process(b1)

    b2 = Block(genesis.get_hash(), pk_bob.get_public_key())
    b2.finalize()
    hb.block_process(b2)

    b3 = Block(b1.get_hash(), pk_cyril.get_public_key())
    b3.finalize()
    hb.block_process(b3)

    check("Max height je 3", bc.get_block_at_max_height().get_hash() == b3.get_hash())


def test7_invalid_parent():
    print("\nTest 7: Neexistujuci rodic")
    genesis, bc, hb = new_chain()
    b = Block(b'\xff' * 32, pk_alice.get_public_key())
    b.finalize()
    check("Blok s neexistujucim rodicom odmietnuty", not hb.block_process(b))


def test8_chain_spending():
    print("\nTest 8: Retaz utracania coinov")
    genesis, bc, hb = new_chain()

    # Block 1: Bob -> Alice
    b1 = Block(genesis.get_hash(), pk_alice.get_public_key())
    tx1 = Transaction()
    tx1.add_input(genesis.get_coinbase().get_hash(), 0)
    tx1.add_output(3.0, pk_alice.get_public_key())
    tx1.sign_tx(pk_bob.get_private_key(), 0)
    b1.transaction_add(tx1)
    b1.finalize()
    check("Block 1 (Bob->Alice) platny", hb.block_process(b1))

    # Block 2: Alice -> Cyril
    b2 = Block(b1.get_hash(), pk_bob.get_public_key())
    tx2 = Transaction()
    tx2.add_input(tx1.get_hash(), 0)
    tx2.add_output(2.0, pk_cyril.get_public_key())
    tx2.sign_tx(pk_alice.get_private_key(), 0)
    b2.transaction_add(tx2)
    b2.finalize()
    check("Block 2 (Alice->Cyril) platny", hb.block_process(b2))


def test9_invalid_tx_in_block():
    print("\nTest 9: Neplatna tx v bloku")
    genesis, bc, hb = new_chain()
    b = Block(genesis.get_hash(), pk_alice.get_public_key())
    tx = Transaction()
    tx.add_input(genesis.get_coinbase().get_hash(), 0)
    tx.add_output(2.0, pk_alice.get_public_key())
    tx.sign_tx(pk_alice.get_private_key(), 0)  # nespravny kluc
    b.transaction_add(tx)
    b.finalize()
    check("Blok s neplatnou tx odmietnuty", not hb.block_process(b))


def test10_cut_off_age():
    print("\nTest 10: CUT_OFF_AGE")
    genesis, bc, hb = new_chain()

    prev = genesis
    blocks = [genesis]
    for i in range(15):
        b = Block(prev.get_hash(), pk_alice.get_public_key())
        b.finalize()
        hb.block_process(b)
        blocks.append(b)
        prev = b

    # Blok nad genesis (vyska 2) - mal by byt odmietnuty (prilis stary)
    old = Block(genesis.get_hash(), pk_bob.get_public_key())
    old.finalize()
    check("Blok nad starym blokom (pod CUT_OFF_AGE) odmietnuty",
          not hb.block_process(old))


def main():
    print("=== Blockchain Tests (Faza 3) ===")

    test1_block_no_txs()
    test2_block_valid_tx()
    test3_double_spend()
    test4_new_genesis()
    test5_fork()
    test6_longest_chain()
    test7_invalid_parent()
    test8_chain_spending()
    test9_invalid_tx_in_block()
    test10_cut_off_age()

    print(f"\n=== Vysledky: {passed} PASSED, {failed} FAILED z {passed + failed} ===")


if __name__ == "__main__":
    main()
