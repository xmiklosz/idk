import { useEffect } from "react";
import toast from "react-hot-toast";
import { WalletState } from "./useContract";
import { outcomeLabel } from "../utils/outcome";

// Subscribes to PredictionMarket events and shows a toast when something
// relevant happens to the connected user (their proposal got disputed, the
// market they staked on got finalized, etc.).
//
// For real cross-device notifications, hook into Push Protocol: register a
// channel, then forward these events to push.subscribers via their REST API.
// See README "Push Protocol integration" for the channel setup.

export function useNotifications(wallet: WalletState) {
  useEffect(() => {
    if (!wallet.account) return;
    const me = wallet.account.toLowerCase();
    const m = wallet.market;

    const onProposed = async (marketId: bigint, proposer: string, outcome: bigint) => {
      try {
        const market = await m.markets(marketId);
        const userY: bigint = await m.yesStakes(marketId, me);
        const userN: bigint = await m.noStakes(marketId, me);
        if (proposer.toLowerCase() === me) {
          toast.success(`Your proposal of ${outcomeLabel(Number(outcome))} on market #${marketId} is live`);
        } else if (userY + userN > 0n || market.creator.toLowerCase() === me) {
          toast(`Market #${marketId}: ${outcomeLabel(Number(outcome))} proposed — dispute window open`, { icon: "⚠️" });
        }
      } catch {}
    };

    const onDisputed = async (marketId: bigint, disputer: string) => {
      try {
        const market = await m.markets(marketId);
        if (market.proposer.toLowerCase() === me) {
          toast.error(`Your proposal on market #${marketId} was disputed`);
        } else if (disputer.toLowerCase() === me) {
          toast.success(`Your dispute on market #${marketId} is open for oracle vote`);
        }
      } catch {}
    };

    const onFinalized = async (marketId: bigint, result: bigint) => {
      try {
        const userY: bigint = await m.yesStakes(marketId, me);
        const userN: bigint = await m.noStakes(marketId, me);
        if (userY + userN > 0n) {
          toast(`Market #${marketId} resolved as ${outcomeLabel(Number(result))} — claim your winnings`, { icon: "🏁" });
        }
      } catch {}
    };

    m.on("Proposed", onProposed);
    m.on("Disputed", onDisputed);
    m.on("MarketFinalized", onFinalized);
    return () => {
      m.off("Proposed", onProposed);
      m.off("Disputed", onDisputed);
      m.off("MarketFinalized", onFinalized);
    };
  }, [wallet.account, wallet.market]);
}
