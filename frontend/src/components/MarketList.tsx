import { useEffect, useState } from "react";
import { Link } from "react-router-dom";
import { ethers } from "ethers";
import { WalletState } from "../hooks/useContract";
import { State, outcomeLabel, stateLabel } from "../utils/outcome";

interface MarketRow {
  id: bigint;
  question: string;
  tradingDeadline: bigint;
  proposalDeadline: bigint;
  totalYesStake: bigint;
  totalNoStake: bigint;
  state: number;
  result: number;
}

const stateStyle: Record<number, string> = {
  [State.Trading]:  "bg-blue-500/20 text-blue-300",
  [State.Proposed]: "bg-amber-500/20 text-amber-300",
  [State.Disputed]: "bg-rose-500/20 text-rose-300",
  [State.Resolved]: "bg-emerald-500/20 text-emerald-300",
  [State.Expired]:  "bg-slate-500/20 text-slate-300",
};

export default function MarketList({ wallet }: { wallet: WalletState }) {
  const [markets, setMarkets] = useState<MarketRow[]>([]);
  const [filter, setFilter] = useState<"all" | "trading" | "resolving" | "resolved">("all");
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let cancelled = false;
    async function load() {
      setLoading(true);
      try {
        const count: bigint = await wallet.market.marketCount();
        const rows: MarketRow[] = [];
        for (let i = 0n; i < count; i++) {
          const m = await wallet.market.markets(i);
          rows.push({
            id: i,
            question: m.question,
            tradingDeadline: m.tradingDeadline,
            proposalDeadline: m.proposalDeadline,
            totalYesStake: m.totalYesStake,
            totalNoStake: m.totalNoStake,
            state: Number(m.state),
            result: Number(m.result),
          });
        }
        if (!cancelled) setMarkets(rows.reverse());
      } catch (e) { console.error(e); }
      finally { if (!cancelled) setLoading(false); }
    }
    load();
    return () => { cancelled = true; };
  }, [wallet.market]);

  const visible = markets.filter((m) => {
    if (filter === "all") return true;
    if (filter === "trading") return m.state === State.Trading;
    if (filter === "resolving") return m.state === State.Proposed || m.state === State.Disputed;
    return m.state === State.Resolved || m.state === State.Expired;
  });

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-semibold">Markets</h2>
        <div className="flex gap-2 flex-wrap">
          {(["all", "trading", "resolving", "resolved"] as const).map((f) => (
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
        <p className="text-slate-400">
          No markets yet. <Link to="/create" className="text-brand-500 underline">Create one</Link>.
        </p>
      )}

      <div className="grid gap-4 md:grid-cols-2">
        {visible.map((m) => {
          const total = m.totalYesStake + m.totalNoStake;
          const yesPct = total === 0n ? 50 : Number((m.totalYesStake * 100n) / total);
          return (
            <Link key={m.id.toString()} to={`/market/${m.id}`}
                  className="card hover:border-brand-500 transition">
              <div className="flex items-start justify-between gap-2">
                <h3 className="font-semibold leading-snug">{m.question}</h3>
                <span className={`badge ${stateStyle[m.state]}`}>{stateLabel(m.state)}</span>
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
              {(m.state === State.Resolved || m.state === State.Expired) && (
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
