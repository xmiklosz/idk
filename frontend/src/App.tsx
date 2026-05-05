import { Link, Route, Routes } from "react-router-dom";
import { useWallet, MARKET_ADDRESS, REGISTRY_ADDRESS, CHAIN_ID } from "./hooks/useContract";
import { useNotifications } from "./hooks/useNotifications";
import WalletConnect from "./components/WalletConnect";
import MarketList from "./components/MarketList";
import MarketDetail from "./components/MarketDetail";
import CreateMarket from "./components/CreateMarket";
import OraclePage from "./components/OraclePage";

export default function App() {
  const wallet = useWallet();
  useNotifications(wallet);

  return (
    <div className="min-h-screen flex flex-col">
      <header className="border-b border-slate-800 bg-slate-900/80 backdrop-blur sticky top-0 z-10">
        <div className="max-w-5xl mx-auto px-4 py-3 flex items-center justify-between flex-wrap gap-3">
          <div className="flex items-center gap-4">
            <Link to="/" className="font-bold text-lg">OptiMarket</Link>
            <Link to="/" className="text-sm text-slate-300 hover:text-white">Markets</Link>
            <Link to="/create" className="text-sm text-slate-300 hover:text-white">Create</Link>
            <Link to="/oracles" className="text-sm text-slate-300 hover:text-white">Oracles</Link>
          </div>
          <WalletConnect wallet={wallet} />
        </div>
      </header>

      <main className="flex-1">
        <div className="max-w-5xl mx-auto px-4 py-8">
          <Routes>
            <Route path="/" element={<MarketList wallet={wallet} />} />
            <Route path="/create" element={<CreateMarket wallet={wallet} />} />
            <Route path="/market/:id" element={<MarketDetail wallet={wallet} />} />
            <Route path="/oracles" element={<OraclePage wallet={wallet} />} />
          </Routes>
        </div>
      </main>

      <footer className="border-t border-slate-800 mt-10">
        <div className="max-w-5xl mx-auto px-4 py-4 text-xs text-slate-500 flex flex-wrap gap-3 justify-between">
          <span>DMBLOCK Assignment 2 — Optimistic Oracle Prediction Market</span>
          <span className="font-mono">
            chain {CHAIN_ID} · market {MARKET_ADDRESS.slice(0, 6)}…{MARKET_ADDRESS.slice(-4)}
            {" "}· registry {REGISTRY_ADDRESS.slice(0, 6)}…{REGISTRY_ADDRESS.slice(-4)}
          </span>
        </div>
      </footer>
    </div>
  );
}
