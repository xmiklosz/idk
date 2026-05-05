// Chainlink price-feed presets for the supported testnets. These addresses
// are the official Chainlink Data Feeds — see
// https://docs.chain.link/data-feeds/price-feeds/addresses

export interface PriceFeed {
  address: string;
  pair: string;
  decimals: number;
}

export const PRICE_FEEDS: Record<number, PriceFeed[]> = {
  // Sepolia
  11155111: [
    { address: "0x694AA1769357215DE4FAC081bf1f309aDC325306", pair: "ETH / USD",  decimals: 8 },
    { address: "0x1b44F3514812d835EB1BDB0acB33d3fA3351Ee43", pair: "BTC / USD",  decimals: 8 },
    { address: "0xc59E3633BAAC79493d908e63626716e204A45EdF", pair: "LINK / USD", decimals: 8 },
    { address: "0xA2F78ab2355fe2f984D808B5CeE7FD0A93D5270E", pair: "USDC / USD", decimals: 8 },
  ],
  // Base Sepolia
  84532: [
    { address: "0x4aDC67696bA383F43DD60A9e78F2C97Fbbfc7cb1", pair: "ETH / USD", decimals: 8 },
    { address: "0x4aDC67696bA383F43DD60A9e78F2C97Fbbfc7cb1", pair: "BTC / USD", decimals: 8 },
  ],
};

export function feedsForChain(chainId: number | null): PriceFeed[] {
  if (!chainId) return [];
  return PRICE_FEEDS[chainId] ?? [];
}
