# AI: Implementacia vytvorena s pomocou Claude AI (Anthropic) - Python rewrite (bonus 5 bodov).
# Simulacia byzantskeho konsenzu (Faza 2).

import random
import sys
from trusted_node import TrustedNode, Transaction2


class ByzantineNode:
    """Skodlivy uzol - neposiela nic."""

    def __init__(self, num_nodes, malicious, followees):
        pass

    def initial_proposal(self):
        return set()

    def followees_receive(self, candidates):
        pass

    def get_consensus(self):
        return set()


def run_simulation(p_graph, p_malicious, p_tx_dist, num_rounds, seed=None):
    """Spusti simulaciu konsenzu.

    Args:
        p_graph: pravdepodobnost hrany medzi uzlami
        p_malicious: podiel skodlivych uzlov
        p_tx_dist: pravdepodobnost ze uzol dostane transakciu
        num_rounds: pocet kol konsenzu
        seed: seed pre reprodukovatelnost
    """
    num_nodes = 100
    num_txs = 500

    if seed is not None:
        random.seed(seed)

    # Vytvor nahodny graf
    followees = [[False] * num_nodes for _ in range(num_nodes)]
    for i in range(num_nodes):
        for j in range(num_nodes):
            if i != j and random.random() < p_graph:
                followees[i][j] = True

    # Urc skodlivych
    malicious = [False] * num_nodes
    num_malicious = int(p_malicious * num_nodes)
    mal_indices = random.sample(range(num_nodes), num_malicious)
    for idx in mal_indices:
        malicious[idx] = True

    # Vytvor uzly
    nodes = []
    for i in range(num_nodes):
        if malicious[i]:
            nodes.append(ByzantineNode(num_nodes, malicious, followees[i]))
        else:
            nodes.append(TrustedNode(num_nodes, malicious, followees[i]))

    # Rozdel transakcie
    initial_txs = [set() for _ in range(num_nodes)]
    for i in range(num_nodes):
        for tx_id in range(num_txs):
            if random.random() < p_tx_dist:
                initial_txs[i].add(Transaction2(tx_id))

    # Nastav pociatocne navrhy
    for i in range(num_nodes):
        if not malicious[i]:
            nodes[i].consensus_txs = set(initial_txs[i])

    # Simulacia kol
    for r in range(num_rounds):
        proposals = [node.initial_proposal() for node in nodes]

        for i in range(num_nodes):
            candidates = []
            for j in range(num_nodes):
                if followees[i][j]:
                    for tx in proposals[j]:
                        candidates.append((tx.id, j))
            nodes[i].followees_receive(candidates)

    # Kontrola konsenzu
    good_nodes = [i for i in range(num_nodes) if not malicious[i]]

    if not good_nodes:
        print("Ziadne dobre uzly!")
        return False

    baseline = nodes[good_nodes[0]].get_consensus()
    baseline_ids = {tx.id for tx in baseline}
    all_agree = True

    for i in good_nodes[1:]:
        node_ids = {tx.id for tx in nodes[i].get_consensus()}
        if node_ids != baseline_ids:
            all_agree = False
            break

    return all_agree


def main():
    if len(sys.argv) == 5:
        p_graph = float(sys.argv[1])
        p_malicious = float(sys.argv[2])
        p_tx_dist = float(sys.argv[3])
        num_rounds = int(sys.argv[4])
        result = run_simulation(p_graph, p_malicious, p_tx_dist, num_rounds)
        print(f"p_graph={p_graph} p_malicious={p_malicious} "
              f"p_txDist={p_tx_dist} rounds={num_rounds}")
        print("Konsenzus:", "PASS" if result else "FAIL")
    else:
        print("=== Simulacia byzantskeho konsenzu ===\n")
        tests = [
            (0.1, 0.30, 0.01, 10),
            (0.1, 0.30, 0.05, 10),
            (0.1, 0.45, 0.01, 10),
            (0.1, 0.45, 0.05, 10),
        ]
        passed = 0
        for p_g, p_m, p_t, rounds in tests:
            result = run_simulation(p_g, p_m, p_t, rounds, seed=42)
            status = "PASS" if result else "FAIL"
            print(f"  p_graph={p_g} p_malicious={p_m} "
                  f"p_txDist={p_t} rounds={rounds} -> {status}")
            if result:
                passed += 1
        print(f"\nVysledky: {passed}/{len(tests)} PASSED")


if __name__ == "__main__":
    main()
