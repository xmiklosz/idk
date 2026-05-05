import { BigInt, Bytes, log } from "@graphprotocol/graph-ts";
import {
  MarketCreated,
  Staked,
  Proposed,
  Disputed,
  OracleVoted,
  MarketFinalized,
  PriceMarketResolved,
  Claimed,
  ProposerBondClaimed,
  DisputerBondClaimed,
  OracleRewardClaimed,
} from "../generated/PredictionMarket/PredictionMarket";
import {
  Market,
  Stake,
  Proposal,
  Dispute,
  OracleVote,
  PriceResolution,
  Claim,
} from "../generated/schema";

function eventId(txHash: Bytes, logIndex: BigInt): string {
  return txHash.toHexString() + "-" + logIndex.toString();
}

export function handleMarketCreated(event: MarketCreated): void {
  let id = event.params.marketId.toString();
  let m = new Market(id);
  m.creator           = event.params.creator;
  m.question          = event.params.question;
  m.metadataCID       = event.params.metadataCID;
  m.marketType        = event.params.marketType;
  m.tradingDeadline   = event.params.tradingDeadline;
  m.proposalDeadline  = BigInt.zero(); // filled by a follow-up read if needed
  m.state             = 0;
  m.result            = 0;
  m.proposedOutcome   = 0;
  m.proposer          = null;
  m.disputer          = null;
  m.totalYesStake     = BigInt.zero();
  m.totalNoStake      = BigInt.zero();
  m.creatorBond       = BigInt.zero();
  m.slashPool         = BigInt.zero();
  m.winningVoteWeight = BigInt.zero();
  m.createdAt         = event.block.timestamp;
  m.resolvedAt        = null;
  m.save();
}

export function handleStaked(event: Staked): void {
  let id = eventId(event.transaction.hash, event.logIndex);
  let s = new Stake(id);
  s.market         = event.params.marketId.toString();
  s.user           = event.params.user;
  s.isYes          = event.params.isYes;
  s.amount         = event.params.amount;
  s.blockNumber    = event.block.number;
  s.blockTimestamp = event.block.timestamp;
  s.txHash         = event.transaction.hash;
  s.save();

  let m = Market.load(event.params.marketId.toString());
  if (m) {
    if (event.params.isYes) m.totalYesStake = m.totalYesStake.plus(event.params.amount);
    else                    m.totalNoStake  = m.totalNoStake.plus(event.params.amount);
    m.save();
  }
}

export function handleProposed(event: Proposed): void {
  let id = eventId(event.transaction.hash, event.logIndex);
  let p = new Proposal(id);
  p.market          = event.params.marketId.toString();
  p.proposer        = event.params.proposer;
  p.outcome         = event.params.outcome;
  p.disputeDeadline = event.params.disputeDeadline;
  p.blockTimestamp  = event.block.timestamp;
  p.txHash          = event.transaction.hash;
  p.save();

  let m = Market.load(event.params.marketId.toString());
  if (m) {
    m.state           = 1;
    m.proposedOutcome = event.params.outcome;
    m.proposer        = event.params.proposer;
    m.save();
  }
}

export function handleDisputed(event: Disputed): void {
  let id = eventId(event.transaction.hash, event.logIndex);
  let d = new Dispute(id);
  d.market         = event.params.marketId.toString();
  d.disputer       = event.params.disputer;
  d.voteDeadline   = event.params.voteDeadline;
  d.blockTimestamp = event.block.timestamp;
  d.txHash         = event.transaction.hash;
  d.save();

  let m = Market.load(event.params.marketId.toString());
  if (m) {
    m.state    = 2;
    m.disputer = event.params.disputer;
    m.save();
  }
}

export function handleOracleVoted(event: OracleVoted): void {
  let id = eventId(event.transaction.hash, event.logIndex);
  let v = new OracleVote(id);
  v.market         = event.params.marketId.toString();
  v.oracle         = event.params.oracle;
  v.outcome        = event.params.outcome;
  v.weight         = event.params.weight;
  v.blockTimestamp = event.block.timestamp;
  v.txHash         = event.transaction.hash;
  v.save();
}

export function handleMarketFinalized(event: MarketFinalized): void {
  let m = Market.load(event.params.marketId.toString());
  if (m) {
    m.state      = m.state == 0 ? 4 : 3; // Trading -> Expired, else Resolved
    m.result     = event.params.result;
    m.resolvedAt = event.block.timestamp;
    m.save();
  }
}

export function handlePriceResolved(event: PriceMarketResolved): void {
  let id = eventId(event.transaction.hash, event.logIndex);
  let p = new PriceResolution(id);
  p.market         = event.params.marketId.toString();
  p.feed           = event.params.feed;
  p.price          = event.params.price;
  p.threshold      = event.params.threshold;
  p.outcome        = event.params.result;
  p.blockTimestamp = event.block.timestamp;
  p.txHash         = event.transaction.hash;
  p.save();
}

function recordClaim(
  marketId: BigInt,
  user: Bytes,
  amount: BigInt,
  kind: string,
  txHash: Bytes,
  logIndex: BigInt,
  ts: BigInt
): void {
  let id = eventId(txHash, logIndex);
  let c = new Claim(id);
  c.market         = marketId.toString();
  c.user           = user;
  c.kind           = kind;
  c.amount         = amount;
  c.blockTimestamp = ts;
  c.txHash         = txHash;
  c.save();
}

export function handleClaimed(event: Claimed): void {
  recordClaim(event.params.marketId, event.params.user, event.params.amount,
              "winnings", event.transaction.hash, event.logIndex, event.block.timestamp);
}
export function handleProposerBondClaimed(event: ProposerBondClaimed): void {
  recordClaim(event.params.marketId, event.params.proposer, event.params.amount,
              "proposerBond", event.transaction.hash, event.logIndex, event.block.timestamp);
}
export function handleDisputerBondClaimed(event: DisputerBondClaimed): void {
  recordClaim(event.params.marketId, event.params.disputer, event.params.amount,
              "disputerBond", event.transaction.hash, event.logIndex, event.block.timestamp);
}
export function handleOracleRewardClaimed(event: OracleRewardClaimed): void {
  recordClaim(event.params.marketId, event.params.oracle, event.params.amount,
              "oracleReward", event.transaction.hash, event.logIndex, event.block.timestamp);
}
