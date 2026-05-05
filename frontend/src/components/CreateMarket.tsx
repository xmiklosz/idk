import { useState } from "react";
import { useNavigate } from "react-router-dom";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import { feedsForChain } from "../utils/priceFeeds";
import { isProbablyCID, ipfsUrl } from "../utils/ipfs";

const toUnix = (local: string) => Math.floor(new Date(local).getTime() / 1000);

type Mode = "manual" | "price";

export default function CreateMarket({ wallet }: { wallet: WalletState }) {
  const nav = useNavigate();
  const [mode, setMode] = useState<Mode>("manual");
  const [question, setQuestion] = useState("");
  const [metadataCID, setMetadataCID] = useState("");
  const [tradingLocal, setTradingLocal] = useState("");
  const [proposalMinutes, setProposalMinutes] = useState(120);
  const [bond, setBond] = useState("0.01");

  const [feedAddress, setFeedAddress] = useState("");
  const [thresholdHuman, setThresholdHuman] = useState("3000");
  const [feedDecimals, setFeedDecimals] = useState(8);

  const [busy, setBusy] = useState(false);

  const presets = feedsForChain(wallet.chainId);

  async function submit(e: React.FormEvent) {
    e.preventDefault();
    if (!wallet.marketWrite) return toast.error("Connect your wallet first");
    if (!wallet.isCorrectChain) return toast.error("Wrong network");

    const tradingDeadline = toUnix(tradingLocal);
    if (!tradingDeadline || tradingDeadline <= Math.floor(Date.now() / 1000)) {
      return toast.error("Trading deadline must be in the future");
    }
    const proposalDeadline = tradingDeadline + proposalMinutes * 60;

    if (metadataCID && !isProbablyCID(metadataCID.trim())) {
      if (!confirm("That doesn't look like an IPFS CID — submit anyway?")) return;
    }

    setBusy(true);
    const t = toast.loading("Submitting transaction…");
    try {
      let tx;
      if (mode === "manual") {
        tx = await wallet.marketWrite.createMarket(
          question, metadataCID.trim(),
          tradingDeadline, proposalDeadline,
          { value: ethers.parseEther(bond) }
        );
      } else {
        if (!ethers.isAddress(feedAddress)) {
          toast.error("Invalid feed address", { id: t });
          setBusy(false);
          return;
        }
        const threshold = ethers.parseUnits(thresholdHuman, feedDecimals);
        tx = await wallet.marketWrite.createPriceMarket(
          question, metadataCID.trim(),
          tradingDeadline, proposalDeadline,
          feedAddress, threshold,
          { value: ethers.parseEther(bond) }
        );
      }
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

      <div className="card mb-4">
        <div className="flex gap-2">
          <button
            type="button"
            className={`flex-1 px-3 py-2 rounded-md text-sm ${mode === "manual" ? "bg-brand-600 text-white" : "bg-slate-700 text-slate-300"}`}
            onClick={() => setMode("manual")}
          >Optimistic Oracle</button>
          <button
            type="button"
            className={`flex-1 px-3 py-2 rounded-md text-sm ${mode === "price" ? "bg-brand-600 text-white" : "bg-slate-700 text-slate-300"}`}
            onClick={() => setMode("price")}
          >Chainlink Auto-resolve</button>
        </div>
        <p className="text-xs text-slate-400 mt-2">
          {mode === "manual"
            ? "Resolved by anyone proposing an outcome (with a bond), with optional dispute that escalates to the oracle network."
            : "Resolved trustlessly by reading a Chainlink price feed at trading-close."}
        </p>
      </div>

      <form onSubmit={submit} className="card space-y-4">
        <div>
          <label className="label">Question</label>
          <input className="input"
            placeholder={mode === "price" ? "Will ETH > $3000 at trading close?" : "Will the EU pass a CBDC bill in 2026?"}
            value={question}
            onChange={(e) => setQuestion(e.target.value)}
            required />
        </div>

        <div>
          <label className="label">Metadata CID (IPFS, optional)</label>
          <input className="input font-mono text-xs"
            placeholder="bafy… or Qm…"
            value={metadataCID}
            onChange={(e) => setMetadataCID(e.target.value)} />
          <p className="text-xs text-slate-500 mt-1">
            Pin a JSON file (description, image, sources) to IPFS via Pinata or
            web3.storage and paste its CID here.
            {metadataCID && isProbablyCID(metadataCID.trim()) && (
              <> · <a className="text-brand-500 underline" target="_blank" rel="noreferrer" href={ipfsUrl(metadataCID.trim())}>preview</a></>
            )}
          </p>
        </div>

        <div>
          <label className="label">Trading deadline (when staking closes)</label>
          <input type="datetime-local" className="input"
            value={tradingLocal} onChange={(e) => setTradingLocal(e.target.value)} required />
        </div>

        <div className="grid grid-cols-2 gap-3">
          <div>
            <label className="label">Proposal window (minutes)</label>
            <input type="number" min={10} className="input"
              value={proposalMinutes} onChange={(e) => setProposalMinutes(Number(e.target.value))} />
            <p className="text-xs text-slate-500 mt-1">
              {mode === "manual"
                ? "How long after trading closes a proposer must step up."
                : "Fallback expiry if the price feed is unavailable."}
            </p>
          </div>
          <div>
            <label className="label">Creator bond (ETH)</label>
            <input type="text" className="input" value={bond} onChange={(e) => setBond(e.target.value)} required />
            <p className="text-xs text-slate-500 mt-1">Distributed pro-rata to winning stakers.</p>
          </div>
        </div>

        {mode === "price" && (
          <div className="bg-slate-900 border border-slate-700 rounded-md p-3 space-y-3">
            <div>
              <label className="label">Chainlink price feed</label>
              <select
                className="input"
                value={feedAddress}
                onChange={(e) => {
                  setFeedAddress(e.target.value);
                  const f = presets.find((p) => p.address === e.target.value);
                  if (f) setFeedDecimals(f.decimals);
                }}
              >
                <option value="">— select preset —</option>
                {presets.map((f) => (
                  <option key={f.address} value={f.address}>{f.pair} — {f.address.slice(0, 8)}…</option>
                ))}
              </select>
              <input
                className="input mt-2 font-mono text-xs"
                placeholder="…or paste any AggregatorV3 address"
                value={feedAddress}
                onChange={(e) => setFeedAddress(e.target.value)}
              />
            </div>
            <div className="grid grid-cols-2 gap-3">
              <div>
                <label className="label">Threshold (human-readable)</label>
                <input className="input" value={thresholdHuman}
                  onChange={(e) => setThresholdHuman(e.target.value)} />
                <p className="text-xs text-slate-500 mt-1">
                  YES if feed price &gt; threshold at trading-close. e.g. 3000 for ETH/USD.
                </p>
              </div>
              <div>
                <label className="label">Feed decimals</label>
                <input type="number" className="input" min={0} max={18}
                  value={feedDecimals} onChange={(e) => setFeedDecimals(Number(e.target.value))} />
              </div>
            </div>
          </div>
        )}

        <button type="submit" className="btn-primary w-full" disabled={busy}>
          {busy ? "Submitting…" : "Create market"}
        </button>
      </form>
    </div>
  );
}
