import { useCallback, useEffect, useState } from "react";
import { useParams, Link } from "react-router-dom";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import StakePanel from "./StakePanel";
import CommitPanel from "./CommitPanel";
import RevealPanel from "./RevealPanel";
import { Outcome, outcomeLabel } from "../utils/commitHash";

interface FullMarket {
  creator: string;
  question: string;
  resolutionTime: bigint;
  commitDeadline: bigint;
  revealDeadline: bigint;
  totalYesStake: bigint;
  totalNoStake: bigint;
  creatorBond: bigint;
  resolved: boolean;
  result: number;
  quorum: bigint;
  yesVotes: bigint;
  noVotes: bigint;
  invalidVotes: bigint;
  slashPool: bigint;
  winningResolverCount: bigint;
}

interface UserState {
  yesStake: bigint;
  noStake: bigint;
  claimed: boolean;
  resolverCommitted: boolean;
  resolverRevealed: boolean;
  resolverRewardClaimed: boolean;
  resolverVote: number;
}

function fmtTimestamp(t: bigint): string {
  return new Date(Number(t) * 1000).toLocaleString();
}

function countdown(target: bigint, now: number): string {
  const s = Number(target) - now;
  if (s <= 0) return "now";
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${h}h ${m}m ${sec}s`;
}

type Phase = "Staking" | "Committing" | "Revealing" | "Finalizable" | "Resolved";
function phaseOf(m: FullMarket, now: number): Phase {
  if (m.resolved) return "Resolved";
  if (now < Number(m.resolutionTime)) return "Staking";
  if (now < Number(m.commitDeadline)) return "Committing";
  if (now < Number(m.revealDeadline)) return "Revealing";
  return "Finalizable";
}

export default function MarketDetail({ wallet }: { wallet: WalletState }) {
  const { id } = useParams<{ id: string }>();
  const marketId = BigInt(id ?? "0");

  const [market, setMarket] = useState<FullMarket | null>(null);
  const [user, setUser] = useState<UserState | null>(null);
  const [now, setNow] = useState(Math.floor(Date.now() / 1000));
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    const i = setInterval(() => setNow(Math.floor(Date.now() / 1000)), 1000);
    return () => clearInterval(i);
  }, []);

  const refresh = useCallback(async () => {
    try {
      const m = await wallet.readContract.markets(marketId);
      setMarket({
        creator: m.creator,
        question: m.question,
        resolutionTime: m.resolutionTime,
        commitDeadline: m.commitDeadline,
        revealDeadline: m.revealDeadline,
        totalYesStake: m.totalYesStake,
        totalNoStake: m.totalNoStake,
        creatorBond: m.creatorBond,
        resolved: m.resolved,
        result: Number(m.result),
        quorum: m.quorum,
        yesVotes: m.yesVotes,
        noVotes: m.noVotes,
        invalidVotes: m.invalidVotes,
        slashPool: m.slashPool,
        winningResolverCount: m.winningResolverCount,
      });
      if (wallet.account) {
        const [y, n, c, ri] = await Promise.all([
          wallet.readContract.yesStakes(marketId, wallet.account),
          wallet.readContract.noStakes(marketId, wallet.account),
          wallet.readContract.claimed(marketId, wallet.account),
          wallet.readContract.resolverInfo(marketId, wallet.account),
        ]);
        setUser({
          yesStake: y,
          noStake: n,
          claimed: c,
          resolverCommitted: ri.committed,
          resolverRevealed: ri.revealed,
          resolverRewardClaimed: ri.rewardClaimed,
          resolverVote: Number(ri.revealedVote),
        });
      } else {
        setUser(null);
      }
    } catch (e) {
      console.error(e);
    }
  }, [marketId, wallet.readContract, wallet.account]);

  useEffect(() => { refresh(); }, [refresh]);

  async function finalize() {
    if (!wallet.writeContract) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Finalizing market…");
    try {
      const tx = await wallet.writeContract.finalizeMarket(marketId);
      await tx.wait();
      toast.success("Market finalized", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  async function claim() {
    if (!wallet.writeContract) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Claiming winnings…");
    try {
      const tx = await wallet.writeContract.claimWinnings(marketId);
      await tx.wait();
      toast.success("Winnings claimed", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  async function claimResolver() {
    if (!wallet.writeContract) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Claiming resolver reward…");
    try {
      const tx = await wallet.writeContract.claimResolverReward(marketId);
      await tx.wait();
      toast.success("Resolver reward claimed", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  if (!market) return <p className="text-slate-400">Loading market…</p>;

  const phase = phaseOf(market, now);
  const total = market.totalYesStake + market.totalNoStake;
  const yesPct = total === 0n ? 50 : Number((market.totalYesStake * 100n) / total);

  const userStakedOnWinner =
    user &&
    market.resolved &&
    ((market.result === Outcome.YES && user.yesStake > 0n) ||
     (market.result === Outcome.NO && user.noStake > 0n) ||
     (market.result === Outcome.INVALID && (user.yesStake + user.noStake) > 0n));
  const canClaimWin = userStakedOnWinner && user && !user.claimed;

  const isWinningResolver =
    user &&
    market.resolved &&
    user.resolverRevealed &&
    !user.resolverRewardClaimed &&
    (
      (market.result === Outcome.INVALID && market.winningResolverCount === 0n) ||
      user.resolverVote === market.result
    );

  return (
    <div className="space-y-6">
      <div>
        <Link to="/" className="text-brand-500 text-sm">&larr; Back to markets</Link>
      </div>

      <div className="card">
        <div className="flex items-start justify-between gap-3">
          <div>
            <h2 className="text-xl font-semibold leading-snug">{market.question}</h2>
            <div className="text-sm text-slate-400 mt-1">Market #{marketId.toString()} • Phase: {phase}</div>
          </div>
          {market.resolved && (
            <span className="badge bg-emerald-500/20 text-emerald-300">
              {outcomeLabel(market.result)}
            </span>
          )}
        </div>

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
            <div className="text-slate-400">Quorum</div>
            <div className="font-mono">{market.quorum.toString()}</div>
          </div>
        </div>

        <div className="mt-4">
          <div className="flex justify-between text-xs text-slate-300 mb-1">
            <span>YES {yesPct}%</span><span>NO {100 - yesPct}%</span>
          </div>
          <div className="h-2 rounded bg-slate-700 overflow-hidden">
            <div className="h-full bg-emerald-500" style={{ width: `${yesPct}%` }} />
          </div>
        </div>

        <div className="mt-4 grid grid-cols-1 md:grid-cols-3 gap-3 text-xs text-slate-400">
          <div>Stake closes: {fmtTimestamp(market.resolutionTime)}<br/>(<span className="text-slate-200">{countdown(market.resolutionTime, now)}</span>)</div>
          <div>Commit closes: {fmtTimestamp(market.commitDeadline)}<br/>(<span className="text-slate-200">{countdown(market.commitDeadline, now)}</span>)</div>
          <div>Reveal closes: {fmtTimestamp(market.revealDeadline)}<br/>(<span className="text-slate-200">{countdown(market.revealDeadline, now)}</span>)</div>
        </div>

        {market.resolved && (
          <div className="mt-4 text-sm text-slate-300">
            Reveal tally — YES: {market.yesVotes.toString()}, NO: {market.noVotes.toString()}, INVALID: {market.invalidVotes.toString()}
            <br/>Slash pool: {ethers.formatEther(market.slashPool)} ETH • Winning resolvers: {market.winningResolverCount.toString()}
          </div>
        )}
      </div>

      {/* Action panel by phase */}
      <div className="grid md:grid-cols-2 gap-4">
        {phase === "Staking" && (
          <StakePanel wallet={wallet} marketId={marketId} onDone={refresh} />
        )}
        {phase === "Committing" && !user?.resolverCommitted && (
          <CommitPanel wallet={wallet} marketId={marketId} onDone={refresh} />
        )}
        {phase === "Committing" && user?.resolverCommitted && (
          <div className="card">
            <h3 className="font-semibold mb-2">Commitment submitted</h3>
            <p className="text-sm text-slate-400">
              Wait for the reveal phase to disclose your vote.
            </p>
          </div>
        )}
        {phase === "Revealing" && user?.resolverCommitted && !user?.resolverRevealed && (
          <RevealPanel wallet={wallet} marketId={marketId} onDone={refresh} />
        )}
        {phase === "Revealing" && user?.resolverRevealed && (
          <div className="card">
            <h3 className="font-semibold mb-2">Vote revealed</h3>
            <p className="text-sm text-slate-400">
              Your vote is recorded. Wait for the reveal window to close, then anyone can finalize the market.
            </p>
          </div>
        )}
        {phase === "Finalizable" && (
          <div className="card">
            <h3 className="font-semibold mb-2">Ready to finalize</h3>
            <p className="text-sm text-slate-400 mb-3">
              Reveal window is over. Anyone may now tally the votes.
            </p>
            <button className="btn-primary w-full" disabled={busy} onClick={finalize}>
              Finalize market
            </button>
          </div>
        )}

        {/* Your stake summary always visible */}
        {user && (user.yesStake > 0n || user.noStake > 0n) && (
          <div className="card">
            <h3 className="font-semibold mb-2">Your stake</h3>
            <div className="text-sm space-y-1">
              <div>YES: <span className="font-mono">{ethers.formatEther(user.yesStake)} ETH</span></div>
              <div>NO:  <span className="font-mono">{ethers.formatEther(user.noStake)} ETH</span></div>
            </div>
            {market.resolved && canClaimWin && (
              <button className="btn-success w-full mt-3" disabled={busy} onClick={claim}>
                Claim winnings
              </button>
            )}
            {market.resolved && user.claimed && (
              <p className="text-xs text-slate-400 mt-2">Already claimed.</p>
            )}
          </div>
        )}

        {/* Resolver reward */}
        {user?.resolverCommitted && market.resolved && (
          <div className="card">
            <h3 className="font-semibold mb-2">Resolver reward</h3>
            {user.resolverRevealed ? (
              <div className="text-sm space-y-1">
                <div>You revealed <strong>{outcomeLabel(user.resolverVote)}</strong></div>
                <div>Market result: <strong>{outcomeLabel(market.result)}</strong></div>
              </div>
            ) : (
              <p className="text-sm text-rose-400">You did not reveal — collateral slashed.</p>
            )}
            {isWinningResolver && (
              <button className="btn-success w-full mt-3" disabled={busy} onClick={claimResolver}>
                Claim resolver reward
              </button>
            )}
            {user.resolverRewardClaimed && (
              <p className="text-xs text-slate-400 mt-2">Already claimed.</p>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
