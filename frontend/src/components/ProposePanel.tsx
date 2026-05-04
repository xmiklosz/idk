import { useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import { Outcome } from "../utils/outcome";

const PROPOSAL_BOND = ethers.parseEther("0.05");

export default function ProposePanel({
  wallet, marketId, onDone,
}: { wallet: WalletState; marketId: bigint; onDone: () => void }) {
  const [outcome, setOutcome] = useState<Outcome>(Outcome.YES);
  const [busy, setBusy] = useState(false);

  async function submit() {
    if (!wallet.marketWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Submitting proposal…");
    try {
      const tx = await wallet.marketWrite.proposeOutcome(marketId, outcome, { value: PROPOSAL_BOND });
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      toast.success("Proposal submitted — dispute window now open", { id: t });
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-1">Propose outcome</h3>
      <p className="text-sm text-slate-400 mb-3">
        Anyone can propose. You stake <strong>{ethers.formatEther(PROPOSAL_BOND)} ETH</strong> as a
        bond. If undisputed, you reclaim it. If disputed and you&apos;re right, you also win the
        disputer&apos;s bond.
      </p>
      <label className="label">Outcome</label>
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
      <button className="btn-primary w-full" disabled={busy} onClick={submit}>
        {busy ? "Submitting…" : `Propose (${ethers.formatEther(PROPOSAL_BOND)} ETH bond)`}
      </button>
    </div>
  );
}
