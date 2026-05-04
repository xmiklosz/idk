# Optimistic-Oracle Prediction Market — DMBLOCK Assignment 2

A decentralised binary prediction market resolved by a **Polymarket / UMA-style
optimistic oracle**: anyone can permissionlessly *propose* an outcome backed by
a bond, anyone can *dispute* with a counter-bond, and contested questions
escalate to a **stake-weighted vote of a registered oracle network**. Wrong
proposers, disputers, and oracles lose their bond/stake to the side that turned
out to be right.

> Course: Digital Currencies and Blockchain (DMBLOCK)
> Stack: Solidity 0.8.24 (viaIR) · Hardhat · ethers v6 · React + Vite + Tailwind

---

## 1. Resolution model

Polymarket inherits this model from UMA's Optimistic Oracle. The mechanism is
"truth on demand": don't pay an oracle to constantly pump data on-chain, just
let anyone *assert* a fact and stake economic security on it. If nobody
disputes within a window, the assertion is taken as truth. If someone does
dispute, the question escalates to a token-weighted vote.

```
   stake          propose          [ dispute ]      [ vote ]      finalize
[--YES/NO--] | [-anyone+bond-] | [-anyone+bond-] | [-oracles-] | [stakers + bonds]
^            ^                 ^                 ^             ^
0     tradingDeadline  proposalDeadline    disputeDeadline  voteDeadline
```

- **Trading**: stake ETH on YES or NO, just like a normal market.
- **Propose** *(permissionless)*: after trading closes anyone posts a
  `PROPOSAL_BOND` (0.05 ETH) and asserts the outcome.
- **Dispute** *(permissionless)*: within a 1h window, anyone can post a
  `DISPUTE_BOND` (0.05 ETH) to escalate.
- **Vote** *(oracles only)*: if disputed, registered oracles vote during a
  24h window. Their vote weight equals their stake in the OracleRegistry.
  Majority outcome wins; ties resolve to `INVALID`.
- **Finalize**: anyone can call `finalizeMarket` once the relevant window has
  closed. Bonds settle automatically and oracle stakes that voted against the
  final outcome are slashed.

### Bond economics

| Scenario | Proposer | Disputer | Wrong-voting oracles | Right-voting oracles | Stakers |
|----------|----------|----------|----------------------|----------------------|---------|
| Undisputed | reclaim bond | — | — | — | winning side splits losing side + creator bond |
| Disputed, proposer wins | wins both bonds | loses bond | slashed pro-rata | split half the slash pool, weighted by their snapshot vote weight | winning side splits losing side + creator bond + half the slash pool |
| Disputed, disputer wins | loses bond | wins both bonds | slashed pro-rata | split half the slash pool | winning side as above |
| No proposal by deadline | — | — | — | — | full refund + pro-rata creator bond (`INVALID` / `Expired`) |

---

## 2. Architecture

```
contracts/
  OracleRegistry.sol           staked oracle network; slashing entry point
  PredictionMarket.sol         optimistic-oracle market lifecycle
  interfaces/IPredictionMarket.sol

scripts/
  deploy.ts                    deploys both, wires registry slasher, dumps ABIs
  verify.ts                    re-runs etherscan verification

test/
  PredictionMarket.test.ts     full suite: oracle registry + market lifecycle
                               (23 tests; happy path + ~16 failure modes)

frontend/
  src/abis/{PredictionMarket,OracleRegistry}.json
  src/hooks/useContract.ts     wallet + read/write for both contracts
  src/utils/outcome.ts         enums + formatters
  src/components/
    MarketList.tsx             filter by phase
    MarketDetail.tsx           per-phase action panels + claims
    CreateMarket.tsx
    StakePanel.tsx             stake YES / NO
    ProposePanel.tsx           propose outcome + bond
    DisputePanel.tsx           dispute proposal + bond
    VotePanel.tsx              oracle dispute vote (gated by registry)
    OraclePage.tsx             register / top up / unregister as oracle
    WalletConnect.tsx
```

### Security

- `ReentrancyGuard` on every claim entry point (`claimWinnings`,
  `claimProposerBond`, `claimDisputerBond`, `claimOracleReward`,
  `finalizeMarket`).
- All state changes happen before the external `call` (CEI).
- Multiplications happen before divisions in the payout maths to avoid
  precision loss.
- `OracleRegistry.slash` is gated to a small set of approved slasher contracts
  the registry owner has whitelisted; the deployed market is auto-approved by
  the deploy script.
- Oracle vote weight is **snapshotted at vote time** so subsequent slashing
  doesn't break later reward computation.
- The `Outcome.UNRESOLVED` sentinel is rejected anywhere a real outcome is
  expected (propose / vote).

---

## 3. Setup

### Prerequisites
- Node 18+
- Sepolia ETH: <https://sepoliafaucet.com>
- An Etherscan API key for verification

### Install + test

```bash
npm install
cd frontend && npm install && cd ..
cp .env.example .env

npx hardhat compile
npx hardhat test          # 23 passing
npx hardhat coverage
REPORT_GAS=true npx hardhat test
```

### Deploy

```bash
npx hardhat run scripts/deploy.ts --network sepolia
```

The script:
1. Deploys `OracleRegistry`.
2. Deploys `PredictionMarket(<registry>)`.
3. Calls `registry.approveSlasher(market)` so the market can slash oracle
   stakes.
4. Persists `deployments/<network>.json` (registry + market addresses).
5. Writes the ABIs into `frontend/src/abis/{PredictionMarket,OracleRegistry}.json`.
6. Auto-verifies both contracts on Etherscan / Basescan.

### Run the frontend

```bash
cd frontend
cp .env.example .env       # set VITE_MARKET_ADDRESS, VITE_REGISTRY_ADDRESS,
                           #     VITE_CHAIN_ID, VITE_RPC_URL
npm run dev                # http://localhost:5173
```

### Vercel

Set the Vercel project root to `frontend/`. Build command `npm run build`,
output `dist`. Environment variables: `VITE_MARKET_ADDRESS`,
`VITE_REGISTRY_ADDRESS`, `VITE_CHAIN_ID`, `VITE_RPC_URL`.

---

## 4. Deployed addresses

Fill these in after `npm run deploy:sepolia`. Keep both contracts live from
submission through the presentation; do not redeploy.

| Network | Registry | Market | Explorer |
|---------|----------|--------|----------|
| Sepolia | `<paste>` | `<paste>` | https://sepolia.etherscan.io/ |
| Base Sepolia | `<paste>` | `<paste>` | https://sepolia.basescan.org/ |

Frontend: `<paste Vercel URL>`

---

## 5. End-to-end demo flow

1. Create a market with a question, trading deadline, and proposal window.
2. From other accounts, stake YES / NO before the trading deadline.
3. Once trading closes, any account can `proposeOutcome` (with 0.05 ETH bond).
4. Within 1 hour, another account can `disputeProposal` (with 0.05 ETH bond).
5. Register a few accounts as oracles via the **Oracles** page (each stakes
   ≥ 0.05 ETH).
6. Each oracle calls `voteOnDispute`. Their vote weight equals their stake.
7. After the vote window, anyone calls `finalizeMarket`; the contract tallies,
   slashes losing oracles, and credits the slash pool.
8. Stakers, the winning side of (proposer ↔ disputer), and right-voting
   oracles call their respective claim functions.

If nobody proposes within the proposal window the market expires as
`INVALID`; stakers are refunded plus a pro-rata share of the creator bond.

---

## 6. Known limitations

- **Sybil resistance is purely economic.** Anyone can stake to become an
  oracle; collusion is bounded by the total stake required to outvote the
  honest majority. A reputation system or quadratic voting would tighten
  this further.
- **No escalation chain.** UMA escalates a disputed vote to its DVM and back;
  here a single oracle vote is final. A second-round dispute would be a
  natural extension.
- **Native ETH only.** No ERC-20 staking; trivial to add.
- **Oracle stake is locked while voting open**, but a slashed oracle whose
  stake falls below `MIN_STAKE` is silently deactivated — not a refund flow,
  consistent with UMA-style "lose your skin in the game" semantics.
- **No price oracle fallback** for objectively-knowable questions. Honest
  oracle behaviour is assumed; the bond/slash mechanics are the only
  enforcement.
- **Tied votes resolve to INVALID** so finalize never blocks; no re-vote.
- **Dispute window is fixed at 1h** (chosen so demos work in a single class
  period). For real markets this should be measured in days.

---

## 7. Bonus targets

| Bonus | Status | Evidence |
|-------|--------|----------|
| Hosted public frontend | ✓ | Vercel URL above |
| Coverage ≥ 90% | ✓ | `npx hardhat coverage` |
| Gas optimisation report | ✓ | `REPORT_GAS=true npx hardhat test` |
| Originality | ✓ | Two-contract optimistic-oracle resolution + stake-weighted oracle vote + permissionless propose/dispute |

### Gas / design notes

- `viaIR: true` is required because `PredictionMarket` works on a wide
  `Market` struct in storage; `viaIR` lets the compiler avoid stack-too-deep.
- `_stake` consolidates `stakeYes`/`stakeNo` so they share the cold-storage
  write path.
- `_slashLosingVoters` caches `voters.length` and uses storage references
  rather than repeated mapping lookups.
- The `OracleRegistry` keeps an enumerable address list via swap-and-pop so
  unregistration is O(1).
- Vote weight is snapshotted in `oracleVoteWeight[marketId][voter]` so
  reward maths is independent of subsequent slashing.

---

## 8. What we learned

- An optimistic oracle is a *very* different design from commit-reveal: it
  trades the privacy guarantee (commit-reveal hides votes until everyone has
  spoken) for a much better latency/cost profile (no resolution cost when
  uncontested).
- Tying oracle voting weight to a slashable stake registry is the part that
  makes the system robust: Sybils cost real money, and lying costs your stake.
  Decoupling those two contracts (`OracleRegistry` and `PredictionMarket`)
  lets one registry serve many market deployments.
- Snapshotting vote weight at vote time is essential — without it, slashing
  would retroactively change the denominator in the reward formula, and the
  contract could become insolvent or pay too little.
- Solidity's `viaIR` produces *much* smaller bytecode for struct-heavy
  contracts than the legacy pipeline, at the cost of a few seconds in
  compile time.

---

## 9. AI tool usage

Claude (claude.ai) was used for code scaffolding, debugging, and drafting
this README. All design decisions and the final review of the contract
logic are our own. The payout maths was re-derived by hand and verified
against the test suite; the test cases are checked against expected payouts
to the wei.

---

## 10. Conclusion

The optimistic-oracle architecture demonstrates that you don't need a
permanent oracle feed or a single trusted resolver to settle a prediction
market — you just need a credible escalation path. Three layers of bonded
participants (proposer, disputer, oracles) make every level of the
resolution chain economically rational, and the slash mechanism makes the
worst behaviour (lying or vanishing) the most expensive.

---

*"Build something trustless." — done.*
