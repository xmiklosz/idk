import { useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";

export default function StakePanel({
  wallet,
  marketId,
  onDone,
}: {
  wallet: WalletState;
  marketId: bigint;
  onDone: () => void;
}) {
  const [amount, setAmount] = useState("0.01");
  const [busy, setBusy] = useState(false);

  async function stake(side: "yes" | "no") {
    if (!wallet.marketWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading(`Staking ${amount} ETH on ${side.toUpperCase()}…`);
    try {
      const tx = await (side === "yes"
        ? wallet.marketWrite.stakeYes(marketId, { value: ethers.parseEther(amount) })
        : wallet.marketWrite.stakeNo(marketId, { value: ethers.parseEther(amount) }));
      toast.loading("Waiting for confirmation…", { id: t });
      await tx.wait();
      toast.success(`Staked on ${side.toUpperCase()}`, { id: t });
      onDone();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="card">
      <h3 className="font-semibold mb-3">Stake</h3>
      <label className="label">Amount (ETH)</label>
      <input
        className="input mb-3"
        type="text"
        value={amount}
        onChange={(e) => setAmount(e.target.value)}
      />
      <div className="grid grid-cols-2 gap-2">
        <button className="btn-success" disabled={busy} onClick={() => stake("yes")}>YES</button>
        <button className="btn-danger" disabled={busy} onClick={() => stake("no")}>NO</button>
      </div>
    </div>
  );
}
