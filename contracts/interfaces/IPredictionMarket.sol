// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

interface IPredictionMarket {
    enum Outcome { UNRESOLVED, YES, NO, INVALID }

    event MarketCreated(uint256 indexed marketId, address indexed creator, string question, uint256 resolutionTime);
    event Staked(uint256 indexed marketId, address indexed user, bool isYes, uint256 amount);
    event Committed(uint256 indexed marketId, address indexed resolver);
    event Revealed(uint256 indexed marketId, address indexed resolver, Outcome outcome);
    event MarketFinalized(uint256 indexed marketId, Outcome result);
    event Claimed(uint256 indexed marketId, address indexed user, uint256 amount);
    event ResolverRewardClaimed(uint256 indexed marketId, address indexed resolver, uint256 amount);

    function createMarket(
        string calldata question,
        uint256 resolutionTime,
        uint256 commitDeadline,
        uint256 revealDeadline,
        uint256 quorum
    ) external payable returns (uint256);

    function stakeYes(uint256 marketId) external payable;
    function stakeNo(uint256 marketId) external payable;
    function commitResolution(uint256 marketId, bytes32 commitment) external payable;
    function revealResolution(uint256 marketId, Outcome outcome, bytes32 salt) external;
    function finalizeMarket(uint256 marketId) external;
    function claimWinnings(uint256 marketId) external;
    function claimResolverReward(uint256 marketId) external;

    function computeCommitment(Outcome outcome, bytes32 salt) external pure returns (bytes32);
}
