import { expect } from "chai";
import { ethers } from "hardhat";
import { time } from "@nomicfoundation/hardhat-network-helpers";

const ONE_HOUR = 60 * 60;
const ONE_DAY = 24 * ONE_HOUR;
const PROPOSAL_BOND = ethers.parseEther("0.05");
const DISPUTE_BOND  = ethers.parseEther("0.05");
const MIN_STAKE     = ethers.parseEther("0.05");
const BOND          = ethers.parseEther("0.1");

enum Outcome { UNRESOLVED = 0, YES = 1, NO = 2, INVALID = 3 }
enum State   { Trading = 0, Proposed = 1, Disputed = 2, Resolved = 3, Expired = 4 }

async function deployStack() {
  const Registry = await ethers.getContractFactory("OracleRegistry");
  const registry = await Registry.deploy();
  await registry.waitForDeployment();
  const Market = await ethers.getContractFactory("PredictionMarket");
  const market = await Market.deploy(await registry.getAddress());
  await market.waitForDeployment();
  await (await registry.approveSlasher(await market.getAddress())).wait();
  return { registry, market };
}

async function createDefaultMarket(market: any, creator: any) {
  const now = await time.latest();
  const tradingDeadline  = now + ONE_HOUR;
  const proposalDeadline = tradingDeadline + ONE_HOUR;
  await (await market.connect(creator).createMarket(
    "Will ETH > $3000 on June 1?",
    tradingDeadline,
    proposalDeadline,
    { value: BOND }
  )).wait();
  return { id: 0n, tradingDeadline, proposalDeadline };
}

describe("OracleRegistry", () => {
  it("registers and tracks oracles", async () => {
    const { registry } = await deployStack();
    const [, a, b] = await ethers.getSigners();
    await expect(registry.connect(a).register({ value: MIN_STAKE }))
      .to.emit(registry, "OracleRegistered").withArgs(a.address, MIN_STAKE);
    await registry.connect(b).register({ value: MIN_STAKE * 2n });
    expect(await registry.oracleCount()).to.equal(2n);
    expect(await registry.totalStake()).to.equal(MIN_STAKE * 3n);
    expect(await registry.isOracle(a.address)).to.be.true;
    expect(await registry.stakeOf(b.address)).to.equal(MIN_STAKE * 2n);
  });

  it("rejects below-min stake on register", async () => {
    const { registry } = await deployStack();
    const [, a] = await ethers.getSigners();
    await expect(registry.connect(a).register({ value: MIN_STAKE - 1n }))
      .to.be.revertedWithCustomError(registry, "InsufficientStake");
  });

  it("topUp increases stake; unregister returns funds", async () => {
    const { registry } = await deployStack();
    const [, a] = await ethers.getSigners();
    await registry.connect(a).register({ value: MIN_STAKE });
    await registry.connect(a).topUp({ value: MIN_STAKE });
    expect(await registry.stakeOf(a.address)).to.equal(MIN_STAKE * 2n);

    const before = await ethers.provider.getBalance(a.address);
    const tx = await registry.connect(a).unregister();
    const r = await tx.wait();
    const after = await ethers.provider.getBalance(a.address);
    expect(after - before + r!.gasUsed * r!.gasPrice).to.equal(MIN_STAKE * 2n);
    expect(await registry.isOracle(a.address)).to.be.false;
  });

  it("only owner can approve a slasher", async () => {
    const { registry } = await deployStack();
    const [, a] = await ethers.getSigners();
    await expect(registry.connect(a).approveSlasher(a.address))
      .to.be.revertedWithCustomError(registry, "NotOwner");
  });

  it("only approved slasher can slash", async () => {
    const { registry } = await deployStack();
    const [, a, b] = await ethers.getSigners();
    await registry.connect(a).register({ value: MIN_STAKE });
    await expect(registry.connect(b).slash(a.address, MIN_STAKE, b.address))
      .to.be.revertedWithCustomError(registry, "NotSlasher");
  });
});

describe("PredictionMarket — Optimistic Oracle", () => {
  describe("creation & staking", () => {
    it("creates a market and emits MarketCreated", async () => {
      const { market } = await deployStack();
      const [creator] = await ethers.getSigners();
      const now = await time.latest();
      const td = now + ONE_HOUR;
      const pd = td + ONE_HOUR;
      await expect(market.connect(creator).createMarket("Q?", td, pd, { value: BOND }))
        .to.emit(market, "MarketCreated").withArgs(0n, creator.address, "Q?", td);
      const m = await market.markets(0n);
      expect(m.state).to.equal(State.Trading);
      expect(m.creatorBond).to.equal(BOND);
    });

    it("rejects invalid create params", async () => {
      const { market } = await deployStack();
      const [c] = await ethers.getSigners();
      const now = await time.latest();
      await expect(market.connect(c).createMarket("", now + 100, now + 200, { value: BOND }))
        .to.be.revertedWith("empty question");
      await expect(market.connect(c).createMarket("Q", now - 1, now + 200, { value: BOND }))
        .to.be.revertedWith("tradingDeadline in past");
      await expect(market.connect(c).createMarket("Q", now + 100, now + 100, { value: BOND }))
        .to.be.revertedWith("proposalDeadline <= tradingDeadline");
      await expect(market.connect(c).createMarket("Q", now + 100, now + 200))
        .to.be.revertedWith("no creator bond");
    });

    it("staking opens before tradingDeadline, closed after", async () => {
      const { market } = await deployStack();
      const [creator, alice] = await ethers.getSigners();
      const { tradingDeadline } = await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") });
      await time.increaseTo(tradingDeadline + 1);
      await expect(market.connect(alice).stakeNo(0n, { value: ethers.parseEther("1") }))
        .to.be.revertedWith("trading closed");
    });
  });

  describe("propose / dispute lifecycle", () => {
    it("rejects propose before tradingDeadline", async () => {
      const { market } = await deployStack();
      const [creator, p] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await expect(market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND }))
        .to.be.revertedWith("propose not open");
    });

    it("rejects bad proposal bond", async () => {
      const { market } = await deployStack();
      const [creator, p] = await ethers.getSigners();
      const { tradingDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(tradingDeadline + 1);
      await expect(market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND - 1n }))
        .to.be.revertedWith("bad proposal bond");
    });

    it("rejects propose of UNRESOLVED outcome", async () => {
      const { market } = await deployStack();
      const [creator, p] = await ethers.getSigners();
      const { tradingDeadline } = await createDefaultMarket(market, creator);
      await time.increaseTo(tradingDeadline + 1);
      await expect(market.connect(p).proposeOutcome(0n, Outcome.UNRESOLVED, { value: PROPOSAL_BOND }))
        .to.be.revertedWith("bad outcome");
    });

    it("undisputed proposal -> proposed outcome stands; proposer reclaims bond", async () => {
      const { market } = await deployStack();
      const [creator, alice, p] = await ethers.getSigners();
      const { tradingDeadline } = await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") });
      await time.increaseTo(tradingDeadline + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });

      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.disputeDeadline) + 1);

      await expect(market.finalizeMarket(0n))
        .to.emit(market, "MarketFinalized").withArgs(0n, Outcome.YES);

      const m2 = await market.markets(0n);
      expect(m2.state).to.equal(State.Resolved);
      expect(m2.result).to.equal(Outcome.YES);

      const before = await ethers.provider.getBalance(p.address);
      const tx = await market.connect(p).claimProposerBond(0n);
      const r = await tx.wait();
      const after = await ethers.provider.getBalance(p.address);
      expect(after - before + r!.gasUsed * r!.gasPrice).to.equal(PROPOSAL_BOND);

      // double-claim rejected
      await expect(market.connect(p).claimProposerBond(0n)).to.be.revertedWith("already claimed");
    });

    it("disputed proposal: oracle vote flips outcome, disputer wins both bonds", async () => {
      const { registry, market } = await deployStack();
      const [creator, alice, bob, p, d, o1, o2, o3] = await ethers.getSigners();

      await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("3") });
      await market.connect(bob).stakeNo(0n,  { value: ethers.parseEther("1") });

      // Register 3 oracles — two will vote NO (true outcome), one YES (with proposer)
      await registry.connect(o1).register({ value: MIN_STAKE });
      await registry.connect(o2).register({ value: MIN_STAKE });
      await registry.connect(o3).register({ value: MIN_STAKE });

      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);

      // Proposer falsely says YES; disputer challenges.
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });

      // Oracle vote: 2 NO, 1 YES → NO wins
      await market.connect(o1).voteOnDispute(0n, Outcome.NO);
      await market.connect(o2).voteOnDispute(0n, Outcome.NO);
      await market.connect(o3).voteOnDispute(0n, Outcome.YES);

      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.voteDeadline) + 1);

      await expect(market.finalizeMarket(0n))
        .to.emit(market, "MarketFinalized").withArgs(0n, Outcome.NO);

      const m2 = await market.markets(0n);
      expect(m2.result).to.equal(Outcome.NO);
      expect(m2.slashPool).to.equal(MIN_STAKE);            // o3's stake fully slashed
      expect(m2.winningVoteWeight).to.equal(MIN_STAKE * 2n);

      // Proposer was wrong -> cannot claim
      await expect(market.connect(p).claimProposerBond(0n)).to.be.revertedWith("proposer was wrong");

      // Disputer was right -> gets both bonds
      const dBefore = await ethers.provider.getBalance(d.address);
      const tx = await market.connect(d).claimDisputerBond(0n);
      const r = await tx.wait();
      const dAfter = await ethers.provider.getBalance(d.address);
      expect(dAfter - dBefore + r!.gasUsed * r!.gasPrice).to.equal(PROPOSAL_BOND + DISPUTE_BOND);

      // Winning oracles each claim half-slash-pool / 2 (equal weights)
      const expected = (m2.slashPool / 2n) * MIN_STAKE / m2.winningVoteWeight;
      const oBefore = await ethers.provider.getBalance(o1.address);
      const tx2 = await market.connect(o1).claimOracleReward(0n);
      const r2 = await tx2.wait();
      const oAfter = await ethers.provider.getBalance(o1.address);
      expect(oAfter - oBefore + r2!.gasUsed * r2!.gasPrice).to.equal(expected);

      // Losing oracle cannot claim
      await expect(market.connect(o3).claimOracleReward(0n)).to.be.revertedWith("not winning vote");

      // Bob staked NO (winning side) -> claims his stake + loser pool share
      const bobStake = ethers.parseEther("1");
      const totalNo = ethers.parseEther("1");
      const stakerSlashShare = m2.slashPool / 2n;
      const loserPool = ethers.parseEther("3") + stakerSlashShare + BOND;
      const expectedBob = bobStake + (loserPool * bobStake) / totalNo;
      const bBefore = await ethers.provider.getBalance(bob.address);
      const tx3 = await market.connect(bob).claimWinnings(0n);
      const r3 = await tx3.wait();
      const bAfter = await ethers.provider.getBalance(bob.address);
      expect(bAfter - bBefore + r3!.gasUsed * r3!.gasPrice).to.equal(expectedBob);

      // Alice staked YES (losing side) -> cannot claim
      await expect(market.connect(alice).claimWinnings(0n)).to.be.revertedWith("nothing to claim");
    });

    it("disputed proposal but disputer was wrong: proposer takes both bonds", async () => {
      const { registry, market } = await deployStack();
      const [creator, , , p, d, o1, o2] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await registry.connect(o1).register({ value: MIN_STAKE });
      await registry.connect(o2).register({ value: MIN_STAKE });

      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      await market.connect(o1).voteOnDispute(0n, Outcome.YES);
      await market.connect(o2).voteOnDispute(0n, Outcome.YES);

      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.voteDeadline) + 1);
      await market.finalizeMarket(0n);

      const m2 = await market.markets(0n);
      expect(m2.result).to.equal(Outcome.YES);

      const before = await ethers.provider.getBalance(p.address);
      const tx = await market.connect(p).claimProposerBond(0n);
      const r = await tx.wait();
      const after = await ethers.provider.getBalance(p.address);
      expect(after - before + r!.gasUsed * r!.gasPrice).to.equal(PROPOSAL_BOND + DISPUTE_BOND);

      await expect(market.connect(d).claimDisputerBond(0n)).to.be.revertedWith("disputer was wrong");
    });

    it("non-oracle cannot vote", async () => {
      const { market } = await deployStack();
      const [creator, , , p, d, o] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      await expect(market.connect(o).voteOnDispute(0n, Outcome.YES)).to.be.revertedWith("not oracle");
    });

    it("oracle cannot vote twice", async () => {
      const { registry, market } = await deployStack();
      const [creator, , , p, d, o] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await registry.connect(o).register({ value: MIN_STAKE });
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      await market.connect(o).voteOnDispute(0n, Outcome.YES);
      await expect(market.connect(o).voteOnDispute(0n, Outcome.NO)).to.be.revertedWith("already voted");
    });

    it("vote outside window is rejected", async () => {
      const { registry, market } = await deployStack();
      const [creator, , , p, d, o] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await registry.connect(o).register({ value: MIN_STAKE });
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      // not disputed yet
      await expect(market.connect(o).voteOnDispute(0n, Outcome.YES)).to.be.revertedWith("wrong state");
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.voteDeadline) + 1);
      await expect(market.connect(o).voteOnDispute(0n, Outcome.YES)).to.be.revertedWith("vote closed");
    });

    it("dispute window closed -> dispute reverts", async () => {
      const { market } = await deployStack();
      const [creator, , , p, d] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.disputeDeadline) + 1);
      await expect(market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND }))
        .to.be.revertedWith("dispute closed");
    });
  });

  describe("expiry & edge cases", () => {
    it("no proposal by deadline -> Expired, INVALID, stakers refunded with bond", async () => {
      const { market } = await deployStack();
      const [creator, alice, bob] = await ethers.getSigners();
      const { proposalDeadline } = await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("3") });
      await market.connect(bob).stakeNo(0n,   { value: ethers.parseEther("1") });

      await time.increaseTo(proposalDeadline + 1);
      await expect(market.finalizeMarket(0n))
        .to.emit(market, "MarketFinalized").withArgs(0n, Outcome.INVALID);

      const m = await market.markets(0n);
      expect(m.state).to.equal(State.Expired);

      const total = ethers.parseEther("4");
      const expectedAlice = ethers.parseEther("3") + (BOND * ethers.parseEther("3")) / total;
      const before = await ethers.provider.getBalance(alice.address);
      const tx = await market.connect(alice).claimWinnings(0n);
      const r = await tx.wait();
      const after = await ethers.provider.getBalance(alice.address);
      expect(after - before + r!.gasUsed * r!.gasPrice).to.equal(expectedAlice);
    });

    it("cannot finalize twice", async () => {
      const { market } = await deployStack();
      const [creator, , , p] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.disputeDeadline) + 1);
      await market.finalizeMarket(0n);
      await expect(market.finalizeMarket(0n)).to.be.revertedWith("already done");
    });

    it("cannot claim before finalize", async () => {
      const { market } = await deployStack();
      const [creator, alice] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") });
      await expect(market.connect(alice).claimWinnings(0n)).to.be.revertedWith("not finalized");
    });

    it("disputed but no oracle votes -> proposed outcome stands by default", async () => {
      const { market } = await deployStack();
      const [creator, alice, , p, d] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      await market.connect(alice).stakeYes(0n, { value: ethers.parseEther("1") });
      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.voteDeadline) + 1);
      await market.finalizeMarket(0n);
      const m2 = await market.markets(0n);
      expect(m2.result).to.equal(Outcome.YES);
      // Proposer reclaims both bonds (treated as winner since outcome matched proposal)
      // Actually proposedOutcome == result, so disputer "lost".
      await expect(market.connect(d).claimDisputerBond(0n)).to.be.revertedWith("disputer was wrong");
    });

    it("oracle is deactivated after being slashed below MIN_STAKE", async () => {
      const { registry, market } = await deployStack();
      const [creator, , , p, d, o1, o2] = await ethers.getSigners();
      await createDefaultMarket(market, creator);
      // o1 has more weight than o2; both vote — YES wins, o2 gets slashed.
      await registry.connect(o1).register({ value: MIN_STAKE * 2n });
      await registry.connect(o2).register({ value: MIN_STAKE });

      const m0 = await market.markets(0n);
      await time.increaseTo(Number(m0.tradingDeadline) + 1);
      await market.connect(p).proposeOutcome(0n, Outcome.YES, { value: PROPOSAL_BOND });
      await market.connect(d).disputeProposal(0n, { value: DISPUTE_BOND });
      await market.connect(o1).voteOnDispute(0n, Outcome.YES);
      await market.connect(o2).voteOnDispute(0n, Outcome.NO);

      const m1 = await market.markets(0n);
      await time.increaseTo(Number(m1.voteDeadline) + 1);
      await market.finalizeMarket(0n);

      const m2 = await market.markets(0n);
      expect(m2.result).to.equal(Outcome.YES);
      expect(await registry.isOracle(o2.address)).to.be.false; // stake fully slashed
      expect(await registry.stakeOf(o2.address)).to.equal(0n);
    });
  });
});
