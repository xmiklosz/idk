import { useEffect, useState } from "react";
import { ethers } from "ethers";
import { WalletState } from "../hooks/useContract";
import { shortAddr } from "../utils/outcome";

interface StakerRow { address: string; total: bigint; markets: number; }
interface OracleRow { address: string; stake: bigint; }

export default function Leaderboard({ wallet }: { wallet: WalletState }) {
  const [stakers, setStakers] = useState<StakerRow[]>([]);
  const [oracles, setOracles] = useState<OracleRow[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let cancelled = false;
    async function load() {
      setLoading(true);
      try {
        // Top stakers — derived by replaying the Staked event log.
        const filter = wallet.market.filters.Staked();
        const events = await wallet.market.queryFilter(filter, 0);
        const totals = new Map<string, { total: bigint; markets: Set<string> }>();
        for (const e of events) {
          const args = (e as any).args;
          if (!args) continue;
          const addr = (args.user as string).toLowerCase();
          const amt  = args.amount as bigint;
          const mid  = (args.marketId as bigint).toString();
          const cur  = totals.get(addr) ?? { total: 0n, markets: new Set<string>() };
          cur.total += amt;
          cur.markets.add(mid);
          totals.set(addr, cur);
        }
        const stakerRows: StakerRow[] = Array.from(totals.entries())
          .map(([address, v]) => ({ address, total: v.total, markets: v.markets.size }))
          .sort((a, b) => (a.total < b.total ? 1 : a.total > b.total ? -1 : 0))
          .slice(0, 10);

        // Top oracles — by current stake in the registry.
        const list: string[] = await wallet.registry.getOracles();
        const oracleRows: OracleRow[] = await Promise.all(
          list.map(async (a) => ({ address: a, stake: await wallet.registry.stakeOf(a) }))
        );
        oracleRows.sort((a, b) => (a.stake < b.stake ? 1 : a.stake > b.stake ? -1 : 0));

        if (!cancelled) {
          setStakers(stakerRows);
          setOracles(oracleRows.slice(0, 10));
        }
      } catch (e) {
        console.error(e);
      } finally {
        if (!cancelled) setLoading(false);
      }
    }
    load();
    return () => { cancelled = true; };
  }, [wallet.market, wallet.registry]);

  return (
    <div className="space-y-6">
      <h2 className="text-2xl font-semibold">Leaderboard</h2>
      {loading && <p className="text-slate-400">Loading…</p>}

      <div className="grid md:grid-cols-2 gap-6">
        <section>
          <h3 className="font-semibold mb-3">Top stakers</h3>
          <div className="card p-0 divide-y divide-slate-700">
            {stakers.length === 0 && (
              <div className="p-4 text-slate-400 text-sm">No stakes yet.</div>
            )}
            {stakers.map((s, i) => (
              <div key={s.address} className="p-3 flex items-center justify-between text-sm">
                <span className="flex items-center gap-3">
                  <span className="badge bg-slate-700 text-slate-200 w-6 text-center">{i + 1}</span>
                  <span className="font-mono">{shortAddr(s.address)}</span>
                </span>
                <span className="text-right">
                  <div className="font-mono">{ethers.formatEther(s.total)} ETH</div>
                  <div className="text-xs text-slate-400">{s.markets} market{s.markets === 1 ? "" : "s"}</div>
                </span>
              </div>
            ))}
          </div>
        </section>

        <section>
          <h3 className="font-semibold mb-3">Top oracles by stake</h3>
          <div className="card p-0 divide-y divide-slate-700">
            {oracles.length === 0 && (
              <div className="p-4 text-slate-400 text-sm">No oracles registered yet.</div>
            )}
            {oracles.map((o, i) => (
              <div key={o.address} className="p-3 flex items-center justify-between text-sm">
                <span className="flex items-center gap-3">
                  <span className="badge bg-slate-700 text-slate-200 w-6 text-center">{i + 1}</span>
                  <span className="font-mono">{shortAddr(o.address)}</span>
                </span>
                <span className="font-mono">{ethers.formatEther(o.stake)} ETH</span>
              </div>
            ))}
          </div>
        </section>
      </div>
    </div>
  );
}
