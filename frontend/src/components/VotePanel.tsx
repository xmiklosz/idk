import { useEffect, useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import { Outcome } from "../utils/outcome";

export default function VotePanel({
  wallet, marketId, onDone,
}: { wallet: WalletState; marketId: bigint; onDone: () => void }) {
  const [outcome, setOutcome] = useState<Outcome>(Outcome.YES);
  const [busy, setBusy] = useState(false);
  const [isOracle, setIsOracle] = useState(false);
  const [stake, setStake] = useState<bigint>(0n);
  const [alreadyVoted, setAlreadyVoted] = useState(false);

  useEffect(() => {
    async function load() {
      if (!wallet.account) return;
      const ok = await wallet.registry.isOracle(wallet.account);
      setIsOracle(ok);
      setStake(await wallet.registry.stakeOf(wallet.account));
      const myVote = await wallet.market.oracleVote(marketId, wallet.account);
      setAlreadyVoted(Number(myVote) !== 0);
    }
    load();
  }, [wallet.account, wallet.market, wallet.registry, marketId]);

  async function vote() {
    if (!wallet.marketWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Submitting vote…");
    try {
      const tx = await wallet.marketWrite.voteOnDispute(marketId, outcome);
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      toast.success("Vote recorded", { id: t });
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  if (!isOracle) {
    return (
      <div className="card">
        <h3 className="font-semibold mb-1">Oracle vote in progress</h3>
        <p className="text-sm text-slate-400">
          Only registered oracles can vote on disputes. Visit the
          <a href="/oracles" className="text-brand-500 underline ml-1">Oracles</a> page to register.
        </p>
      </div>
    );
  }

  if (alreadyVoted) {
    return (
      <div className="card">
        <h3 className="font-semibold mb-1">Vote submitted</h3>
        <p className="text-sm text-slate-400">
          Your vote has been recorded. Wait for the vote window to close, then anyone can finalize.
        </p>
      </div>
    );
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-1">Cast your oracle vote</h3>
      <p className="text-sm text-slate-400 mb-3">
        Your stake of <strong>{ethers.formatEther(stake)} ETH</strong> determines your vote
        weight. Vote correctly to share the slash pool; vote incorrectly and your stake is
        slashed in proportion.
      </p>
      <div className="grid grid-cols-3 gap-2 mb-3">
        {[
          { v: Outcome.YES, label: "YES" },
          { v: Outcome.NO,  label: "NO" },
          { v: Outcome.INVALID, label: "INVALID" },
        ].map((o) => (
          <button
            key={o.v}
            type="button"
            onClick={() => setOutcome(o.v)}
            className={`px-3 py-2 rounded-md text-sm ${
              outcome === o.v ? "bg-brand-600 text-white" : "bg-slate-700 text-slate-300"
            }`}
          >{o.label}</button>
        ))}
      </div>
      <button className="btn-primary w-full" disabled={busy} onClick={vote}>
        {busy ? "Submitting…" : "Vote"}
      </button>
    </div>
  );
}
