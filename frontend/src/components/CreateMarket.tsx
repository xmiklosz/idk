import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";

const toUnix = (local: string) => Math.floor(new Date(local).getTime() / 1000);

export default function CreateMarket({ wallet }: { wallet: WalletState }) {
  const nav = useNavigate();
  const [question, setQuestion] = useState("");
  const [tradingLocal, setTradingLocal] = useState("");
  const [proposalMinutes, setProposalMinutes] = useState(120);
  const [bond, setBond] = useState("0.01");
  const [busy, setBusy] = useState(false);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    if (!wallet.marketWrite) return toast.error("Connect your wallet first");
    if (!wallet.isCorrectChain) return toast.error("Wrong network");

    const tradingDeadline = toUnix(tradingLocal);
    if (!tradingDeadline || tradingDeadline <= Math.floor(Date.now() / 1000)) {
      return toast.error("Trading deadline must be in the future");
    }
    const proposalDeadline = tradingDeadline + proposalMinutes * 60;

    setBusy(true);
    const t = toast.loading("Submitting transaction…");
    try {
      const tx = await wallet.marketWrite.createMarket(
        question,
        tradingDeadline,
        proposalDeadline,
        { value: ethers.parseEther(bond) }
      );
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      const newCount: bigint = await wallet.market.marketCount();
      const newId = newCount - 1n;
      toast.success(`Market #${newId} created`, { id: t });
      nav(`/market/${newId}`);
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
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
          <label className="label">Trading deadline (when staking closes)</label>
          <input
            type="datetime-local"
            className="input"
            value={tradingLocal}
            onChange={(e) => setTradingLocal(e.target.value)}
            required
          />
        </div>
        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="label">Proposal window (minutes)</label>
            <input
              type="number"
              min={10}
              className="input"
              value={proposalMinutes}
              onChange={(e) => setProposalMinutes(Number(e.target.value))}
            />
            <p className="text-xs text-slate-500 mt-1">
              Time after trading closes within which an oracle must propose an outcome.
            </p>
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
            <p className="text-xs text-slate-500 mt-1">
              Distributed pro-rata to winning stakers.
            </p>
          </div>
        </div>
        <div className="text-xs text-slate-400 bg-slate-900 border border-slate-700 rounded-md p-3">
          Resolution flow: <strong>Trading → Proposed</strong> (proposer posts 0.05 ETH bond) <strong>→ optional Disputed</strong> (disputer posts 0.05 ETH) <strong>→ stake-weighted oracle vote → Resolved</strong>. Dispute window 1h, vote window 24h.
        </div>
        <button type="submit" className="btn-primary w-full" disabled={busy}>
          {busy ? "Submitting…" : "Create market"}
        </button>
      </form>
    </div>
  );
}
