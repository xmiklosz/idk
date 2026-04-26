// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

import "@openzeppelin/contracts/utils/ReentrancyGuard.sol";

/// @title  Commit-Reveal Prediction Market
/// @notice Binary yes/no prediction markets resolved by a quorum of resolvers
///         using a commit-reveal scheme. Resolvers post collateral; minority
///         and non-revealing resolvers are slashed in favour of the majority.
contract PredictionMarket is ReentrancyGuard {
    enum Outcome { UNRESOLVED, YES, NO, INVALID }

    struct Market {
        address creator;
        string  question;
        uint256 resolutionTime;
        uint256 commitDeadline;
        uint256 revealDeadline;
        uint256 totalYesStake;
        uint256 totalNoStake;
        uint256 creatorBond;
        bool    resolved;
        Outcome result;
        uint256 quorum;
        uint256 yesVotes;
        uint256 noVotes;
        uint256 invalidVotes;
        uint256 slashPool;
        uint256 winningResolverCount;
    }

    struct ResolverInfo {
        bytes32 commitment;
        Outcome revealedVote;
        bool    committed;
        bool    revealed;
        bool    rewardClaimed;
        uint256 collateral;
    }

    /// @notice Fixed collateral required to register as a resolver.
    uint256 public constant RESOLVER_COLLATERAL = 0.01 ether;

    uint256 public marketCount;

    mapping(uint256 => Market) public markets;
    mapping(uint256 => mapping(address => uint256))     public yesStakes;
    mapping(uint256 => mapping(address => uint256))     public noStakes;
    mapping(uint256 => mapping(address => ResolverInfo)) public resolverInfo;
    mapping(uint256 => mapping(address => bool))         public claimed;
    mapping(uint256 => address[]) private _resolvers;

    event MarketCreated(uint256 indexed marketId, address indexed creator, string question, uint256 resolutionTime);
    event Staked(uint256 indexed marketId, address indexed user, bool isYes, uint256 amount);
    event Committed(uint256 indexed marketId, address indexed resolver);
    event Revealed(uint256 indexed marketId, address indexed resolver, Outcome outcome);
    event MarketFinalized(uint256 indexed marketId, Outcome result);
    event Claimed(uint256 indexed marketId, address indexed user, uint256 amount);
    event ResolverRewardClaimed(uint256 indexed marketId, address indexed resolver, uint256 amount);

    // ---------------------------------------------------------------------
    // Market lifecycle
    // ---------------------------------------------------------------------

    function createMarket(
        string calldata question,
        uint256 resolutionTime,
        uint256 commitDeadline,
        uint256 revealDeadline,
        uint256 quorum
    ) external payable returns (uint256 id) {
        require(bytes(question).length > 0, "empty question");
        require(resolutionTime > block.timestamp, "resolutionTime in past");
        require(commitDeadline > resolutionTime, "commitDeadline <= resolutionTime");
        require(revealDeadline > commitDeadline, "revealDeadline <= commitDeadline");
        require(quorum > 0, "quorum zero");
        require(msg.value > 0, "no creator bond");

        id = marketCount++;
        Market storage m = markets[id];
        m.creator        = msg.sender;
        m.question       = question;
        m.resolutionTime = resolutionTime;
        m.commitDeadline = commitDeadline;
        m.revealDeadline = revealDeadline;
        m.creatorBond    = msg.value;
        m.quorum         = quorum;
        m.result         = Outcome.UNRESOLVED;

        emit MarketCreated(id, msg.sender, question, resolutionTime);
    }

    function stakeYes(uint256 marketId) external payable {
        _stake(marketId, true);
    }

    function stakeNo(uint256 marketId) external payable {
        _stake(marketId, false);
    }

    function _stake(uint256 marketId, bool isYes) internal {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(block.timestamp < m.resolutionTime, "staking closed");
        require(msg.value > 0, "zero stake");

        if (isYes) {
            yesStakes[marketId][msg.sender] += msg.value;
            m.totalYesStake                 += msg.value;
        } else {
            noStakes[marketId][msg.sender] += msg.value;
            m.totalNoStake                 += msg.value;
        }
        emit Staked(marketId, msg.sender, isYes, msg.value);
    }

    function commitResolution(uint256 marketId, bytes32 commitment) external payable {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(block.timestamp >= m.resolutionTime, "commit not open");
        require(block.timestamp <  m.commitDeadline, "commit closed");
        require(msg.value == RESOLVER_COLLATERAL, "bad collateral");

        ResolverInfo storage r = resolverInfo[marketId][msg.sender];
        require(!r.committed, "already committed");

        r.commitment = commitment;
        r.committed  = true;
        r.collateral = msg.value;
        _resolvers[marketId].push(msg.sender);

        emit Committed(marketId, msg.sender);
    }

    function revealResolution(uint256 marketId, Outcome outcome, bytes32 salt) external {
        Market storage m = markets[marketId];
        require(block.timestamp >= m.commitDeadline, "reveal not open");
        require(block.timestamp <  m.revealDeadline, "reveal closed");
        require(
            outcome == Outcome.YES || outcome == Outcome.NO || outcome == Outcome.INVALID,
            "bad outcome"
        );

        ResolverInfo storage r = resolverInfo[marketId][msg.sender];
        require(r.committed, "no commit");
        require(!r.revealed, "already revealed");
        require(keccak256(abi.encodePacked(outcome, salt)) == r.commitment, "bad reveal");

        r.revealedVote = outcome;
        r.revealed     = true;

        if      (outcome == Outcome.YES)     m.yesVotes++;
        else if (outcome == Outcome.NO)      m.noVotes++;
        else                                 m.invalidVotes++;

        emit Revealed(marketId, msg.sender, outcome);
    }

    function finalizeMarket(uint256 marketId) external {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(!m.resolved, "already resolved");
        require(block.timestamp >= m.revealDeadline, "reveal still open");

        uint256 totalReveals = m.yesVotes + m.noVotes + m.invalidVotes;

        if (totalReveals < m.quorum) {
            // Quorum not met: market is INVALID, only non-revealers slashed.
            m.result    = Outcome.INVALID;
            m.slashPool = _slashNonRevealers(marketId);
            // winningResolverCount stays 0 -> claimResolverReward refunds collateral
            // to all who revealed, regardless of which way they voted.
        } else {
            Outcome winning = _majority(m);
            m.result    = winning;
            m.slashPool = _slashLosers(marketId, winning);
            // winningResolverCount written inside _slashLosers
        }

        m.resolved = true;
        emit MarketFinalized(marketId, m.result);
    }

    function _majority(Market storage m) internal view returns (Outcome) {
        if (m.yesVotes > m.noVotes && m.yesVotes > m.invalidVotes)         return Outcome.YES;
        if (m.noVotes > m.yesVotes && m.noVotes > m.invalidVotes)          return Outcome.NO;
        if (m.invalidVotes > m.yesVotes && m.invalidVotes > m.noVotes)     return Outcome.INVALID;
        return Outcome.INVALID; // ties resolve to INVALID
    }

    function _slashNonRevealers(uint256 marketId) internal returns (uint256 slashed) {
        address[] storage rs = _resolvers[marketId];
        uint256 len = rs.length;
        for (uint256 i = 0; i < len; i++) {
            ResolverInfo storage r = resolverInfo[marketId][rs[i]];
            if (!r.revealed) {
                slashed     += r.collateral;
                r.collateral = 0;
            }
        }
    }

    function _slashLosers(uint256 marketId, Outcome winning) internal returns (uint256 slashed) {
        address[] storage rs = _resolvers[marketId];
        Market storage m = markets[marketId];
        uint256 winners;
        uint256 len = rs.length;
        for (uint256 i = 0; i < len; i++) {
            ResolverInfo storage r = resolverInfo[marketId][rs[i]];
            if (!r.revealed || r.revealedVote != winning) {
                slashed     += r.collateral;
                r.collateral = 0;
            } else {
                winners++;
            }
        }
        m.winningResolverCount = winners;
    }

    // ---------------------------------------------------------------------
    // Claims
    // ---------------------------------------------------------------------

    function claimWinnings(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.resolved, "not finalized");
        require(!claimed[marketId][msg.sender], "already claimed");

        uint256 payout;

        if (m.result == Outcome.INVALID) {
            uint256 totalStake = m.totalYesStake + m.totalNoStake;
            require(totalStake > 0, "no stake");
            uint256 myStake = yesStakes[marketId][msg.sender] + noStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");

            uint256 extra;
            if (m.winningResolverCount == 0) {
                // No-quorum invalidation: stakers absorb the entire slash pool.
                extra = m.creatorBond + m.slashPool;
            } else {
                // Quorum-resolved INVALID: slash pool split with winning resolvers.
                extra = m.creatorBond + (m.slashPool / 2);
            }
            payout = myStake + (extra * myStake) / totalStake;

        } else if (m.result == Outcome.YES) {
            uint256 myStake = yesStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            uint256 stakerSlashShare = m.slashPool / 2;
            uint256 loserPool = m.totalNoStake + stakerSlashShare + m.creatorBond;
            payout = myStake + (loserPool * myStake) / m.totalYesStake;

        } else {
            // Outcome.NO
            uint256 myStake = noStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            uint256 stakerSlashShare = m.slashPool / 2;
            uint256 loserPool = m.totalYesStake + stakerSlashShare + m.creatorBond;
            payout = myStake + (loserPool * myStake) / m.totalNoStake;
        }

        claimed[marketId][msg.sender] = true;

        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");

        emit Claimed(marketId, msg.sender, payout);
    }

    function claimResolverReward(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.resolved, "not finalized");

        ResolverInfo storage r = resolverInfo[marketId][msg.sender];
        require(r.revealed, "did not reveal");
        require(!r.rewardClaimed, "already claimed");

        uint256 payout;
        if (m.result == Outcome.INVALID && m.winningResolverCount == 0) {
            // No-quorum: revealed resolvers simply get their collateral back.
            payout = r.collateral;
        } else if (r.revealedVote == m.result) {
            require(m.winningResolverCount > 0, "no winners");
            uint256 resolverSlashShare = m.slashPool / 2;
            payout = r.collateral + resolverSlashShare / m.winningResolverCount;
        } else {
            revert("not a winning resolver");
        }

        r.rewardClaimed = true;

        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");

        emit ResolverRewardClaimed(marketId, msg.sender, payout);
    }

    // ---------------------------------------------------------------------
    // Views / helpers
    // ---------------------------------------------------------------------

    function getResolvers(uint256 marketId) external view returns (address[] memory) {
        return _resolvers[marketId];
    }

    function resolverCount(uint256 marketId) external view returns (uint256) {
        return _resolvers[marketId].length;
    }

    /// @notice Helper for off-chain code: builds the same commitment hash the
    ///         contract expects for `commitResolution`.
    function computeCommitment(Outcome outcome, bytes32 salt) external pure returns (bytes32) {
        return keccak256(abi.encodePacked(outcome, salt));
    }
}
