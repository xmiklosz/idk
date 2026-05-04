import { ethers, network, run } from "hardhat";
import * as fs from "fs";
import * as path from "path";

async function main() {
  const [deployer] = await ethers.getSigners();
  console.log(`Deploying with: ${deployer.address}`);
  console.log(`Network:        ${network.name}`);
  console.log(`Balance:        ${ethers.formatEther(await ethers.provider.getBalance(deployer.address))} ETH`);

  // 1. Deploy the oracle registry first.
  const Registry = await ethers.getContractFactory("OracleRegistry");
  const registry = await Registry.deploy();
  await registry.waitForDeployment();
  const registryAddress = await registry.getAddress();
  console.log(`OracleRegistry deployed at: ${registryAddress}`);

  // 2. Deploy the market, wired to the registry.
  const Market = await ethers.getContractFactory("PredictionMarket");
  const market = await Market.deploy(registryAddress);
  await market.waitForDeployment();
  const marketAddress = await market.getAddress();
  console.log(`PredictionMarket deployed at: ${marketAddress}`);

  // 3. Approve the market as a slasher in the registry.
  const tx = await registry.approveSlasher(marketAddress);
  await tx.wait();
  console.log(`Approved market as slasher in registry`);

  // 4. Persist deployment info + ABIs for the frontend.
  const outDir = path.join(__dirname, "..", "deployments");
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
  fs.writeFileSync(
    path.join(outDir, `${network.name}.json`),
    JSON.stringify({
      registry: registryAddress,
      market: marketAddress,
      network: network.name,
      chainId: Number(network.config.chainId ?? 0),
      deployedAt: new Date().toISOString(),
    }, null, 2)
  );

  const abiOutDir = path.join(__dirname, "..", "frontend", "src", "abis");
  fs.mkdirSync(abiOutDir, { recursive: true });

  function dumpAbi(contractName: string, address: string, outName: string) {
    const src = path.join(__dirname, "..", "artifacts", "contracts", `${contractName}.sol`, `${contractName}.json`);
    if (!fs.existsSync(src)) return;
    const built = JSON.parse(fs.readFileSync(src, "utf8"));
    fs.writeFileSync(
      path.join(abiOutDir, outName),
      JSON.stringify({ address, abi: built.abi }, null, 2)
    );
    console.log(`Wrote ${outName}`);
  }
  dumpAbi("PredictionMarket", marketAddress, "PredictionMarket.json");
  dumpAbi("OracleRegistry", registryAddress, "OracleRegistry.json");

  // 5. Auto-verify on supported testnets.
  if (network.name === "sepolia" || network.name === "baseSepolia") {
    console.log("Waiting 30s for explorer indexing before verification...");
    await new Promise((r) => setTimeout(r, 30_000));
    try {
      await run("verify:verify", { address: registryAddress, constructorArguments: [] });
      console.log("Registry verified.");
    } catch (e: any) { console.warn(`Registry verify: ${e.message ?? e}`); }
    try {
      await run("verify:verify", { address: marketAddress, constructorArguments: [registryAddress] });
      console.log("Market verified.");
    } catch (e: any) { console.warn(`Market verify: ${e.message ?? e}`); }
  }
}

main().catch((err) => { console.error(err); process.exitCode = 1; });
