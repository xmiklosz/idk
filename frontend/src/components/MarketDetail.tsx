import { useCallback, useEffect, useState } from "react";
import { useParams, Link } from "react-router-dom";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import StakePanel from "./StakePanel";
import ProposePanel from "./ProposePanel";
import DisputePanel from "./DisputePanel";
import VotePanel from "./VotePanel";
import {
  Outcome,
  State,
  outcomeLabel,
  stateLabel,
  fmtTimestamp,
  countdown,
  shortAddr,
} from "../utils/outcome";
import { ipfsUrl, fetchMetadata, MarketMetadata } from "../utils/ipfs";

const MARKET_TYPE_MANUAL = 0;
const MARKET_TYPE_PRICE  = 1;

interface FullMarket {
  creator: string;
  question: string;
  metadataCID: string;
  tradingDeadline: bigint;
  proposalDeadline: bigint;
  disputeDeadline: bigint;
  voteDeadline: bigint;
  totalYesStake: bigint;
  totalNoStake: bigint;
  creatorBond: bigint;
  state: number;
  marketType: number;
  proposedOutcome: number;
  result: number;
  proposer: string;
  disputer: string;
  yesVoteWeight: bigint;
  noVoteWeight: bigint;
  invalidVoteWeight: bigint;
  slashPool: bigint;
  winningVoteWeight: bigint;
  proposerBondClaimed: boolean;
  disputerBondClaimed: boolean;
  priceFeed: string;
  priceThreshold: bigint;
}

interface UserState {
  yesStake: bigint;
  noStake: bigint;
  claimed: boolean;
  vote: number;        // 0 = none
  voteWeight: bigint;
  oracleClaimed: boolean;
}

const stateStyle: Record<number, string> = {
  [State.Trading]:  "bg-blue-500/20 text-blue-300",
  [State.Proposed]: "bg-amber-500/20 text-amber-300",
  [State.Disputed]: "bg-rose-500/20 text-rose-300",
  [State.Resolved]: "bg-emerald-500/20 text-emerald-300",
  [State.Expired]:  "bg-slate-500/20 text-slate-300",
};

export default function MarketDetail({ wallet }: { wallet: WalletState }) {
  const { id } = useParams<{ id: string }>();
  const marketId = BigInt(id ?? "0");

  const [market, setMarket] = useState<FullMarket | null>(null);
  const [user, setUser] = useState<UserState | null>(null);
  const [now, setNow] = useState(Math.floor(Date.now() / 1000));
  const [busy, setBusy] = useState(false);
  const [metadata, setMetadata] = useState<MarketMetadata | null>(null);

  useEffect(() => {
    const i = setInterval(() => setNow(Math.floor(Date.now() / 1000)), 1000);
    return () => clearInterval(i);
  }, []);

  const refresh = useCallback(async () => {
    try {
      const m = await wallet.market.markets(marketId);
      setMarket({
        creator: m.creator,
        question: m.question,
        metadataCID: m.metadataCID,
        tradingDeadline: m.tradingDeadline,
        proposalDeadline: m.proposalDeadline,
        disputeDeadline: m.disputeDeadline,
        voteDeadline: m.voteDeadline,
        totalYesStake: m.totalYesStake,
        totalNoStake: m.totalNoStake,
        creatorBond: m.creatorBond,
        state: Number(m.state),
        marketType: Number(m.marketType),
        proposedOutcome: Number(m.proposedOutcome),
        result: Number(m.result),
        proposer: m.proposer,
        disputer: m.disputer,
        yesVoteWeight: m.yesVoteWeight,
        noVoteWeight: m.noVoteWeight,
        invalidVoteWeight: m.invalidVoteWeight,
        slashPool: m.slashPool,
        winningVoteWeight: m.winningVoteWeight,
        proposerBondClaimed: m.proposerBondClaimed,
        disputerBondClaimed: m.disputerBondClaimed,
        priceFeed: m.priceFeed,
        priceThreshold: m.priceThreshold,
      });
      if (m.metadataCID) {
        fetchMetadata(m.metadataCID).then(setMetadata).catch(() => setMetadata(null));
      } else {
        setMetadata(null);
      }
      if (wallet.account) {
        const [y, n, c, v, vw, oc] = await Promise.all([
          wallet.market.yesStakes(marketId, wallet.account),
          wallet.market.noStakes(marketId, wallet.account),
          wallet.market.claimed(marketId, wallet.account),
          wallet.market.oracleVote(marketId, wallet.account),
          wallet.market.oracleVoteWeight(marketId, wallet.account),
          wallet.market.oracleClaimed(marketId, wallet.account),
        ]);
        setUser({
          yesStake: y, noStake: n, claimed: c,
          vote: Number(v), voteWeight: vw, oracleClaimed: oc,
        });
      } else {
        setUser(null);
      }
    } catch (e) { console.error(e); }
  }, [marketId, wallet.market, wallet.account]);

  useEffect(() => { refresh(); }, [refresh]);

  async function call(method: string, label: string) {
    if (!wallet.marketWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading(`${label}…`);
    try {
      const tx = await (wallet.marketWrite as any)[method](marketId);
      await tx.wait();
      toast.success(label, { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  if (!market) return <p className="text-slate-400">Loading market…</p>;

  const isPriceMarket = market.marketType === MARKET_TYPE_PRICE;

  // What action is currently available?
  const canAutoResolve = isPriceMarket && market.state === State.Trading && now >= Number(market.tradingDeadline);
  const canPropose = !isPriceMarket && market.state === State.Trading && now >= Number(market.tradingDeadline) && now < Number(market.proposalDeadline);
  const canDispute = !isPriceMarket && market.state === State.Proposed && now < Number(market.disputeDeadline);
  const canVote    = !isPriceMarket && market.state === State.Disputed && now < Number(market.voteDeadline);
  const canFinalize = !isPriceMarket && (
    (market.state === State.Trading  && now >= Number(market.proposalDeadline)) ||
    (market.state === State.Proposed && now >= Number(market.disputeDeadline)) ||
    (market.state === State.Disputed && now >= Number(market.voteDeadline))
  );

  const isResolved = market.state === State.Resolved || market.state === State.Expired;

  const userStakedOnWinner = user && isResolved && (
    (market.state === State.Expired)                              ? (user.yesStake + user.noStake) > 0n :
    (market.result === Outcome.YES)                               ? user.yesStake > 0n :
    (market.result === Outcome.NO)                                ? user.noStake > 0n :
    /* INVALID */                                                   (user.yesStake + user.noStake) > 0n
  );

  const isProposer = !!user && wallet.account?.toLowerCase() === market.proposer.toLowerCase();
  const isDisputer = !!user && market.disputer !== ethers.ZeroAddress &&
    wallet.account?.toLowerCase() === market.disputer.toLowerCase();

  const proposerCanClaim =
    isProposer && market.state === State.Resolved && !market.proposerBondClaimed &&
    (market.disputer === ethers.ZeroAddress || market.proposedOutcome === market.result);
  const disputerCanClaim =
    isDisputer && market.state === State.Resolved && !market.disputerBondClaimed &&
    market.proposedOutcome !== market.result;

  const oracleCanClaim =
    !!user && market.state === State.Resolved &&
    user.vote !== 0 && user.vote === market.result &&
    !user.oracleClaimed && market.winningVoteWeight > 0n;

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <Link to="/" className="text-brand-500 text-sm">&larr; Back to markets</Link>
        <button
          type="button"
          onClick={async () => {
            try {
              if ((navigator as any).share) {
                await (navigator as any).share({ title: market.question, url: window.location.href });
              } else {
                await navigator.clipboard.writeText(window.location.href);
                toast.success("Link copied");
              }
            } catch {}
          }}
          className="text-xs px-3 py-1.5 rounded-md bg-slate-800 hover:bg-slate-700 text-slate-300"
        >
          Share
        </button>
      </div>

      <div className="card">
        <div className="flex items-start justify-between gap-3 flex-wrap">
          <div>
            <h2 className="text-xl font-semibold leading-snug">{market.question}</h2>
            <div className="text-sm text-slate-400 mt-1">
              Market #{marketId.toString()} · created by {shortAddr(market.creator)}
            </div>
          </div>
          <div className="flex flex-col items-end gap-1">
            <span className={`badge ${stateStyle[market.state]}`}>{stateLabel(market.state)}</span>
            <span className={`badge ${isPriceMarket ? "bg-purple-500/20 text-purple-300" : "bg-cyan-500/20 text-cyan-300"}`}>
              {isPriceMarket ? "Chainlink" : "Optimistic"}
            </span>
          </div>
        </div>

        {(market.metadataCID || metadata) && (
          <div className="mt-3 text-xs text-slate-400 border-t border-slate-700 pt-3 space-y-1">
            {metadata?.description && <div>{metadata.description}</div>}
            {market.metadataCID && (
              <div className="font-mono break-all">
                IPFS: <a className="text-brand-500 underline" target="_blank" rel="noreferrer"
                         href={ipfsUrl(market.metadataCID)}>{market.metadataCID}</a>
              </div>
            )}
            {metadata?.imageCID && (
              <img src={ipfsUrl(metadata.imageCID)} alt="" className="mt-2 max-h-40 rounded border border-slate-700" />
            )}
            {metadata?.sources && metadata.sources.length > 0 && (
              <div>Sources: {metadata.sources.map((s, i) => (
                <a key={i} target="_blank" rel="noreferrer" className="text-brand-500 underline mr-2" href={s}>{i + 1}</a>
              ))}</div>
            )}
          </div>
        )}

        {isPriceMarket && (
          <div className="mt-3 text-xs text-slate-400 border-t border-slate-700 pt-3 font-mono break-all">
            Feed: {market.priceFeed} · Threshold (raw): {market.priceThreshold.toString()}
          </div>
        )}

        <div className="mt-4 grid grid-cols-2 md:grid-cols-4 gap-4 text-sm">
          <div>
            <div className="text-slate-400">YES stake</div>
            <div className="font-mono">{ethers.formatEther(market.totalYesStake)} ETH</div>
          </div>
          <div>
            <div className="text-slate-400">NO stake</div>
            <div className="font-mono">{ethers.formatEther(market.totalNoStake)} ETH</div>
          </div>
          <div>
            <div className="text-slate-400">Creator bond</div>
            <div className="font-mono">{ethers.formatEther(market.creatorBond)} ETH</div>
          </div>
          <div>
            <div className="text-slate-400">Slash pool</div>
            <div className="font-mono">{ethers.formatEther(market.slashPool)} ETH</div>
          </div>
        </div>

        <div className="mt-4 grid md:grid-cols-2 gap-3 text-xs text-slate-400">
          <div>Trading closes: {fmtTimestamp(market.tradingDeadline)}<br/>
               (<span className="text-slate-200">{countdown(market.tradingDeadline, now)}</span>)</div>
          <div>Proposal deadline: {fmtTimestamp(market.proposalDeadline)}<br/>
               (<span className="text-slate-200">{countdown(market.proposalDeadline, now)}</span>)</div>
          {market.disputeDeadline > 0n && (
            <div>Dispute closes: {fmtTimestamp(market.disputeDeadline)}<br/>
                 (<span className="text-slate-200">{countdown(market.disputeDeadline, now)}</span>)</div>
          )}
          {market.voteDeadline > 0n && (
            <div>Vote closes: {fmtTimestamp(market.voteDeadline)}<br/>
                 (<span className="text-slate-200">{countdown(market.voteDeadline, now)}</span>)</div>
          )}
        </div>

        {market.state >= State.Proposed && (
          <div className="mt-4 text-sm text-slate-300 border-t border-slate-700 pt-3">
            <div>Proposed outcome: <strong>{outcomeLabel(market.proposedOutcome)}</strong> by {shortAddr(market.proposer)}</div>
            {market.disputer !== ethers.ZeroAddress && (
              <div className="mt-1">Disputed by: {shortAddr(market.disputer)}</div>
            )}
            {market.state >= State.Disputed && (
              <div className="mt-1 text-xs">
                Vote weights — YES {ethers.formatEther(market.yesVoteWeight)},
                {" "}NO {ethers.formatEther(market.noVoteWeight)},
                {" "}INVALID {ethers.formatEther(market.invalidVoteWeight)}
              </div>
            )}
            {isResolved && (
              <div className="mt-1">Final result: <strong className="text-emerald-400">{outcomeLabel(market.result)}</strong></div>
            )}
          </div>
        )}
      </div>

      {/* Action panels by phase */}
      <div className="grid md:grid-cols-2 gap-4">
        {market.state === State.Trading && now < Number(market.tradingDeadline) && (
          <StakePanel wallet={wallet} marketId={marketId} onDone={refresh} />
        )}
        {canAutoResolve && (
          <div className="card">
            <h3 className="font-semibold mb-2">Auto-resolve via Chainlink</h3>
            <p className="text-sm text-slate-400 mb-3">
              The trading window has closed. Anyone can read the price feed and
              settle the market — no oracle vote needed.
            </p>
            <div className="text-xs text-slate-400 mb-3 font-mono break-all">
              feed: {market.priceFeed}<br/>threshold: {market.priceThreshold.toString()}
            </div>
            <button className="btn-primary w-full" disabled={busy}
                    onClick={() => call("autoResolve", "Auto-resolving")}>
              Auto-resolve
            </button>
          </div>
        )}
        {canPropose && <ProposePanel wallet={wallet} marketId={marketId} onDone={refresh} />}
        {canDispute && <DisputePanel wallet={wallet} marketId={marketId} onDone={refresh} />}
        {canVote    && <VotePanel    wallet={wallet} marketId={marketId} onDone={refresh} />}
        {canFinalize && (
          <div className="card">
            <h3 className="font-semibold mb-2">Ready to finalize</h3>
            <p className="text-sm text-slate-400 mb-3">
              The relevant window has closed. Anyone can settle the market now.
            </p>
            <button className="btn-primary w-full" disabled={busy}
                    onClick={() => call("finalizeMarket", "Finalizing")}>
              Finalize market
            </button>
          </div>
        )}

        {/* Stake summary + claim */}
        {user && (user.yesStake > 0n || user.noStake > 0n) && (
          <div className="card">
            <h3 className="font-semibold mb-2">Your stake</h3>
            <div className="text-sm space-y-1">
              <div>YES: <span className="font-mono">{ethers.formatEther(user.yesStake)} ETH</span></div>
              <div>NO:  <span className="font-mono">{ethers.formatEther(user.noStake)} ETH</span></div>
            </div>
            {isResolved && userStakedOnWinner && !user.claimed && (
              <button className="btn-success w-full mt-3" disabled={busy}
                      onClick={() => call("claimWinnings", "Claiming winnings")}>
                Claim winnings
              </button>
            )}
            {isResolved && user.claimed && (
              <p className="text-xs text-slate-400 mt-2">Already claimed.</p>
            )}
          </div>
        )}

        {/* Proposer bond claim */}
        {proposerCanClaim && (
          <div className="card">
            <h3 className="font-semibold mb-2">Proposer bond</h3>
            <button className="btn-success w-full" disabled={busy}
                    onClick={() => call("claimProposerBond", "Claiming proposer bond")}>
              Claim proposer bond
            </button>
          </div>
        )}

        {/* Disputer bond claim */}
        {disputerCanClaim && (
          <div className="card">
            <h3 className="font-semibold mb-2">Disputer bond</h3>
            <button className="btn-success w-full" disabled={busy}
                    onClick={() => call("claimDisputerBond", "Claiming disputer bond")}>
              Claim disputer bond
            </button>
          </div>
        )}

        {/* Oracle reward claim */}
        {oracleCanClaim && (
          <div className="card">
            <h3 className="font-semibold mb-2">Oracle reward</h3>
            <p className="text-sm text-slate-400 mb-3">
              Your weight {ethers.formatEther(user.voteWeight)} ETH voted with the final outcome.
            </p>
            <button className="btn-success w-full" disabled={busy}
                    onClick={() => call("claimOracleReward", "Claiming oracle reward")}>
              Claim oracle reward
            </button>
          </div>
        )}
      </div>
    </div>
  );
}
