import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";

function toUnix(local: string): number {
  return Math.floor(new Date(local).getTime() / 1000);
}

export default function CreateMarket({ wallet }: { wallet: WalletState }) {
  const nav = useNavigate();
  const [question, setQuestion] = useState("");
  const [resolutionLocal, setResolutionLocal] = useState("");
  const [commitMinutes, setCommitMinutes] = useState(60);
  const [revealMinutes, setRevealMinutes] = useState(60);
  const [quorum, setQuorum] = useState(3);
  const [bond, setBond] = useState("0.01");
  const [busy, setBusy] = useState(false);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    if (!wallet.writeContract) {
      toast.error("Connect your wallet first");
      return;
    }
    if (!wallet.isCorrectChain) {
      toast.error("Wrong network");
      return;
    }
    const resolutionTime = toUnix(resolutionLocal);
    if (!resolutionTime || resolutionTime <= Math.floor(Date.now() / 1000)) {
      toast.error("Resolution time must be in the future");
      return;
    }
    const commitDeadline = resolutionTime + commitMinutes * 60;
    const revealDeadline = commitDeadline + revealMinutes * 60;

    setBusy(true);
    const t = toast.loading("Submitting transaction…");
    try {
      const tx = await wallet.writeContract.createMarket(
        question,
        resolutionTime,
        commitDeadline,
        revealDeadline,
        quorum,
        { value: ethers.parseEther(bond) }
      );
      toast.loading("Waiting for confirmation…", { id: t });
      const receipt = await tx.wait();
      // marketId = old marketCount; read after.
      const newCount: bigint = await wallet.readContract.marketCount();
      const newId = newCount - 1n;
      toast.success(`Market #${newId} created`, { id: t });
      nav(`/market/${newId}`);
    } catch (err: any) {
      console.error(err);
      toast.error(err?.shortMessage || err?.reason || err?.message || "Transaction failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="max-w-xl mx-auto">
      <h2 className="text-2xl font-semibold mb-4">Create a market</h2>
      <form onSubmit={submit} className="card space-y-4">
        <div>
          <label className="label">Question</label>
          <input
            className="input"
            placeholder="Will ETH be above $3000 on June 1?"
            value={question}
            onChange={(e) => setQuestion(e.target.value)}
            required
          />
        </div>
        <div>
          <label className="label">Resolution time (when staking closes)</label>
          <input
            type="datetime-local"
            className="input"
            value={resolutionLocal}
            onChange={(e) => setResolutionLocal(e.target.value)}
            required
          />
        </div>
        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="label">Commit window (minutes)</label>
            <input
              type="number"
              min={1}
              className="input"
              value={commitMinutes}
              onChange={(e) => setCommitMinutes(Number(e.target.value))}
            />
          </div>
          <div>
            <label className="label">Reveal window (minutes)</label>
            <input
              type="number"
              min={1}
              className="input"
              value={revealMinutes}
              onChange={(e) => setRevealMinutes(Number(e.target.value))}
            />
          </div>
        </div>
        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="label">Quorum (min resolvers)</label>
            <input
              type="number"
              min={1}
              className="input"
              value={quorum}
              onChange={(e) => setQuorum(Number(e.target.value))}
            />
          </div>
          <div>
            <label className="label">Creator bond (ETH)</label>
            <input
              type="text"
              className="input"
              value={bond}
              onChange={(e) => setBond(e.target.value)}
              required
            />
          </div>
        </div>
        <button type="submit" className="btn-primary w-full" disabled={busy}>
          {busy ? "Submitting…" : "Create market"}
        </button>
      </form>
    </div>
  );
}
