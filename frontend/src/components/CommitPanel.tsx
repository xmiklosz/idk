import { useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import {
  Outcome,
  commitmentHash,
  randomSalt,
  saveCommit,
} from "../utils/commitHash";

const COLLATERAL = ethers.parseEther("0.01");

export default function CommitPanel({
  wallet,
  marketId,
  onDone,
}: {
  wallet: WalletState;
  marketId: bigint;
  onDone: () => void;
}) {
  const [outcome, setOutcome] = useState<Outcome>(Outcome.YES);
  const [busy, setBusy] = useState(false);

  async function submit() {
    if (!wallet.writeContract || !wallet.account) return toast.error("Connect wallet");
    const salt = randomSalt();
    const hash = commitmentHash(outcome, salt);

    setBusy(true);
    const t = toast.loading("Submitting commitment…");
    try {
      const tx = await wallet.writeContract.commitResolution(marketId, hash, {
        value: COLLATERAL,
      });
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      saveCommit(marketId, wallet.account, { outcome, salt, savedAt: Date.now() });
      toast.success(
        "Committed. Salt saved to your browser — keep this device until reveal.",
        { id: t }
      );
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-2">Commit your resolution vote</h3>
      <p className="text-sm text-slate-400 mb-3">
        You stake <strong>{ethers.formatEther(COLLATERAL)} ETH</strong> as collateral.
        Your vote is hidden until the reveal phase. The salt is saved in this browser&apos;s
        localStorage; if you lose it you cannot reveal.
      </p>
      <label className="label">Outcome</label>
      <div className="grid grid-cols-3 gap-2 mb-3">
        {[
          { v: Outcome.YES, label: "YES" },
          { v: Outcome.NO, label: "NO" },
          { v: Outcome.INVALID, label: "INVALID" },
        ].map((o) => (
          <button
            key={o.v}
            type="button"
            onClick={() => setOutcome(o.v)}
            className={`px-3 py-2 rounded-md text-sm ${
              outcome === o.v ? "bg-brand-600 text-white" : "bg-slate-700 text-slate-300"
            }`}
          >
            {o.label}
          </button>
        ))}
      </div>
      <button className="btn-primary w-full" disabled={busy} onClick={submit}>
        {busy ? "Submitting…" : `Commit (${ethers.formatEther(COLLATERAL)} ETH)`}
      </button>
    </div>
  );
}
