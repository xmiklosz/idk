export enum Outcome {
  UNRESOLVED = 0,
  YES = 1,
  NO = 2,
  INVALID = 3,
}

export enum State {
  Trading = 0,
  Proposed = 1,
  Disputed = 2,
  Resolved = 3,
  Expired = 4,
}

export const outcomeLabel = (o: number): string => {
  switch (Number(o)) {
    case Outcome.YES: return "YES";
    case Outcome.NO: return "NO";
    case Outcome.INVALID: return "INVALID";
    default: return "—";
  }
};

export const stateLabel = (s: number): string => {
  switch (Number(s)) {
    case State.Trading: return "Trading";
    case State.Proposed: return "Proposed";
    case State.Disputed: return "Disputed";
    case State.Resolved: return "Resolved";
    case State.Expired: return "Expired";
    default: return "—";
  }
};

export function shortAddr(a: string): string {
  if (!a || a === "0x0000000000000000000000000000000000000000") return "—";
  return `${a.slice(0, 6)}…${a.slice(-4)}`;
}

export function fmtTimestamp(t: bigint | number): string {
  const n = typeof t === "bigint" ? Number(t) : t;
  if (!n) return "—";
  return new Date(n * 1000).toLocaleString();
}

export function countdown(target: bigint | number, now: number): string {
  const t = typeof target === "bigint" ? Number(target) : target;
  if (!t) return "—";
  const s = t - now;
  if (s <= 0) return "now";
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const sec = s % 60;
  return `${h}h ${m}m ${sec}s`;
}
