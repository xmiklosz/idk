import { ethers } from "ethers";

export enum Outcome {
  UNRESOLVED = 0,
  YES = 1,
  NO = 2,
  INVALID = 3,
}

export const outcomeLabel = (o: number): string => {
  switch (Number(o)) {
    case Outcome.YES: return "YES";
    case Outcome.NO: return "NO";
    case Outcome.INVALID: return "INVALID";
    default: return "UNRESOLVED";
  }
};

/// Generates a cryptographically random 32-byte salt, hex-encoded.
export function randomSalt(): string {
  const bytes = new Uint8Array(32);
  crypto.getRandomValues(bytes);
  return ethers.hexlify(bytes);
}

/// Reproduces the on-chain commitment: keccak256(abi.encodePacked(uint8 outcome, bytes32 salt)).
export function commitmentHash(outcome: Outcome, salt: string): string {
  return ethers.solidityPackedKeccak256(["uint8", "bytes32"], [outcome, salt]);
}

const KEY = (marketId: bigint | number, account: string) =>
  `pm:salt:${marketId.toString()}:${account.toLowerCase()}`;

export interface SavedCommit {
  outcome: Outcome;
  salt: string;
  savedAt: number;
}

export function saveCommit(marketId: bigint | number, account: string, payload: SavedCommit): void {
  localStorage.setItem(KEY(marketId, account), JSON.stringify(payload));
}

export function loadCommit(marketId: bigint | number, account: string): SavedCommit | null {
  const raw = localStorage.getItem(KEY(marketId, account));
  if (!raw) return null;
  try {
    return JSON.parse(raw) as SavedCommit;
  } catch {
    return null;
  }
}

export function clearCommit(marketId: bigint | number, account: string): void {
  localStorage.removeItem(KEY(marketId, account));
}
