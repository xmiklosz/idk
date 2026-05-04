/// <reference types="vite/client" />

interface ImportMetaEnv {
  readonly VITE_MARKET_ADDRESS?: string;
  readonly VITE_REGISTRY_ADDRESS?: string;
  readonly VITE_CHAIN_ID?: string;
  readonly VITE_RPC_URL?: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
