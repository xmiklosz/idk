import { useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";

const DISPUTE_BOND = ethers.parseEther("0.05");

export default function DisputePanel({
  wallet, marketId, onDone,
}: { wallet: WalletState; marketId: bigint; onDone: () => void }) {
  const [busy, setBusy] = useState(false);

  async function submit() {
    if (!wallet.marketWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Disputing proposal…");
    try {
      const tx = await wallet.marketWrite.disputeProposal(marketId, { value: DISPUTE_BOND });
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      toast.success("Dispute filed — oracle vote is now open", { id: t });
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-1">Dispute the proposal</h3>
      <p className="text-sm text-slate-400 mb-3">
        Think the proposer is wrong? Post a <strong>{ethers.formatEther(DISPUTE_BOND)} ETH</strong>
        {" "}bond and escalate to the oracle network. If the oracles back you, you win
        the proposer&apos;s bond too.
      </p>
      <button className="btn-danger w-full" disabled={busy} onClick={submit}>
        {busy ? "Submitting…" : `Dispute (${ethers.formatEther(DISPUTE_BOND)} ETH bond)`}
      </button>
    </div>
  );
}
