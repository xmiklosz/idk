import { expect } from "chai";
import { ethers } from "hardhat";
import { time } from "@nomicfoundation/hardhat-network-helpers";
import { PredictionMarket } from "../typechain-types";

const ONE_HOUR = 60 * 60;
const COLLATERAL = ethers.parseEther("0.01");
const BOND = ethers.parseEther("0.1");

enum Outcome {
  UNRESOLVED = 0,
  YES = 1,
  NO = 2,
  INVALID = 3,
}

function makeSalt(seed: string): string {
  return ethers.keccak256(ethers.toUtf8Bytes(seed));
}

function commitmentHash(outcome: Outcome, salt: string): string {
  return ethers.solidityPackedKeccak256(["uint8", "bytes32"], [outcome, salt]);
}

async function deploy() {
  const Factory = await ethers.getContractFactory("PredictionMarket");
  const market = (await Factory.deploy()) as unknown as PredictionMarket;
  await market.waitForDeployment();
  return market;
}

async function createDefaultMarket(market: PredictionMarket, creator: any, quorum = 3n) {
  const now = await time.latest();
  const resolutionTime = now + ONE_HOUR;
  const commitDeadline = resolutionTime + ONE_HOUR;
  const revealDeadline = commitDeadline + ONE_HOUR;
  const tx = await market
    .connect(creator)
    .createMarket(
      "Will ETH > $3000 on June 1?",
      resolutionTime,
      commitDeadline,
      revealDeadline,
      quorum,
      { value: BOND }
    );
  await tx.wait();
  return { id: 0n, resolutionTime, commitDeadline, revealDeadline };
}

describe("PredictionMarket", () => {
  describe("createMarket", () => {
    it("creates a market and emits MarketCreated", async () => {
      const market = await deploy();
      const [creator] = await ethers.getSigners();
      const now = await time.latest();
      const rt = now + ONE_HOUR;
      const cd = rt + ONE_HOUR;
      const rd = cd + ONE_HOUR;

      await expect(
        market.connect(creator).createMarket("Q?", rt, cd, rd, 3, { value: BOND })
      )
        .to.emit(market, "MarketCreated")
        .withArgs(0n, creator.address, "Q?", rt);

      expect(await market.marketCount()).to.equal(1n);
      const m = await market.markets(0n);
      expect(m.creator).to.equal(creator.address);
      expect(m.creatorBond).to.equal(BOND);
      expect(m.quorum).to.equal(3n);
    });

    it("reverts on empty question", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("", now + ONE_HOUR, now + 2 * ONE_HOUR, now + 3 * ONE_HOUR, 1, { value: BOND })
      ).to.be.revertedWith("empty question");
    });

    it("reverts when resolutionTime in past", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("Q?", now - 10, now + ONE_HOUR, now + 2 * ONE_HOUR, 1, { value: BOND })
      ).to.be.revertedWith("resolutionTime in past");
    });

    it("reverts when commitDeadline <= resolutionTime", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("Q?", now + ONE_HOUR, now + ONE_HOUR, now + 2 * ONE_HOUR, 1, { value: BOND })
      ).to.be.revertedWith("commitDeadline <= resolutionTime");
    });

    it("reverts when revealDeadline <= commitDeadline", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("Q?", now + ONE_HOUR, now + 2 * ONE_HOUR, now + 2 * ONE_HOUR, 1, { value: BOND })
      ).to.be.revertedWith("revealDeadline <= commitDeadline");
    });

    it("reverts when quorum is zero", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("Q?", now + ONE_HOUR, now + 2 * ONE_HOUR, now + 3 * ONE_HOUR, 0, { value: BOND })
      ).to.be.revertedWith("quorum zero");
    });

    it("reverts when no creator bond", async () => {
      const market = await deploy();
      const now = await time.latest();
      await expect(
        market.createMarket("Q?", now + ONE_HOUR, now + 2 * ONE_HOUR, now + 3 * ONE_HOUR, 1)
      ).to.be.revertedWith("no creator bond");
    });
  });

  describe("staking", () => {
    it("stakes YES and NO before resolution time", async () => {
      const market = await deploy();
      const [creator, alice, bob] = await ethers.getSigners();
      await createDefaultMarket(market, creator);

      await expect(market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") }))
        .to.emit(market, "Staked")
        .withArgs(0n, alice.address, true, ethers.parseEther("1"));

      await market.connect(bob).stakeNo(0n, { value: ethers.parseEther("2") });

      const m = await market.markets(0n);
      expect(m.totalYesStake).to.equal(ethers.parseEther("1"));
      expect(m.totalNoStake).to.equal(ethers.parseEther("2"));
      expect(await market.yesStakes(0n, alice.address)).to.equal(ethers.parseEther("1"));
      expect(await market.noStakes(0n, bob.address)).to.equal(ethers.parseEther("2"));
    });

    it("reverts staking on a non-existent market", async () => {
      const market = await deploy();
      await expect(market.stakeYes(99n, { value: 1n })).to.be.revertedWith("no market");
      await expect(market.stakeNo(99n, { value: 1n })).to.be.revertedWith("no market");
    });

    it("reverts staking after resolutionTime", async () => {
      const market = await deploy();
      const [creator, alice] = await ethers.getSigners();
      const { resolutionTime } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      await expect(
        market.connect(alice).stakeYes(0n, { value: 1n })
      ).to.be.revertedWith("staking closed");
      await expect(
        market.connect(alice).stakeNo(0n, { value: 1n })
      ).to.be.revertedWith("staking closed");
    });

    it("reverts on zero-value stake", async () => {
      const market = await deploy();
      const [creator, alice] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await expect(market.connect(alice).stakeYes(0n)).to.be.revertedWith("zero stake");
    });
  });

  describe("commit-reveal", () => {
    it("rejects commit before resolutionTime", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      const c = commitmentHash(Outcome.YES, makeSalt("a"));
      await expect(
        market.connect(r1).commitResolution(0n, c, { value: COLLATERAL })
      ).to.be.revertedWith("commit not open");
    });

    it("requires correct collateral", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const c = commitmentHash(Outcome.YES, makeSalt("a"));
      await expect(
        market.connect(r1).commitResolution(0n, c, { value: ethers.parseEther("0.005") })
      ).to.be.revertedWith("bad collateral");
    });

    it("rejects double commit", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const c = commitmentHash(Outcome.YES, makeSalt("a"));
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await expect(
        market.connect(r1).commitResolution(0n, c, { value: COLLATERAL })
      ).to.be.revertedWith("already committed");
    });

    it("rejects reveal before commitDeadline", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.YES, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await expect(
        market.connect(r1).revealResolution(0n, Outcome.YES, salt)
      ).to.be.revertedWith("reveal not open");
    });

    it("rejects reveal with wrong salt", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime, commitDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.YES, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await time.increaseTo(commitDeadline + 1);
      await expect(
        market.connect(r1).revealResolution(0n, Outcome.YES, makeSalt("not-a"))
      ).to.be.revertedWith("bad reveal");
    });

    it("rejects reveal with wrong outcome", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime, commitDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.YES, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await time.increaseTo(commitDeadline + 1);
      await expect(
        market.connect(r1).revealResolution(0n, Outcome.NO, salt)
      ).to.be.revertedWith("bad reveal");
    });

    it("rejects reveal of UNRESOLVED outcome", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime, commitDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.UNRESOLVED, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await time.increaseTo(commitDeadline + 1);
      await expect(
        market.connect(r1).revealResolution(0n, Outcome.UNRESOLVED, salt)
      ).to.be.revertedWith("bad outcome");
    });

    it("accepts valid reveal and emits event", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime, commitDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.YES, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await time.increaseTo(commitDeadline + 1);
      await expect(market.connect(r1).revealResolution(0n, Outcome.YES, salt))
        .to.emit(market, "Revealed")
        .withArgs(0n, r1.address, Outcome.YES);
    });

    it("rejects double reveal", async () => {
      const market = await deploy();
      const [creator, r1] = await ethers.getSigners();
      const { resolutionTime, commitDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      const salt = makeSalt("a");
      const c = commitmentHash(Outcome.YES, salt);
      await market.connect(r1).commitResolution(0n, c, { value: COLLATERAL });
      await time.increaseTo(commitDeadline + 1);
      await market.connect(r1).revealResolution(0n, Outcome.YES, salt);
      await expect(
        market.connect(r1).revealResolution(0n, Outcome.YES, salt)
      ).to.be.revertedWith("already revealed");
    });
  });

  describe("finalization & payouts", () => {
    async function setupVotedMarket(quorum: number, votes: Outcome[]) {
      const market = await deploy();
      const signers = await ethers.getSigners();
      const [creator, alice, bob, ...resolvers] = signers;

      const { resolutionTime, commitDeadline, revealDeadline } =
        await createDefaultMarket(market, creator, BigInt(quorum));

      // Stake from two stakers
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("3") });
      await market.connect(bob).stakeNo(0n, { value: ethers.parseEther("1") });

      // Commit phase
      await time.increaseTo(resolutionTime + 1);
      const salts: string[] = [];
      for (let i = 0; i < votes.length; i++) {
        const salt = makeSalt(`r${i}`);
        salts.push(salt);
        const c = commitmentHash(votes[i], salt);
        await market.connect(resolvers[i]).commitResolution(0n, c, { value: COLLATERAL });
      }

      // Reveal phase
      await time.increaseTo(commitDeadline + 1);
      for (let i = 0; i < votes.length; i++) {
        await market
          .connect(resolvers[i])
          .revealResolution(0n, votes[i], salts[i]);
      }

      await time.increaseTo(revealDeadline + 1);
      return { market, creator, alice, bob, resolvers: resolvers.slice(0, votes.length) };
    }

    it("rejects finalize before revealDeadline", async () => {
      const market = await deploy();
      const [creator] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await expect(market.finalizeMarket(0n)).to.be.revertedWith("reveal still open");
    });

    it("YES wins: stakers and winning resolvers paid out", async () => {
      const { market, alice, bob, resolvers } = await setupVotedMarket(3, [
        Outcome.YES,
        Outcome.YES,
        Outcome.NO, // minority - will be slashed
      ]);

      await expect(market.finalizeMarket(0n))
        .to.emit(market, "MarketFinalized")
        .withArgs(0n, Outcome.YES);

      const m = await market.markets(0n);
      expect(m.result).to.equal(Outcome.YES);
      expect(m.winningResolverCount).to.equal(2n);
      expect(m.slashPool).to.equal(COLLATERAL); // one minority slashed

      // Alice (YES staker) claims
      const aliceBefore = await ethers.provider.getBalance(alice.address);
      const tx = await market.connect(alice).claimWinnings(0n);
      const receipt = await tx.wait();
      const gas = receipt!.gasUsed * receipt!.gasPrice;
      const aliceAfter = await ethers.provider.getBalance(alice.address);
      // Alice was the only YES staker and gets her stake + all losing pool.
      const expectedExtra = m.totalNoStake + m.creatorBond + m.slashPool / 2n;
      const expectedPayout = ethers.parseEther("3") + expectedExtra;
      expect(aliceAfter - aliceBefore + gas).to.equal(expectedPayout);

      // Bob (NO staker) cannot claim
      await expect(market.connect(bob).claimWinnings(0n)).to.be.revertedWith("nothing to claim");

      // Winning resolvers each get collateral + share of slash pool / 2
      const r0Before = await ethers.provider.getBalance(resolvers[0].address);
      const tx2 = await market.connect(resolvers[0]).claimResolverReward(0n);
      const r2 = await tx2.wait();
      const r0After = await ethers.provider.getBalance(resolvers[0].address);
      const gas2 = r2!.gasUsed * r2!.gasPrice;
      const expectedReward = COLLATERAL + (COLLATERAL / 2n) / 2n;
      expect(r0After - r0Before + gas2).to.equal(expectedReward);

      // Minority resolver cannot claim
      await expect(
        market.connect(resolvers[2]).claimResolverReward(0n)
      ).to.be.revertedWith("not a winning resolver");
    });

    it("double-claim is rejected", async () => {
      const { market, alice } = await setupVotedMarket(3, [
        Outcome.YES,
        Outcome.YES,
        Outcome.NO,
      ]);
      await market.finalizeMarket(0n);
      await market.connect(alice).claimWinnings(0n);
      await expect(market.connect(alice).claimWinnings(0n)).to.be.revertedWith("already claimed");
    });

    it("cannot finalize twice", async () => {
      const { market } = await setupVotedMarket(3, [Outcome.YES, Outcome.YES, Outcome.YES]);
      await market.finalizeMarket(0n);
      await expect(market.finalizeMarket(0n)).to.be.revertedWith("already resolved");
    });

    it("cannot claim before finalize", async () => {
      const market = await deploy();
      const [creator, alice] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await expect(market.connect(alice).claimWinnings(0n)).to.be.revertedWith("not finalized");
      await expect(market.connect(alice).claimResolverReward(0n)).to.be.revertedWith("not finalized");
    });

    it("quorum not met -> INVALID, stakers refunded with bond + slash pool", async () => {
      // quorum is 3 but only 1 reveals
      const market = await deploy();
      const signers = await ethers.getSigners();
      const [creator, alice, bob, r1, r2] = signers;
      const { resolutionTime, commitDeadline, revealDeadline } =
        await createDefaultMarket(market, creator, 3n);

      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("3") });
      await market.connect(bob).stakeNo(0n, { value: ethers.parseEther("1") });

      await time.increaseTo(resolutionTime + 1);
      const s1 = makeSalt("x");
      const s2 = makeSalt("y");
      await market.connect(r1).commitResolution(0n, commitmentHash(Outcome.YES, s1), { value: COLLATERAL });
      await market.connect(r2).commitResolution(0n, commitmentHash(Outcome.NO, s2), { value: COLLATERAL });

      // Only r1 reveals -> below quorum
      await time.increaseTo(commitDeadline + 1);
      await market.connect(r1).revealResolution(0n, Outcome.YES, s1);

      await time.increaseTo(revealDeadline + 1);
      await expect(market.finalizeMarket(0n))
        .to.emit(market, "MarketFinalized")
        .withArgs(0n, Outcome.INVALID);

      const m = await market.markets(0n);
      expect(m.winningResolverCount).to.equal(0n);
      expect(m.slashPool).to.equal(COLLATERAL); // r2 (non-revealer) slashed

      // Alice (YES staker) claims pro-rata share of bond + slash pool
      const total = ethers.parseEther("4");
      const aliceStake = ethers.parseEther("3");
      const extra = m.creatorBond + m.slashPool;
      const expectedAlice = aliceStake + (extra * aliceStake) / total;
      const before = await ethers.provider.getBalance(alice.address);
      const tx = await market.connect(alice).claimWinnings(0n);
      const r = await tx.wait();
      const after = await ethers.provider.getBalance(alice.address);
      expect(after - before + r!.gasUsed * r!.gasPrice).to.equal(expectedAlice);

      // Bob (NO staker) also gets refund + share
      const expectedBob = ethers.parseEther("1") + (extra * ethers.parseEther("1")) / total;
      const beforeBob = await ethers.provider.getBalance(bob.address);
      const tx2 = await market.connect(bob).claimWinnings(0n);
      const r2recpt = await tx2.wait();
      const afterBob = await ethers.provider.getBalance(bob.address);
      expect(afterBob - beforeBob + r2recpt!.gasUsed * r2recpt!.gasPrice).to.equal(expectedBob);

      // r1 (revealed) gets collateral back
      const beforeR = await ethers.provider.getBalance(r1.address);
      const tx3 = await market.connect(r1).claimResolverReward(0n);
      const r3 = await tx3.wait();
      const afterR = await ethers.provider.getBalance(r1.address);
      expect(afterR - beforeR + r3!.gasUsed * r3!.gasPrice).to.equal(COLLATERAL);
    });

    it("non-revealing resolver cannot claim", async () => {
      const market = await deploy();
      const [creator, alice, , r1, r2] = await ethers.getSigners();
      const { resolutionTime, commitDeadline, revealDeadline } =
        await createDefaultMarket(market, creator, 1n);

      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") });
      await time.increaseTo(resolutionTime + 1);
      const s1 = makeSalt("x");
      const s2 = makeSalt("y");
      await market.connect(r1).commitResolution(0n, commitmentHash(Outcome.YES, s1), { value: COLLATERAL });
      await market.connect(r2).commitResolution(0n, commitmentHash(Outcome.YES, s2), { value: COLLATERAL });

      await time.increaseTo(commitDeadline + 1);
      await market.connect(r1).revealResolution(0n, Outcome.YES, s1);
      // r2 never reveals
      await time.increaseTo(revealDeadline + 1);
      await market.finalizeMarket(0n);

      await expect(market.connect(r2).claimResolverReward(0n)).to.be.revertedWith("did not reveal");
    });

    it("computeCommitment matches off-chain hash", async () => {
      const market = await deploy();
      const salt = makeSalt("z");
      const expected = commitmentHash(Outcome.YES, salt);
      expect(await market.computeCommitment(Outcome.YES, salt)).to.equal(expected);
    });

    it("getResolvers returns committed resolver addresses", async () => {
      const market = await deploy();
      const [creator, , , r1, r2] = await ethers.getSigners();
      const { resolutionTime } = await createDefaultMarket(market, creator);
      await time.increaseTo(resolutionTime + 1);
      await market.connect(r1).commitResolution(0n, commitmentHash(Outcome.YES, makeSalt("x")), { value: COLLATERAL });
      await market.connect(r2).commitResolution(0n, commitmentHash(Outcome.NO, makeSalt("y")), { value: COLLATERAL });
      const list = await market.getResolvers(0n);
      expect(list).to.deep.equal([r1.address, r2.address]);
      expect(await market.resolverCount(0n)).to.equal(2n);
    });

    it("tie between YES and NO falls back to INVALID with quorum met", async () => {
      const { market } = await setupVotedMarket(2, [Outcome.YES, Outcome.NO]);
      await market.finalizeMarket(0n);
      const m = await market.markets(0n);
      expect(m.result).to.equal(Outcome.INVALID);
      // Both resolvers slashed because neither matches winning (INVALID).
      expect(m.winningResolverCount).to.equal(0n);
      expect(m.slashPool).to.equal(COLLATERAL * 2n);
    });
  });
});
