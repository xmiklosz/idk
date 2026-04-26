import { WalletState } from "../hooks/useContract";

const short = (a: string) => `${a.slice(0, 6)}…${a.slice(-4)}`;

export default function WalletConnect({ wallet }: { wallet: WalletState }) {
  if (!wallet.account) {
    return (
      <button className="btn-primary" onClick={() => wallet.connect()}>
        Connect Wallet
      </button>
    );
  }
  if (!wallet.isCorrectChain) {
    return (
      <button className="btn-danger" onClick={() => wallet.switchChain()}>
        Wrong network — switch
      </button>
    );
  }
  return (
    <div className="flex items-center gap-2">
      <span className="badge bg-emerald-500/20 text-emerald-300">Connected</span>
      <span className="font-mono text-sm">{short(wallet.account)}</span>
    </div>
  );
}
