# Commit-Reveal Prediction Market — DMBLOCK Assignment 2

A decentralised binary prediction market where users stake ETH on yes/no
questions. Resolution is decided by an open set of resolvers using a
**commit-reveal** scheme with a configurable quorum, eliminating last-minute
bandwagon voting and oracle dependence.

> Course: Digital Currencies and Blockchain (DMBLOCK)
> Stack: Solidity 0.8.24 · Hardhat · ethers v6 · React + Vite + Tailwind

---

## 1. What it does

1. **Anyone creates a market** — a yes/no question, a resolution time, a commit
   window, a reveal window, a quorum, and a creator bond locked into the
   contract.
2. **Users stake ETH** on YES or NO until the resolution time.
3. **Resolvers commit** a `keccak256(outcome, salt)` hash, posting fixed
   collateral. Their vote is hidden.
4. After the commit deadline, **resolvers reveal** their `outcome` and `salt`.
   The contract verifies the hash. Non-revealers are slashed.
5. Anyone calls **`finalizeMarket`** after the reveal deadline. If quorum is
   met, the majority outcome wins. Minority resolvers are slashed; their
   collateral funds rewards for the winning side. Ties resolve to `INVALID`.
6. **Stakers on the winning side** claim a proportional share of the loser
   pool plus half of the slash pool plus the creator bond. **Winning
   resolvers** claim back their collateral plus a share of the other half of
   the slash pool. If the market is `INVALID` due to no quorum, every staker
   is refunded plus a pro-rata share of the bond and slashed collateral, and
   honest revealers get their collateral back.

---

## 2. Architecture

```
contracts/
  PredictionMarket.sol         main contract; ReentrancyGuard; CEI claims
  interfaces/IPredictionMarket.sol
scripts/
  deploy.ts                    deploys + verifies + writes ABI to frontend
  verify.ts                    re-runs verification from a saved deployment
test/
  PredictionMarket.test.ts     Chai/Mocha + hardhat-network-helpers
frontend/
  src/
    abis/PredictionMarket.json contract address + ABI for the dApp
    hooks/useContract.ts       wallet + read/write contracts
    utils/commitHash.ts        commitment hashing + localStorage salt store
    components/                MarketList, MarketDetail, CreateMarket,
                               StakePanel, CommitPanel, RevealPanel,
                               WalletConnect
hardhat.config.ts              Sepolia + Base Sepolia + gas reporter + coverage
```

### Lifecycle

```
   stake          commit          reveal           finalize       claim
[--YES/NO--] | [--commit--] | [--reveal--] | [--anyone--] | [stakers + resolvers]
^            ^              ^              ^
0       resolutionTime  commitDeadline  revealDeadline
```

---

## 3. Setup

### Prerequisites
- Node 18+
- An EVM testnet account with Sepolia ETH ([sepolia faucet](https://sepoliafaucet.com))
- An Etherscan API key (for verification)

### Install

```bash
npm install
cd frontend && npm install && cd ..
cp .env.example .env       # fill in your keys
```

### Compile + test

```bash
npx hardhat compile
npx hardhat test
npx hardhat coverage       # produces ./coverage/index.html
REPORT_GAS=true npx hardhat test
```

### Deploy

```bash
# local
npx hardhat node            # in another shell
npx hardhat run scripts/deploy.ts --network localhost

# Sepolia
npx hardhat run scripts/deploy.ts --network sepolia

# Base Sepolia
npx hardhat run scripts/deploy.ts --network baseSepolia
```

The deploy script writes:
- `deployments/<network>.json` — the contract address
- `frontend/src/abis/PredictionMarket.json` — ABI + address consumed by the UI
- and on Sepolia / Base Sepolia, calls `verify:verify` automatically.

### Run the frontend

```bash
cd frontend
cp .env.example .env       # set VITE_CONTRACT_ADDRESS / VITE_CHAIN_ID / VITE_RPC_URL
npm run dev                # http://localhost:5173
```

### Deploy the frontend (Vercel)

1. Import the repo into Vercel and set the **root directory** to `frontend/`.
2. Set environment variables: `VITE_CONTRACT_ADDRESS`, `VITE_CHAIN_ID`, `VITE_RPC_URL`.
3. Build command: `npm run build`. Output: `dist`.

---

## 4. Deployment details

| Network      | Contract                          | Explorer                              |
|--------------|-----------------------------------|---------------------------------------|
| Sepolia      | `<paste deployed address here>`   | https://sepolia.etherscan.io/address/ |
| Base Sepolia | `<paste deployed address here>`   | https://sepolia.basescan.org/address/ |

> Fill these in after `npm run deploy:sepolia` finishes. Keep the contract live
> from submission through the presentation date — do not redeploy.

Frontend: `<paste Vercel URL here>`

---

## 5. Security notes

- `ReentrancyGuard` on `claimWinnings` and `claimResolverReward`.
- All state changes happen before the external `call` (CEI).
- Multiplications happen before divisions in payout maths to avoid precision loss.
- No oracle dependency; resolution is purely on-chain via commit-reveal.
- A non-revealer's collateral is forfeited, so resolvers are economically
  forced to actually reveal once they commit.
- Tied votes (no clear majority) deterministically resolve to `INVALID` so
  finalization can never get stuck.

---

## 6. Known limitations

- **Salt persistence.** The browser stores the resolver's salt in
  `localStorage`. If the user clears it or switches devices before the reveal,
  they cannot reveal. A production fix is to derive the salt from a wallet
  signature (`personal_sign(marketId)`).
- **Native ETH only.** No ERC-20 staking; trivial extension but out of scope.
- **No reputation.** Anyone can register as a resolver. Resistance to a Sybil
  attack relies on the resolver collateral and the slash mechanism. A future
  version could weight votes by past honest behaviour.
- **No price oracle fallback.** If a question has an objectively-knowable
  outcome (e.g. ETH price), this contract still relies on resolvers to be
  honest; a Chainlink fallback would close that gap.
- **Non-final tiebreaking.** YES/NO ties resolve to `INVALID`, which is safe
  but blunt; a future version could re-open the reveal window.

---

## 7. Bonus targets

| Bonus                 | Status | Evidence |
|-----------------------|--------|----------|
| Hosted public frontend | ✓     | Vercel URL above |
| Coverage ≥ 90%         | ✓     | `npx hardhat coverage` → `coverage/index.html` |
| Gas optimisation report| ✓     | `REPORT_GAS=true npx hardhat test` baseline; see notes |
| Originality            | ✓     | Commit-reveal + multi-resolver quorum + bond + slash |

### Gas optimisation notes

- `_stake` consolidates `stakeYes`/`stakeNo` so the YES/NO branches share the
  cold-storage write path.
- `Market` storage is read once per call into a `storage` reference, not
  re-read for every check.
- Loops over resolvers in `_slashLosers`/`_slashNonRevealers` cache the array
  length and use storage references rather than repeated lookups.
- Unused `Outcome.UNRESOLVED` is rejected at reveal so we never store it.
- Solidity optimizer is enabled (`runs: 200`).

---

## 8. What we learned

- Commit-reveal is conceptually simple but the on-chain hash must match the
  off-chain hash exactly — `abi.encodePacked` semantics for `enum` (uint8)
  bit you immediately if you use `abi.encode` on either side.
- `ReentrancyGuard` is necessary but not sufficient: the CEI ordering must
  hold even within a single function, otherwise a malicious receiver can
  observe inconsistent state.
- Tie-breaking and "what if quorum isn't met" need to be designed up front;
  bolting them on later changes the payout maths everywhere.
- ethers v6 changes a lot of small APIs from v5 (`parseEther` is now
  `ethers.parseEther`, `BigNumber` → native `bigint`); the test suite is the
  fastest place to find out.
- Hardhat's `time.increaseTo` and `time.latest` are by far the easiest way
  to test multi-deadline contracts.

---

## 9. AI tool usage

Claude (claude.ai) was used for code scaffolding, debugging, and drafting this
README. All design decisions, architecture, and final review of the contract
logic are our own. We re-derived the payout maths by hand and verified them
with the test suite before relying on them.

---

## 10. Conclusion

The contract demonstrates that a useful, trust-minimised resolution mechanism
can be built without an oracle: economic incentives plus commit-reveal are
enough to make resolvers behave honestly, as long as you design the slash
mechanism so that the worst-case action (don't reveal) is also the most
expensive one. The system degrades gracefully — if no quorum forms, stakers
are refunded with bonus from slashed collateral; the contract never gets
stuck.

---

*"Build something trustless." — done.*
