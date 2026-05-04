// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

/// @title  Staked Oracle Registry
/// @notice Permissionless registry of oracles that can vote on disputed markets.
///         Each oracle locks `MIN_STAKE` ETH to participate; their voting weight
///         equals their staked balance. Approved markets may slash an oracle's
///         stake (sending the slashed amount to a beneficiary, typically the
///         calling market) when the oracle votes against the final outcome.
contract OracleRegistry {
    address public owner;
    uint256 public constant MIN_STAKE = 0.05 ether;

    mapping(address => uint256) public stakeOf;
    mapping(address => bool)    public isOracle;
    mapping(address => bool)    public approvedSlasher;

    /// 1-based index into `_oracles`; 0 means not present.
    mapping(address => uint256) private _oracleIndex;
    address[] private _oracles;

    uint256 public totalStake;

    event OracleRegistered(address indexed oracle, uint256 stake);
    event OracleUnregistered(address indexed oracle, uint256 returned);
    event StakeIncreased(address indexed oracle, uint256 newStake);
    event Slashed(address indexed oracle, address indexed slasher, uint256 amount, address beneficiary);
    event SlasherApproved(address indexed slasher);
    event SlasherRevoked(address indexed slasher);
    event OwnerTransferred(address indexed prev, address indexed next);

    error NotOwner();
    error NotSlasher();
    error AlreadyRegistered();
    error NotRegistered();
    error InsufficientStake();
    error TransferFailed();

    constructor() {
        owner = msg.sender;
    }

    modifier onlyOwner() {
        if (msg.sender != owner) revert NotOwner();
        _;
    }

    modifier onlySlasher() {
        if (!approvedSlasher[msg.sender]) revert NotSlasher();
        _;
    }

    function transferOwnership(address next) external onlyOwner {
        require(next != address(0), "zero owner");
        emit OwnerTransferred(owner, next);
        owner = next;
    }

    function approveSlasher(address s) external onlyOwner {
        approvedSlasher[s] = true;
        emit SlasherApproved(s);
    }

    function revokeSlasher(address s) external onlyOwner {
        approvedSlasher[s] = false;
        emit SlasherRevoked(s);
    }

    // ---------------------------------------------------------------------
    // Oracle lifecycle
    // ---------------------------------------------------------------------

    function register() external payable {
        if (isOracle[msg.sender]) revert AlreadyRegistered();
        if (msg.value < MIN_STAKE) revert InsufficientStake();
        isOracle[msg.sender] = true;
        stakeOf[msg.sender] = msg.value;
        totalStake += msg.value;
        _oracles.push(msg.sender);
        _oracleIndex[msg.sender] = _oracles.length;
        emit OracleRegistered(msg.sender, msg.value);
    }

    function topUp() external payable {
        if (!isOracle[msg.sender]) revert NotRegistered();
        stakeOf[msg.sender] += msg.value;
        totalStake += msg.value;
        emit StakeIncreased(msg.sender, stakeOf[msg.sender]);
    }

    function unregister() external {
        if (!isOracle[msg.sender]) revert NotRegistered();
        uint256 amount = stakeOf[msg.sender];
        stakeOf[msg.sender] = 0;
        isOracle[msg.sender] = false;
        totalStake -= amount;
        _removeFromList(msg.sender);

        (bool ok, ) = msg.sender.call{value: amount}("");
        if (!ok) revert TransferFailed();
        emit OracleUnregistered(msg.sender, amount);
    }

    // ---------------------------------------------------------------------
    // Slashing (called by approved markets)
    // ---------------------------------------------------------------------

    /// @notice Slash up to `amount` of `oracle`'s stake; transfer the slashed
    ///         ETH to `beneficiary`. Drops oracle below MIN_STAKE → deactivate.
    function slash(address oracle, uint256 amount, address payable beneficiary)
        external
        onlySlasher
        returns (uint256 taken)
    {
        if (!isOracle[oracle]) return 0;
        uint256 s = stakeOf[oracle];
        taken = amount > s ? s : amount;
        if (taken == 0) return 0;

        stakeOf[oracle] = s - taken;
        totalStake -= taken;

        if (stakeOf[oracle] < MIN_STAKE) {
            isOracle[oracle] = false;
            _removeFromList(oracle);
        }

        (bool ok, ) = beneficiary.call{value: taken}("");
        if (!ok) revert TransferFailed();
        emit Slashed(oracle, msg.sender, taken, beneficiary);
    }

    function _removeFromList(address oracle) internal {
        uint256 idx = _oracleIndex[oracle];
        if (idx == 0) return;
        uint256 last = _oracles.length;
        if (idx != last) {
            address swapped = _oracles[last - 1];
            _oracles[idx - 1] = swapped;
            _oracleIndex[swapped] = idx;
        }
        _oracles.pop();
        _oracleIndex[oracle] = 0;
    }

    // ---------------------------------------------------------------------
    // Views
    // ---------------------------------------------------------------------

    function oracleCount() external view returns (uint256) {
        return _oracles.length;
    }

    function getOracles() external view returns (address[] memory) {
        return _oracles;
    }

    function oracleAt(uint256 i) external view returns (address) {
        return _oracles[i];
    }
}
