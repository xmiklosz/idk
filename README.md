# Hybrid Prediction Market — DMBLOCK Assignment 2

A binary prediction market with **two complementary resolution paths**:

1. **Chainlink auto-resolution** — for objectively-knowable price questions
   ("Will ETH > $3000 at trading-close?"). The contract reads a Chainlink
   AggregatorV3 feed and finalizes itself: zero trust, zero manual oracle
   intervention.
2. **Optimistic oracle (Polymarket / UMA style)** — for subjective questions.
   Anyone may propose an outcome backed by a bond, anyone may dispute, and
   contested questions escalate to a **stake-weighted vote of a registered
   oracle network**.

Plus four web3 integrations layered on top:

- **Chainlink Data Feeds** — trustless price-based resolution.
- **IPFS** — extended market metadata (description, image, sources) stored
  off-chain to keep gas costs low.
- **The Graph** — subgraph indexes every event so the frontend loads market
  history in one query rather than scanning blockchain logs.
- **Event-driven notifications** — in-app toasts triggered by the contract's
  events (proposal, dispute, finalize) when something happens to the
  connected user, with a documented hook into Push Protocol for
  cross-device push.

> Course: Digital Currencies and Blockchain (DMBLOCK)
> Stack: Solidity 0.8.24 (viaIR) · Hardhat · ethers v6 · React + Vite + Tailwind

---

## 0. Web3 integrations at a glance

| Integration | Where | What it does |
|-------------|-------|--------------|
| **Chainlink Data Feeds** | `PredictionMarket.autoResolve` | Reads `latestRoundData` from any AggregatorV3 feed and resolves the market YES if `price > threshold`, else NO. Built-in 1h staleness guard; falls back to manual finalize if the feed is broken. |
| **Chainlink Automation** | `PredictionMarket.checkUpkeep` / `performUpkeep` | The contract implements `AutomationCompatibleInterface`. Once a Chainlink Automation upkeep is registered against it, price-feed markets settle themselves the moment they're eligible — no human caller required. |
| **IPFS** | `Market.metadataCID`, `frontend/utils/ipfs.ts` | Stores extended market metadata off-chain. The frontend fetches the JSON via a public gateway and renders description / image / sources. |
| **The Graph** | `subgraph/` | Manifest + schema + AssemblyScript mappings indexing every contract event. Deploy it to The Graph Studio and point `VITE_SUBGRAPH_URL` at it. |
| **Notifications** | `frontend/hooks/useNotifications.ts` | Subscribes to `Proposed`, `Disputed`, `MarketFinalized` events and shows an in-app toast when the connected user is involved. README documents how to forward those events to Push Protocol for true mobile push. |
| **Frontend polish** | `Leaderboard`, theme toggle, search, share | Top-stakers/oracles leaderboard, dark/light theme toggle, market search, share-link button, live countdowns on every market card. |

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

## 1a. Chainlink auto-resolution path

For price questions, create the market with `createPriceMarket(...)` instead
of `createMarket(...)`. After `tradingDeadline`, anyone calls
`autoResolve(marketId)`:

```solidity
(, int256 price, , uint256 updatedAt, ) =
    AggregatorV3Interface(m.priceFeed).latestRoundData();
require(block.timestamp - updatedAt <= 1 hours, "stale price");
Outcome r = price > m.priceThreshold ? Outcome.YES : Outcome.NO;
```

If the feed is stale or reverts, `autoResolve` reverts and stakers can call
`finalizeMarket` after `proposalDeadline` to expire the market and recover
their stakes plus the creator bond. Frontend exposes preset feeds for ETH/USD,
BTC/USD, LINK/USD, USDC/USD on Sepolia (see `frontend/src/utils/priceFeeds.ts`).

## 1b. IPFS metadata

`Market.metadataCID` is an optional CID that points to a JSON document of the
form:

```json
{
  "description": "Long-form market description.",
  "imageCID":    "bafy...",
  "sources":     ["https://example.com/source1"],
  "tags":        ["crypto", "prices"]
}
```

Pin it via Pinata, web3.storage, or IPFS Desktop and paste the CID in the
"Metadata CID" field on the Create Market form. The detail page resolves it
through `https://ipfs.io/ipfs/<cid>` (gateway is overridable via
`VITE_IPFS_GATEWAY`) and renders the description, image, and source links.

## 2. Architecture

```
contracts/
  OracleRegistry.sol           staked oracle network; slashing entry point
  PredictionMarket.sol         hybrid market: optimistic oracle + Chainlink
  interfaces/AggregatorV3Interface.sol
  interfaces/IPredictionMarket.sol
  test/MockAggregatorV3.sol    test-only Chainlink stub

subgraph/
  subgraph.yaml                manifest
  schema.graphql               entities (Market, Stake, Proposal, Dispute,
                               OracleVote, PriceResolution, Claim)
  src/mapping.ts               AssemblyScript event handlers

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
    MarketDetail.tsx           per-phase action panels + claims; renders
                               IPFS metadata + Chainlink autoResolve button
    CreateMarket.tsx           toggle Manual / Chainlink, pick preset feed,
                               paste IPFS CID
    StakePanel.tsx             stake YES / NO
    ProposePanel.tsx           propose outcome + bond
    DisputePanel.tsx           dispute proposal + bond
    VotePanel.tsx              oracle dispute vote (gated by registry)
    OraclePage.tsx             register / top up / unregister as oracle
    WalletConnect.tsx
  src/hooks/
    useContract.ts             wallet + read/write for both contracts
    useNotifications.ts        event-driven in-app notifications
  src/utils/
    outcome.ts                 enums + formatters
    ipfs.ts                    CID validation + gateway URL + metadata fetch
    priceFeeds.ts              Chainlink presets per chain
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

### The Graph subgraph

```bash
cd subgraph
npm install
# Set the deployed contract address + startBlock in subgraph.yaml first.
npm run prepare        # copies the latest ABI from artifacts/
npm run codegen
npm run build
graph auth --studio <DEPLOY_KEY>
npm run deploy:studio
```

After it indexes, set `VITE_SUBGRAPH_URL` in the frontend `.env` so the UI
queries the indexer instead of scanning logs. Example queries are in
`subgraph/README.md`.

### Chainlink Automation integration

The contract implements `AutomationCompatibleInterface`, so a Chainlink
Automation upkeep can settle every price-feed market the moment its trading
window closes:

```solidity
function checkUpkeep(bytes calldata checkData)
    external view returns (bool upkeepNeeded, bytes memory performData);
function performUpkeep(bytes calldata performData) external;
```

`checkUpkeep` is run off-chain by Chainlink's keeper network — gas there is
free. It scans the markets in `[start, end)` (the optional `checkData`
range, default `[0, marketCount)`), skips manual markets and stale feeds,
and returns the first eligible price-market id. `performUpkeep` then calls
`_autoResolve(marketId)` on-chain and wraps it in `nonReentrant`.

To register an upkeep:
1. Deploy the contract.
2. Open <https://automation.chain.link>, connect the deploy wallet, and
   create a Custom Logic upkeep against the deployed `PredictionMarket`
   address.
3. Top up its LINK balance. Done — the network now polls the contract on
   every block and settles eligible markets automatically.

If the LINK balance runs out or the upkeep is paused, the contract still
allows a normal manual `autoResolve(marketId)` call by anyone, so the
Automation integration is purely an upgrade — never a single point of
failure.

### Push Protocol integration

The in-app `useNotifications` hook gives instant feedback while the user has
the dApp open. To extend it to true cross-device push:

1. Register a Push Protocol channel via <https://app.push.org> (one-time;
   testnets are free).
2. Capture the same events server-side (e.g. a small Node listener that
   subscribes to the contract's events) and forward them via
   `@pushprotocol/restapi`'s `PushAPI.payloads.sendNotification` to channel
   subscribers.
3. Users opt in by subscribing to your channel from any Push-compatible
   wallet. Notifications then arrive on iOS / Android / web push.

The same pattern works with XMTP if you'd rather DM the user from a bot
identity instead of broadcasting from a channel.

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
| Coverage ≥ 90% | ✓ | `coverage-summary.txt` — 90.78% statements (37 tests passing) |
| Gas optimisation report | ✓ | `gas-report.txt` (committed) |
| Originality | ✓ | Hybrid resolution (Chainlink Data Feeds **+** Chainlink Automation **+** optimistic oracle **+** stake-weighted vote) plus IPFS metadata, The Graph subgraph, event-driven notifications, leaderboard, theme toggle, search/share/countdowns |

### Coverage snapshot

```
File                                |  % Stmts | % Branch |  % Funcs |  % Lines |
------------------------------------|----------|----------|----------|----------|
 contracts/                         |    90.74 |    68.07 |    84.62 |    92.70 |
  OracleRegistry.sol                |    76.32 |    55.26 |    71.43 |    82.46 |
  PredictionMarket.sol              |    93.82 |    70.50 |    92.00 |    95.39 |
 contracts/interfaces/              |   100.00 |   100.00 |   100.00 |   100.00 |
 contracts/test/                    |   100.00 |   100.00 |   100.00 |   100.00 |
------------------------------------|----------|----------|----------|----------|
 All files                          |    90.78 |    68.07 |    86.05 |    92.96 |
```

Open `coverage/index.html` after running `npx hardhat coverage` for the
line-by-line breakdown.

### Gas snapshot (top entries from `gas-report.txt`)

```
Method            | Avg gas
------------------|--------
createMarket      | 169,051
createPriceMarket | 251,684
stakeYes / stakeNo|  74,506 /  74,550
proposeOutcome    |  75,515
disputeProposal   |  74,981
voteOnDispute     | 135,978
finalizeMarket    |  74,539  (35,986 in the no-vote/no-dispute fast path)
autoResolve       |  52,767
performUpkeep     |  52,928  (Chainlink Automation entry point)
claimWinnings     |  70,135
```

Deployment cost of `PredictionMarket` is 2.80 M gas (4.7% of block limit).
Notable optimisations:
- `_stake` consolidates YES/NO branches so the cold-storage write path is
  shared.
- `_slashLosingVoters` caches the voters array length and uses storage
  references rather than repeated mapping lookups.
- `_autoResolve` is the hot path for both the manual `autoResolve` external
  function and `performUpkeep` — refactoring it out of the external entry
  saves the cost of a `.call` round-trip when Automation triggers.
- The optimizer is enabled (`runs: 200`) and `viaIR: true` so the wider
  `Market` struct compiles without stack-too-deep.

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
