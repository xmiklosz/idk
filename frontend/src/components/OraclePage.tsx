import { useCallback, useEffect, useState } from "react";
import { ethers } from "ethers";
import toast from "react-hot-toast";
import { WalletState } from "../hooks/useContract";
import { shortAddr } from "../utils/outcome";

interface OracleRow {
  address: string;
  stake: bigint;
}

export default function OraclePage({ wallet }: { wallet: WalletState }) {
  const [oracles, setOracles] = useState<OracleRow[]>([]);
  const [minStake, setMinStake] = useState<bigint>(0n);
  const [totalStake, setTotalStake] = useState<bigint>(0n);
  const [myStake, setMyStake] = useState<bigint>(0n);
  const [isOracle, setIsOracle] = useState(false);
  const [stakeAmount, setStakeAmount] = useState("0.05");
  const [busy, setBusy] = useState(false);

  const refresh = useCallback(async () => {
    try {
      const min = await wallet.registry.MIN_STAKE();
      const total = await wallet.registry.totalStake();
      const list: string[] = await wallet.registry.getOracles();
      const rows: OracleRow[] = await Promise.all(
        list.map(async (a) => ({ address: a, stake: await wallet.registry.stakeOf(a) }))
      );
      setMinStake(min);
      setTotalStake(total);
      setOracles(rows);

      if (wallet.account) {
        setIsOracle(await wallet.registry.isOracle(wallet.account));
        setMyStake(await wallet.registry.stakeOf(wallet.account));
      }
    } catch (e) {
      console.error(e);
    }
  }, [wallet.registry, wallet.account]);

  useEffect(() => { refresh(); }, [refresh]);

  async function register() {
    if (!wallet.registryWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Registering as oracle…");
    try {
      const tx = await wallet.registryWrite.register({ value: ethers.parseEther(stakeAmount) });
      await tx.wait();
      toast.success("Registered", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  async function topUp() {
    if (!wallet.registryWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Topping up…");
    try {
      const tx = await wallet.registryWrite.topUp({ value: ethers.parseEther(stakeAmount) });
      await tx.wait();
      toast.success("Stake increased", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  async function unregister() {
    if (!wallet.registryWrite) return toast.error("Connect wallet");
    setBusy(true);
    const t = toast.loading("Unregistering…");
    try {
      const tx = await wallet.registryWrite.unregister();
      await tx.wait();
      toast.success("Stake withdrawn", { id: t });
      refresh();
    } catch (err: any) {
      toast.error(err?.shortMessage || err?.reason || err?.message || "Failed", { id: t });
    } finally { setBusy(false); }
  }

  return (
    <div className="space-y-6">
      <div>
        <h2 className="text-2xl font-semibold">Oracle network</h2>
        <p className="text-sm text-slate-400 mt-1">
          Anyone can register as an oracle by staking at least
          {" "}<strong>{ethers.formatEther(minStake)} ETH</strong>. Voting weight in dispute
          resolutions equals your staked balance. Voting against the final outcome means a
          slashable loss; voting with the majority earns a share of the slash pool.
        </p>
      </div>

      <div className="grid md:grid-cols-3 gap-4">
        <div className="card">
          <div className="text-slate-400 text-sm">Total network stake</div>
          <div className="font-mono text-lg">{ethers.formatEther(totalStake)} ETH</div>
        </div>
        <div className="card">
          <div className="text-slate-400 text-sm">Active oracles</div>
          <div className="font-mono text-lg">{oracles.length}</div>
        </div>
        <div className="card">
          <div className="text-slate-400 text-sm">Your stake</div>
          <div className="font-mono text-lg">
            {isOracle ? `${ethers.formatEther(myStake)} ETH` : "Not registered"}
          </div>
        </div>
      </div>

      <div className="card max-w-md">
        <h3 className="font-semibold mb-2">{isOracle ? "Manage your stake" : "Register as oracle"}</h3>
        <label className="label">Amount (ETH)</label>
        <input
          className="input mb-3"
          value={stakeAmount}
          onChange={(e) => setStakeAmount(e.target.value)}
        />
        <div className="grid grid-cols-2 gap-2">
          {!isOracle ? (
            <button className="btn-primary col-span-2" disabled={busy} onClick={register}>Register</button>
          ) : (
            <>
              <button className="btn-secondary" disabled={busy} onClick={topUp}>Top up</button>
              <button className="btn-danger" disabled={busy} onClick={unregister}>Unregister</button>
            </>
          )}
        </div>
      </div>

      <div>
        <h3 className="font-semibold mb-3">Active oracles</h3>
        <div className="card p-0 divide-y divide-slate-700">
          {oracles.length === 0 ? (
            <div className="p-4 text-slate-400 text-sm">No oracles registered yet.</div>
          ) : oracles.map((o) => (
            <div key={o.address} className="p-3 flex justify-between text-sm font-mono">
              <span>{shortAddr(o.address)}</span>
              <span>{ethers.formatEther(o.stake)} ETH</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
