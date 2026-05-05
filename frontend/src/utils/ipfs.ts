// Lightweight helpers for working with IPFS CIDs from the browser.
//
// Uploads are not done from the frontend (that would require an API key).
// The intended workflow is:
//   1. Pin your metadata JSON / images via Pinata, web3.storage, or
//      IPFS Desktop and copy the resulting CID.
//   2. Paste it in the "Metadata CID" field when creating a market.
// The frontend then resolves it through a public gateway for display.

const PUBLIC_GATEWAY =
  (import.meta.env.VITE_IPFS_GATEWAY as string | undefined) ||
  "https://ipfs.io/ipfs/";

const CID_RE = /^(Qm[1-9A-HJ-NP-Za-km-z]{44}|b[A-Za-z2-7]{50,})$/;

export function isProbablyCID(s: string): boolean {
  if (!s) return false;
  return CID_RE.test(s.trim());
}

export function ipfsUrl(cidOrUri: string): string {
  if (!cidOrUri) return "";
  const cid = cidOrUri.startsWith("ipfs://") ? cidOrUri.slice("ipfs://".length) : cidOrUri;
  return `${PUBLIC_GATEWAY}${cid}`;
}

export interface MarketMetadata {
  description?: string;
  imageCID?: string;
  sources?: string[];
  tags?: string[];
}

export async function fetchMetadata(cid: string): Promise<MarketMetadata | null> {
  if (!cid) return null;
  try {
    const res = await fetch(ipfsUrl(cid));
    if (!res.ok) return null;
    const ct = res.headers.get("content-type") ?? "";
    if (ct.includes("application/json") || ct.includes("text/plain")) {
      return (await res.json()) as MarketMetadata;
    }
    return null;
  } catch {
    return null;
  }
}
