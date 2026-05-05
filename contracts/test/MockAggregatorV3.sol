// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

import "../interfaces/AggregatorV3Interface.sol";

/// @notice Test-only mock of a Chainlink V3 aggregator.
contract MockAggregatorV3 is AggregatorV3Interface {
    uint8  public override decimals;
    string public override description;
    uint256 public override version = 4;

    int256  private _answer;
    uint256 private _updatedAt;
    uint80  private _roundId;

    constructor(uint8 _decimals, string memory _description, int256 initial) {
        decimals    = _decimals;
        description = _description;
        _answer     = initial;
        _updatedAt  = block.timestamp;
        _roundId    = 1;
    }

    function setAnswer(int256 newAnswer) external {
        _answer    = newAnswer;
        _updatedAt = block.timestamp;
        _roundId++;
    }

    /// @notice Force a stale `updatedAt`, used to test the staleness guard.
    function setUpdatedAt(uint256 ts) external {
        _updatedAt = ts;
    }

    function latestRoundData()
        external
        view
        override
        returns (uint80, int256, uint256, uint256, uint80)
    {
        return (_roundId, _answer, _updatedAt, _updatedAt, _roundId);
    }
}
