import { useEffect, useMemo, useState, useCallback } from "react";
import { BrowserProvider, Contract, JsonRpcSigner, ethers } from "ethers";
import abiJson from "../abis/PredictionMarket.json";

declare global {
  interface Window {
    ethereum?: any;
  }
}

const CONTRACT_ADDRESS =
  (import.meta.env.VITE_CONTRACT_ADDRESS as string | undefined) ||
  (abiJson as any).address ||
  "0x0000000000000000000000000000000000000000";

const CHAIN_ID = Number(import.meta.env.VITE_CHAIN_ID ?? 11155111);
const RPC_URL = (import.meta.env.VITE_RPC_URL as string | undefined) || "https://rpc.sepolia.org";

export const ABI = (abiJson as any).abi;
export { CONTRACT_ADDRESS, CHAIN_ID, RPC_URL };

export interface WalletState {
  account: string | null;
  chainId: number | null;
  provider: BrowserProvider | null;
  signer: JsonRpcSigner | null;
  readContract: Contract;
  writeContract: Contract | null;
  isCorrectChain: boolean;
  connect: () => Promise<void>;
  switchChain: () => Promise<void>;
}

export function useWallet(): WalletState {
  const [account, setAccount] = useState<string | null>(null);
  const [chainId, setChainId] = useState<number | null>(null);
  const [provider, setProvider] = useState<BrowserProvider | null>(null);
  const [signer, setSigner] = useState<JsonRpcSigner | null>(null);

  const readProvider = useMemo(() => new ethers.JsonRpcProvider(RPC_URL), []);
  const readContract = useMemo(
    () => new Contract(CONTRACT_ADDRESS, ABI, readProvider),
    [readProvider]
  );

  const writeContract = useMemo(() => {
    if (!signer) return null;
    return new Contract(CONTRACT_ADDRESS, ABI, signer);
  }, [signer]);

  const refresh = useCallback(async () => {
    if (!window.ethereum) return;
    const p = new BrowserProvider(window.ethereum);
    setProvider(p);
    try {
      const accounts: string[] = await window.ethereum.request({ method: "eth_accounts" });
      if (accounts.length > 0) {
        const s = await p.getSigner();
        setAccount(accounts[0]);
        setSigner(s);
      } else {
        setAccount(null);
        setSigner(null);
      }
      const net = await p.getNetwork();
      setChainId(Number(net.chainId));
    } catch (e) {
      console.error(e);
    }
  }, []);

  useEffect(() => {
    refresh();
    if (!window.ethereum) return;
    const onAccounts = () => refresh();
    const onChain = () => refresh();
    window.ethereum.on?.("accountsChanged", onAccounts);
    window.ethereum.on?.("chainChanged", onChain);
    return () => {
      window.ethereum.removeListener?.("accountsChanged", onAccounts);
      window.ethereum.removeListener?.("chainChanged", onChain);
    };
  }, [refresh]);

  const connect = useCallback(async () => {
    if (!window.ethereum) {
      alert("MetaMask not detected. Please install it from https://metamask.io");
      return;
    }
    await window.ethereum.request({ method: "eth_requestAccounts" });
    await refresh();
  }, [refresh]);

  const switchChain = useCallback(async () => {
    if (!window.ethereum) return;
    const hex = "0x" + CHAIN_ID.toString(16);
    try {
      await window.ethereum.request({
        method: "wallet_switchEthereumChain",
        params: [{ chainId: hex }],
      });
    } catch (err: any) {
      if (err?.code === 4902) {
        // Chain not added; we only auto-add Sepolia / Base Sepolia.
        const params =
          CHAIN_ID === 11155111
            ? {
                chainId: hex,
                chainName: "Sepolia",
                nativeCurrency: { name: "SepoliaETH", symbol: "ETH", decimals: 18 },
                rpcUrls: [RPC_URL],
                blockExplorerUrls: ["https://sepolia.etherscan.io"],
              }
            : CHAIN_ID === 84532
            ? {
                chainId: hex,
                chainName: "Base Sepolia",
                nativeCurrency: { name: "ETH", symbol: "ETH", decimals: 18 },
                rpcUrls: [RPC_URL],
                blockExplorerUrls: ["https://sepolia.basescan.org"],
              }
            : null;
        if (params) {
          await window.ethereum.request({ method: "wallet_addEthereumChain", params: [params] });
        }
      } else {
        throw err;
      }
    }
  }, []);

  return {
    account,
    chainId,
    provider,
    signer,
    readContract,
    writeContract,
    isCorrectChain: chainId === CHAIN_ID,
    connect,
    switchChain,
  };
}
