# Subgraph — PredictionMarket events

Indexes the deployed `PredictionMarket` contract so the frontend can load
market history and per-user activity in one query instead of scanning
blockchain logs.

## Setup

```bash
cd subgraph
npm install
# 1. Update subgraph.yaml: set the deployed contract address and startBlock.
# 2. Copy the latest ABI into ./abis (npm run abi after compiling contracts).
npm run codegen
npm run build
```

## Deploy to The Graph Studio

```bash
graph auth --studio <DEPLOY_KEY>
npm run deploy:studio
```

Once deployed, query it from the frontend by setting
`VITE_SUBGRAPH_URL=https://api.studio.thegraph.com/query/<id>/prediction-market/<version>`.

## Example queries

```graphql
query AllMarkets {
  markets(orderBy: createdAt, orderDirection: desc, first: 50) {
    id
    question
    marketType
    state
    result
    totalYesStake
    totalNoStake
    proposer
    disputer
    metadataCID
    createdAt
    resolvedAt
  }
}

query MyActivity($who: Bytes!) {
  stakes(where: { user: $who })       { market { id question } isYes amount blockTimestamp }
  votes:  oracleVotes(where: { oracle: $who }) { market { id } outcome weight }
  claims(where: { user: $who })       { market { id } kind amount }
}
```
