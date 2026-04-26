import { ethers, network, run } from "hardhat";
import * as fs from "fs";
import * as path from "path";

async function main() {
  const [deployer] = await ethers.getSigners();
  console.log(`Deploying with: ${deployer.address}`);
  console.log(`Network:        ${network.name}`);
  const balance = await ethers.provider.getBalance(deployer.address);
  console.log(`Balance:        ${ethers.formatEther(balance)} ETH`);

  const Factory = await ethers.getContractFactory("PredictionMarket");
  const market = await Factory.deploy();
  await market.waitForDeployment();
  const address = await market.getAddress();

  console.log(`PredictionMarket deployed at: ${address}`);

  // Save deployment artifact for the frontend.
  const artifact = {
    address,
    network: network.name,
    chainId: Number(network.config.chainId ?? 0),
    deployedAt: new Date().toISOString(),
  };
  const outDir = path.join(__dirname, "..", "deployments");
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(
    path.join(outDir, `${network.name}.json`),
    JSON.stringify(artifact, null, 2)
  );

  // Also drop the ABI into the frontend so it can be imported directly.
  const abiSrc = path.join(
    __dirname,
    "..",
    "artifacts",
    "contracts",
    "PredictionMarket.sol",
    "PredictionMarket.json"
  );
  if (fs.existsSync(abiSrc)) {
    const built = JSON.parse(fs.readFileSync(abiSrc, "utf8"));
    const abiOut = path.join(__dirname, "..", "frontend", "src", "abis", "PredictionMarket.json");
    fs.mkdirSync(path.dirname(abiOut), { recursive: true });
    fs.writeFileSync(
      abiOut,
      JSON.stringify({ address, abi: built.abi }, null, 2)
    );
    console.log(`Frontend ABI written to: ${abiOut}`);
  }

  // Auto-verify on supported networks.
  if (
    network.name === "sepolia" ||
    network.name === "baseSepolia"
  ) {
    console.log("Waiting 30s for explorer indexing before verification...");
    await new Promise((r) => setTimeout(r, 30_000));
    try {
      await run("verify:verify", { address, constructorArguments: [] });
      console.log("Verified on block explorer.");
    } catch (e: any) {
      console.warn(`Verification skipped/failed: ${e.message ?? e}`);
    }
  }
}

main().catch((err) => {
  console.error(err);
  process.exitCode = 1;
});
