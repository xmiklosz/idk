// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

/// @notice Minimal interface for Chainlink Automation (formerly Keepers).
///         Contracts implementing this interface can be registered as an
///         Upkeep on https://automation.chain.link, which will then call
///         `performUpkeep` whenever `checkUpkeep` returns true off-chain.
interface AutomationCompatibleInterface {
    function checkUpkeep(bytes calldata checkData)
        external
        view
        returns (bool upkeepNeeded, bytes memory performData);

    function performUpkeep(bytes calldata performData) external;
}
