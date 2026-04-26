import { network, run } from "hardhat";
import * as fs from "fs";
import * as path from "path";

async function main() {
  const file = path.join(__dirname, "..", "deployments", `${network.name}.json`);
  if (!fs.existsSync(file)) {
    throw new Error(`No deployment file for ${network.name}: ${file}`);
  }
  const { address } = JSON.parse(fs.readFileSync(file, "utf8"));
  console.log(`Verifying ${address} on ${network.name}...`);
  await run("verify:verify", { address, constructorArguments: [] });
}

main().catch((err) => {
  console.error(err);
  process.exitCode = 1;
});
