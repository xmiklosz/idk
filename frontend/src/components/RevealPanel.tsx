import { useEffect, useState } from "react";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import {
  Outcome,
  loadCommit,
  outcomeLabel,
  SavedCommit,
} from "../utils/commitHash";

export default function RevealPanel({
  wallet,
  marketId,
  onDone,
}: {
  wallet: WalletState;
  marketId: bigint;
  onDone: () => void;
}) {
  const [saved, setSaved] = useState<SavedCommit | null>(null);
  const [manualOutcome, setManualOutcome] = useState<Outcome>(Outcome.YES);
  const [manualSalt, setManualSalt] = useState("");
  const [busy, setBusy] = useState(false);

  useEffect(() => {
    if (wallet.account) {
      setSaved(loadCommit(marketId, wallet.account));
    }
  }, [wallet.account, marketId]);

  async function reveal(outcome: Outcome, salt: string) {
    if (!wallet.writeContract) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Revealing…");
    try {
      const tx = await wallet.writeContract.revealResolution(marketId, outcome, salt);
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      toast.success(`Revealed as ${outcomeLabel(outcome)}`, { id: t });
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-2">Reveal your vote</h3>
      {saved ? (
        <div className="space-y-3">
          <p className="text-sm text-slate-400">
            Found saved commitment in this browser:
          </p>
          <div className="bg-slate-900 border border-slate-700 rounded-md p-3 text-sm">
            <div>Outcome: <strong>{outcomeLabel(saved.outcome)}</strong></div>
            <div className="font-mono break-all text-xs text-slate-400">salt: {saved.salt}</div>
          </div>
          <button
            className="btn-primary w-full"
            disabled={busy}
            onClick={() => reveal(saved.outcome, saved.salt)}
          >
            {busy ? "Revealing…" : "Reveal"}
          </button>
        </div>
      ) : (
        <div className="space-y-3">
          <p className="text-sm text-slate-400">
            No saved commitment found in this browser. Enter your outcome and salt manually.
          </p>
          <div>
            <label className="label">Outcome</label>
            <div className="grid grid-cols-3 gap-2">
              {[
                { v: Outcome.YES, label: "YES" },
                { v: Outcome.NO, label: "NO" },
                { v: Outcome.INVALID, label: "INVALID" },
              ].map((o) => (
                <button
                  key={o.v}
                  type="button"
                  onClick={() => setManualOutcome(o.v)}
                  className={`px-3 py-2 rounded-md text-sm ${
                    manualOutcome === o.v ? "bg-brand-600 text-white" : "bg-slate-700"
                  }`}
                >
                  {o.label}
                </button>
              ))}
            </div>
          </div>
          <div>
            <label className="label">Salt (0x… 32 bytes)</label>
            <input
              className="input font-mono text-xs"
              value={manualSalt}
              onChange={(e) => setManualSalt(e.target.value)}
              placeholder="0x…"
            />
          </div>
          <button
            className="btn-primary w-full"
            disabled={busy || !manualSalt}
            onClick={() => reveal(manualOutcome, manualSalt)}
          >
            {busy ? "Revealing…" : "Reveal"}
          </button>
        </div>
      )}
    </div>
  );
}
