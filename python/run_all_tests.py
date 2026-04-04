# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Spusti vsetky testy (Faza 1 + 2 + 3 + MultiSig).

import test_handle_txs
import test_blockchain
import test_multi_sig
from simulation import run_simulation


def main():
    print("=" * 60)
    print("  PYTHON REWRITE - KOMPLETNE TESTY")
    print("=" * 60)

    # Faza 1
    test_handle_txs.main()
    f1_passed = test_handle_txs.passed
    f1_failed = test_handle_txs.failed

    # Faza 2
    print("\n\n=== TrustedNode Tests (Faza 2) ===")
    f2_passed = 0
    f2_failed = 0
    tests = [
        (0.1, 0.30, 0.01, 10),
        (0.1, 0.30, 0.05, 10),
        (0.1, 0.45, 0.01, 10),
        (0.1, 0.45, 0.05, 10),
    ]
    for p_g, p_m, p_t, rounds in tests:
        result = run_simulation(p_g, p_m, p_t, rounds, seed=42)
        status = "PASS" if result else "FAIL"
        print(f"  {status}: p_graph={p_g} p_malicious={p_m} "
              f"p_txDist={p_t} rounds={rounds}")
        if result:
            f2_passed += 1
        else:
            f2_failed += 1
    print(f"\n=== Vysledky: {f2_passed} PASSED, {f2_failed} FAILED z 4 ===")

    # Faza 3
    print("\n")
    test_blockchain.main()
    f3_passed = test_blockchain.passed
    f3_failed = test_blockchain.failed

    # MultiSig
    print("\n")
    test_multi_sig.main()
    ms_passed = test_multi_sig.passed
    ms_failed = test_multi_sig.failed

    # Sumar
    total_p = f1_passed + f2_passed + f3_passed + ms_passed
    total_f = f1_failed + f2_failed + f3_failed + ms_failed
    print("\n" + "=" * 60)
    print(f"  CELKOVE VYSLEDKY: {total_p} PASSED, {total_f} FAILED")
    print(f"  Faza 1: {f1_passed}/{f1_passed + f1_failed}")
    print(f"  Faza 2: {f2_passed}/{f2_passed + f2_failed}")
    print(f"  Faza 3: {f3_passed}/{f3_passed + f3_failed}")
    print(f"  MultiSig: {ms_passed}/{ms_passed + ms_failed}")
    print("=" * 60)


if __name__ == "__main__":
    main()
