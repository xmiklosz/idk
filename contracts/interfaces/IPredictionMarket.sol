// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

interface IPredictionMarket {
    enum Outcome    { UNRESOLVED, YES, NO, INVALID }
    enum State      { Trading, Proposed, Disputed, Resolved, Expired }
    enum MarketType { Manual, PriceFeed }

    function createMarket(
        string calldata question,
        string calldata metadataCID,
        uint256 tradingDeadline,
        uint256 proposalDeadline
    ) external payable returns (uint256);

    function createPriceMarket(
        string calldata question,
        string calldata metadataCID,
        uint256 tradingDeadline,
        uint256 proposalDeadline,
        address priceFeed,
        int256 priceThreshold
    ) external payable returns (uint256);

    function autoResolve(uint256 marketId) external;

    function stakeYes(uint256 marketId) external payable;
    function stakeNo(uint256 marketId) external payable;

    function proposeOutcome(uint256 marketId, Outcome outcome) external payable;
    function disputeProposal(uint256 marketId) external payable;
    function voteOnDispute(uint256 marketId, Outcome outcome) external;

    function finalizeMarket(uint256 marketId) external;

    function claimWinnings(uint256 marketId) external;
    function claimProposerBond(uint256 marketId) external;
    function claimDisputerBond(uint256 marketId) external;
    function claimOracleReward(uint256 marketId) external;
}
