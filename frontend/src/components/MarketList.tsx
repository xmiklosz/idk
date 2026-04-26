import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { ethers } from "ethers";
import { WalletState } from "../hooks/useContract";
import { outcomeLabel } from "../utils/commitHash";

interface MarketRow {
  id: bigint;
  question: string;
  resolutionTime: bigint;
  commitDeadline: bigint;
  revealDeadline: bigint;
  totalYesStake: bigint;
  totalNoStake: bigint;
  resolved: boolean;
  result: number;
}

type Phase = "Staking" | "Committing" | "Revealing" | "Finalizable" | "Resolved";

function phaseOf(m: MarketRow, now: number): Phase {
  if (m.resolved) return "Resolved";
  if (now < Number(m.resolutionTime)) return "Staking";
  if (now < Number(m.commitDeadline)) return "Committing";
  if (now < Number(m.revealDeadline)) return "Revealing";
  return "Finalizable";
}

const phaseStyle: Record<Phase, string> = {
  Staking:      "bg-blue-500/20 text-blue-300",
  Committing:   "bg-amber-500/20 text-amber-300",
  Revealing:    "bg-purple-500/20 text-purple-300",
  Finalizable:  "bg-orange-500/20 text-orange-300",
  Resolved:     "bg-emerald-500/20 text-emerald-300",
};

export default function MarketList({ wallet }: { wallet: WalletState }) {
  const [markets, setMarkets] = useState<MarketRow[]>([]);
  const [filter, setFilter] = useState<"all" | "active" | "resolving" | "resolved">("all");
  const [loading, setLoading] = useState(true);
  const [now, setNow] = useState(Math.floor(Date.now() / 1000));

  useEffect(() => {
    const i = setInterval(() => setNow(Math.floor(Date.now() / 1000)), 1000);
    return () => clearInterval(i);
  }, []);

  useEffect(() => {
    let cancelled = false;
    async function load() {
      setLoading(true);
      try {
        const count: bigint = await wallet.readContract.marketCount();
        const rows: MarketRow[] = [];
        for (let i = 0n; i < count; i++) {
          const m = await wallet.readContract.markets(i);
          rows.push({
            id: i,
            question: m.question,
            resolutionTime: m.resolutionTime,
            commitDeadline: m.commitDeadline,
            revealDeadline: m.revealDeadline,
            totalYesStake: m.totalYesStake,
            totalNoStake: m.totalNoStake,
            resolved: m.resolved,
            result: Number(m.result),
          });
        }
        if (!cancelled) setMarkets(rows.reverse());
      } catch (e) {
        console.error(e);
      } finally {
        if (!cancelled) setLoading(false);
      }
    }
    load();
    return () => { cancelled = true; };
  }, [wallet.readContract]);

  const visible = markets.filter((m) => {
    const p = phaseOf(m, now);
    if (filter === "all") return true;
    if (filter === "active") return p === "Staking";
    if (filter === "resolving") return p === "Committing" || p === "Revealing" || p === "Finalizable";
    return p === "Resolved";
  });

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-semibold">Markets</h2>
        <div className="flex gap-2">
          {(["all", "active", "resolving", "resolved"] as const).map((f) => (
            <button
              key={f}
              onClick={() => setFilter(f)}
              className={`px-3 py-1.5 rounded-md text-sm capitalize ${
                filter === f ? "bg-brand-600 text-white" : "bg-slate-800 text-slate-300 hover:bg-slate-700"
              }`}
            >
              {f}
            </button>
          ))}
        </div>
      </div>

      {loading && <p className="text-slate-400">Loading markets…</p>}
      {!loading && visible.length === 0 && (
        <p className="text-slate-400">No markets yet. <Link to="/create" className="text-brand-500 underline">Create one</Link>.</p>
      )}

      <div className="grid gap-4 md:grid-cols-2">
        {visible.map((m) => {
          const p = phaseOf(m, now);
          const total = m.totalYesStake + m.totalNoStake;
          const yesPct = total === 0n ? 50 : Number((m.totalYesStake * 100n) / total);
          return (
            <Link
              key={m.id.toString()}
              to={`/market/${m.id}`}
              className="card hover:border-brand-500 transition"
            >
              <div className="flex items-start justify-between gap-2">
                <h3 className="font-semibold leading-snug">{m.question}</h3>
                <span className={`badge ${phaseStyle[p]}`}>{p}</span>
              </div>
              <div className="mt-3 text-sm text-slate-400">Market #{m.id.toString()}</div>

              <div className="mt-3">
                <div className="flex justify-between text-xs text-slate-300 mb-1">
                  <span>YES {yesPct}%</span><span>NO {100 - yesPct}%</span>
                </div>
                <div className="h-2 rounded bg-slate-700 overflow-hidden">
                  <div className="h-full bg-emerald-500" style={{ width: `${yesPct}%` }} />
                </div>
              </div>

              <div className="mt-3 text-xs text-slate-400">
                Total staked: {ethers.formatEther(total)} ETH
              </div>
              {m.resolved && (
                <div className="mt-2 text-sm">
                  Resolved as <strong className="text-emerald-400">{outcomeLabel(m.result)}</strong>
                </div>
              )}
            </Link>
          );
        })}
      </div>
    </div>
  );
}
