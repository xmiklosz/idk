// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

import "@openzeppelin/contracts/utils/ReentrancyGuard.sol";
import "./OracleRegistry.sol";
import "./interfaces/AggregatorV3Interface.sol";
import "./interfaces/AutomationCompatibleInterface.sol";

/// @title  Prediction market with two resolution paths
/// @notice Hybrid Polymarket / UMA-style market:
///           * MANUAL markets resolve via an optimistic oracle: anyone may
///             propose an outcome with a bond, anyone may dispute with a
///             counter-bond, and disputes escalate to a stake-weighted vote
///             of registered oracles.
///           * PRICE markets resolve trustlessly by reading a Chainlink
///             AggregatorV3 feed at trading-close: YES if the feed price is
///             strictly greater than the threshold, otherwise NO. The contract
///             also implements `AutomationCompatibleInterface` so a Chainlink
///             Automation upkeep can settle these markets the moment they're
///             eligible — no human caller required.
contract PredictionMarket is ReentrancyGuard, AutomationCompatibleInterface {
    enum Outcome    { UNRESOLVED, YES, NO, INVALID }
    enum State      { Trading, Proposed, Disputed, Resolved, Expired }
    enum MarketType { Manual, PriceFeed }

    struct Market {
        address creator;
        string  question;
        string  metadataCID;       // optional IPFS CID for extended metadata
        // Lifecycle deadlines
        uint256 tradingDeadline;
        uint256 proposalDeadline;
        uint256 disputeDeadline;
        uint256 voteDeadline;
        // Stakes
        uint256 totalYesStake;
        uint256 totalNoStake;
        uint256 creatorBond;
        // Resolution state
        State    state;
        MarketType marketType;
        Outcome  proposedOutcome;
        Outcome  result;
        address  proposer;
        address  disputer;
        // Vote tallies (stake-weighted)
        uint256 yesVoteWeight;
        uint256 noVoteWeight;
        uint256 invalidVoteWeight;
        // Slash accounting
        uint256 slashPool;
        uint256 winningVoteWeight;
        // Bond claim tracking
        bool proposerBondClaimed;
        bool disputerBondClaimed;
        // Chainlink price-feed config (PriceFeed markets only)
        address priceFeed;
        int256  priceThreshold;
    }

    uint256 public constant PROPOSAL_BOND   = 0.05 ether;
    uint256 public constant DISPUTE_BOND    = 0.05 ether;
    uint256 public constant DISPUTE_WINDOW  = 1 hours;
    uint256 public constant VOTE_WINDOW     = 1 days;
    uint256 public constant PRICE_STALENESS = 1 hours;

    OracleRegistry public immutable registry;

    uint256 public marketCount;
    mapping(uint256 => Market) public markets;
    mapping(uint256 => mapping(address => uint256)) public yesStakes;
    mapping(uint256 => mapping(address => uint256)) public noStakes;
    mapping(uint256 => mapping(address => bool))    public claimed;

    mapping(uint256 => mapping(address => Outcome)) public oracleVote;
    mapping(uint256 => mapping(address => uint256)) public oracleVoteWeight;
    mapping(uint256 => mapping(address => bool))    public oracleClaimed;
    mapping(uint256 => address[]) private _voters;

    event MarketCreated(uint256 indexed marketId, address indexed creator, string question, uint256 tradingDeadline, MarketType marketType, string metadataCID);
    event Staked(uint256 indexed marketId, address indexed user, bool isYes, uint256 amount);
    event Proposed(uint256 indexed marketId, address indexed proposer, Outcome outcome, uint256 disputeDeadline);
    event Disputed(uint256 indexed marketId, address indexed disputer, uint256 voteDeadline);
    event OracleVoted(uint256 indexed marketId, address indexed oracle, Outcome outcome, uint256 weight);
    event MarketFinalized(uint256 indexed marketId, Outcome result);
    event PriceMarketResolved(uint256 indexed marketId, address indexed feed, int256 price, int256 threshold, Outcome result);
    event Claimed(uint256 indexed marketId, address indexed user, uint256 amount);
    event ProposerBondClaimed(uint256 indexed marketId, address indexed proposer, uint256 amount);
    event DisputerBondClaimed(uint256 indexed marketId, address indexed disputer, uint256 amount);
    event OracleRewardClaimed(uint256 indexed marketId, address indexed oracle, uint256 amount);

    constructor(address oracleRegistry) {
        require(oracleRegistry != address(0), "registry zero");
        registry = OracleRegistry(oracleRegistry);
    }

    // ---------------------------------------------------------------------
    // Market creation & staking
    // ---------------------------------------------------------------------

    /// @notice Create a manual market (resolved by the optimistic oracle).
    function createMarket(
        string calldata question,
        string calldata metadataCID,
        uint256 tradingDeadline,
        uint256 proposalDeadline
    ) external payable returns (uint256 id) {
        return _createMarket(question, metadataCID, tradingDeadline, proposalDeadline, MarketType.Manual, address(0), 0);
    }

    /// @notice Create a price-feed market that auto-resolves from Chainlink.
    /// @dev   Result is YES when feed price > threshold at trading-close.
    function createPriceMarket(
        string calldata question,
        string calldata metadataCID,
        uint256 tradingDeadline,
        uint256 proposalDeadline,
        address priceFeed,
        int256  priceThreshold
    ) external payable returns (uint256 id) {
        require(priceFeed != address(0), "feed zero");
        // Sanity: feed must respond and be priced.
        AggregatorV3Interface(priceFeed).decimals();
        return _createMarket(question, metadataCID, tradingDeadline, proposalDeadline, MarketType.PriceFeed, priceFeed, priceThreshold);
    }

    function _createMarket(
        string calldata question,
        string calldata metadataCID,
        uint256 tradingDeadline,
        uint256 proposalDeadline,
        MarketType marketType,
        address priceFeed,
        int256  priceThreshold
    ) internal returns (uint256 id) {
        require(bytes(question).length > 0, "empty question");
        require(tradingDeadline > block.timestamp, "tradingDeadline in past");
        require(proposalDeadline > tradingDeadline, "proposalDeadline <= tradingDeadline");
        require(msg.value > 0, "no creator bond");

        id = marketCount++;
        Market storage m = markets[id];
        m.creator           = msg.sender;
        m.question          = question;
        m.metadataCID       = metadataCID;
        m.tradingDeadline   = tradingDeadline;
        m.proposalDeadline  = proposalDeadline;
        m.creatorBond       = msg.value;
        m.state             = State.Trading;
        m.marketType        = marketType;
        m.result            = Outcome.UNRESOLVED;
        m.proposedOutcome   = Outcome.UNRESOLVED;
        m.priceFeed         = priceFeed;
        m.priceThreshold    = priceThreshold;

        emit MarketCreated(id, msg.sender, question, tradingDeadline, marketType, metadataCID);
    }

    function stakeYes(uint256 marketId) external payable { _stake(marketId, true); }
    function stakeNo(uint256 marketId) external payable  { _stake(marketId, false); }

    function _stake(uint256 marketId, bool isYes) internal {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(m.state == State.Trading, "trading closed");
        require(block.timestamp < m.tradingDeadline, "trading closed");
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

    // ---------------------------------------------------------------------
    // Chainlink auto-resolution (PriceFeed markets only)
    // ---------------------------------------------------------------------

    /// @notice Resolve a price-feed market trustlessly using its Chainlink feed.
    ///         Callable by anyone after `tradingDeadline`.
    function autoResolve(uint256 marketId) external nonReentrant {
        _autoResolve(marketId);
    }

    function _autoResolve(uint256 marketId) internal {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(m.marketType == MarketType.PriceFeed, "not price market");
        require(m.state == State.Trading, "wrong state");
        require(block.timestamp >= m.tradingDeadline, "trading still open");

        (, int256 price, , uint256 updatedAt, ) = AggregatorV3Interface(m.priceFeed).latestRoundData();
        require(updatedAt > 0, "no price");
        require(block.timestamp - updatedAt <= PRICE_STALENESS, "stale price");

        Outcome r = price > m.priceThreshold ? Outcome.YES : Outcome.NO;
        m.proposedOutcome = r;
        m.result          = r;
        m.state           = State.Resolved;

        emit PriceMarketResolved(marketId, m.priceFeed, price, m.priceThreshold, r);
        emit MarketFinalized(marketId, r);
    }

    // ---------------------------------------------------------------------
    // Chainlink Automation hooks
    //
    // `checkUpkeep` is run off-chain by the Automation network on every
    // block; gas there is irrelevant. It scans for the first eligible price
    // market — one whose trading window has closed and whose feed has fresh
    // data — and returns its id. `performUpkeep` is then executed on-chain
    // by the network's keepers to settle it.
    //
    // The `checkData` argument lets a single registered upkeep restrict its
    // scan to a [start, end) window of market ids; pass empty bytes to scan
    // everything. This keeps gas-free off-chain calls bounded as the number
    // of markets grows.
    // ---------------------------------------------------------------------

    function checkUpkeep(bytes calldata checkData)
        external
        view
        override
        returns (bool upkeepNeeded, bytes memory performData)
    {
        (uint256 start, uint256 end) = _decodeRange(checkData);
        uint256 last = end > marketCount ? marketCount : end;

        for (uint256 i = start; i < last; i++) {
            Market storage m = markets[i];
            if (m.marketType != MarketType.PriceFeed) continue;
            if (m.state != State.Trading)             continue;
            if (block.timestamp < m.tradingDeadline)  continue;

            // Probe the feed; reject stale or absent data so we don't burn
            // gas on a perform that would just revert.
            try AggregatorV3Interface(m.priceFeed).latestRoundData() returns (
                uint80, int256, uint256, uint256 updatedAt, uint80
            ) {
                if (updatedAt == 0) continue;
                if (block.timestamp - updatedAt > PRICE_STALENESS) continue;
                return (true, abi.encode(i));
            } catch {
                continue;
            }
        }
        return (false, "");
    }

    function performUpkeep(bytes calldata performData) external override nonReentrant {
        uint256 marketId = abi.decode(performData, (uint256));
        _autoResolve(marketId);
    }

    function _decodeRange(bytes calldata checkData) internal view returns (uint256 start, uint256 end) {
        if (checkData.length == 0) {
            return (0, marketCount);
        }
        (start, end) = abi.decode(checkData, (uint256, uint256));
    }

    // ---------------------------------------------------------------------
    // Optimistic oracle: propose / dispute / vote (Manual markets only)
    // ---------------------------------------------------------------------

    function proposeOutcome(uint256 marketId, Outcome outcome) external payable {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(m.marketType == MarketType.Manual, "use autoResolve");
        require(m.state == State.Trading, "wrong state");
        require(block.timestamp >= m.tradingDeadline, "propose not open");
        require(block.timestamp <  m.proposalDeadline, "propose closed");
        require(msg.value == PROPOSAL_BOND, "bad proposal bond");
        require(_validOutcome(outcome), "bad outcome");

        m.proposedOutcome = outcome;
        m.proposer        = msg.sender;
        m.state           = State.Proposed;
        m.disputeDeadline = block.timestamp + DISPUTE_WINDOW;

        emit Proposed(marketId, msg.sender, outcome, m.disputeDeadline);
    }

    function disputeProposal(uint256 marketId) external payable {
        Market storage m = markets[marketId];
        require(m.state == State.Proposed, "wrong state");
        require(block.timestamp < m.disputeDeadline, "dispute closed");
        require(msg.value == DISPUTE_BOND, "bad dispute bond");

        m.disputer     = msg.sender;
        m.state        = State.Disputed;
        m.voteDeadline = block.timestamp + VOTE_WINDOW;

        emit Disputed(marketId, msg.sender, m.voteDeadline);
    }

    function voteOnDispute(uint256 marketId, Outcome outcome) external {
        Market storage m = markets[marketId];
        require(m.state == State.Disputed, "wrong state");
        require(block.timestamp < m.voteDeadline, "vote closed");
        require(_validOutcome(outcome), "bad outcome");
        require(registry.isOracle(msg.sender), "not oracle");
        require(oracleVote[marketId][msg.sender] == Outcome.UNRESOLVED, "already voted");

        uint256 weight = registry.stakeOf(msg.sender);
        require(weight > 0, "no stake");

        oracleVote[marketId][msg.sender]       = outcome;
        oracleVoteWeight[marketId][msg.sender] = weight;
        _voters[marketId].push(msg.sender);

        if      (outcome == Outcome.YES)     m.yesVoteWeight     += weight;
        else if (outcome == Outcome.NO)      m.noVoteWeight      += weight;
        else                                 m.invalidVoteWeight += weight;

        emit OracleVoted(marketId, msg.sender, outcome, weight);
    }

    // ---------------------------------------------------------------------
    // Finalization (Manual markets)
    // ---------------------------------------------------------------------

    function finalizeMarket(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(m.state != State.Resolved && m.state != State.Expired, "already done");

        if (m.state == State.Trading) {
            require(block.timestamp >= m.proposalDeadline, "proposal still open");
            m.result = Outcome.INVALID;
            m.state  = State.Expired;
            emit MarketFinalized(marketId, m.result);
            return;
        }

        if (m.state == State.Proposed) {
            require(block.timestamp >= m.disputeDeadline, "dispute still open");
            m.result = m.proposedOutcome;
            m.state  = State.Resolved;
            emit MarketFinalized(marketId, m.result);
            return;
        }

        // m.state == State.Disputed
        require(block.timestamp >= m.voteDeadline, "vote still open");

        Outcome winner = _voteWinner(m);
        if (winner == Outcome.UNRESOLVED) winner = m.proposedOutcome;
        m.result = winner;

        if (m.yesVoteWeight + m.noVoteWeight + m.invalidVoteWeight > 0) {
            _slashLosingVoters(marketId, winner);
        }

        m.state = State.Resolved;
        emit MarketFinalized(marketId, m.result);
    }

    function _voteWinner(Market storage m) internal view returns (Outcome) {
        uint256 y = m.yesVoteWeight;
        uint256 n = m.noVoteWeight;
        uint256 i = m.invalidVoteWeight;
        if (y == 0 && n == 0 && i == 0) return Outcome.UNRESOLVED;
        if (y > n && y > i) return Outcome.YES;
        if (n > y && n > i) return Outcome.NO;
        if (i > y && i > n) return Outcome.INVALID;
        return Outcome.INVALID;
    }

    function _slashLosingVoters(uint256 marketId, Outcome winner) internal {
        Market storage m = markets[marketId];
        address[] storage voters = _voters[marketId];
        uint256 len = voters.length;
        uint256 winningWeight;
        uint256 pool;

        for (uint256 i = 0; i < len; i++) {
            address v = voters[i];
            uint256 w = oracleVoteWeight[marketId][v];
            if (oracleVote[marketId][v] == winner) {
                winningWeight += w;
            } else {
                uint256 taken = registry.slash(v, w, payable(address(this)));
                pool += taken;
            }
        }
        m.slashPool         = pool;
        m.winningVoteWeight = winningWeight;
    }

    function _validOutcome(Outcome o) internal pure returns (bool) {
        return o == Outcome.YES || o == Outcome.NO || o == Outcome.INVALID;
    }

    receive() external payable {}

    // ---------------------------------------------------------------------
    // Claims (shared by both market types)
    // ---------------------------------------------------------------------

    function claimWinnings(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved || m.state == State.Expired, "not finalized");
        require(!claimed[marketId][msg.sender], "already claimed");

        uint256 payout;

        if (m.state == State.Expired) {
            uint256 totalStake = m.totalYesStake + m.totalNoStake;
            require(totalStake > 0, "no stake");
            uint256 myStake = yesStakes[marketId][msg.sender] + noStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            payout = myStake + (m.creatorBond * myStake) / totalStake;

        } else if (m.result == Outcome.INVALID) {
            uint256 totalStake = m.totalYesStake + m.totalNoStake;
            require(totalStake > 0, "no stake");
            uint256 myStake = yesStakes[marketId][msg.sender] + noStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            uint256 stakerShare = m.winningVoteWeight == 0 ? m.slashPool : (m.slashPool / 2);
            uint256 extra = m.creatorBond + stakerShare;
            payout = myStake + (extra * myStake) / totalStake;

        } else if (m.result == Outcome.YES) {
            uint256 myStake = yesStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            uint256 stakerShare = m.winningVoteWeight == 0 ? m.slashPool : (m.slashPool / 2);
            uint256 loserPool = m.totalNoStake + stakerShare + m.creatorBond;
            payout = myStake + (loserPool * myStake) / m.totalYesStake;

        } else {
            uint256 myStake = noStakes[marketId][msg.sender];
            require(myStake > 0, "nothing to claim");
            uint256 stakerShare = m.winningVoteWeight == 0 ? m.slashPool : (m.slashPool / 2);
            uint256 loserPool = m.totalYesStake + stakerShare + m.creatorBond;
            payout = myStake + (loserPool * myStake) / m.totalNoStake;
        }

        claimed[marketId][msg.sender] = true;
        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");
        emit Claimed(marketId, msg.sender, payout);
    }

    function claimProposerBond(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved, "not finalized");
        require(msg.sender == m.proposer, "not proposer");
        require(!m.proposerBondClaimed, "already claimed");

        uint256 payout;
        if (m.disputer == address(0)) {
            payout = PROPOSAL_BOND;
        } else if (m.proposedOutcome == m.result) {
            payout = PROPOSAL_BOND + DISPUTE_BOND;
        } else {
            revert("proposer was wrong");
        }

        m.proposerBondClaimed = true;
        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");
        emit ProposerBondClaimed(marketId, msg.sender, payout);
    }

    function claimDisputerBond(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved, "not finalized");
        require(msg.sender == m.disputer, "not disputer");
        require(!m.disputerBondClaimed, "already claimed");

        if (m.proposedOutcome == m.result) revert("disputer was wrong");

        uint256 payout = PROPOSAL_BOND + DISPUTE_BOND;
        m.disputerBondClaimed = true;
        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");
        emit DisputerBondClaimed(marketId, msg.sender, payout);
    }

    function claimOracleReward(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved, "not finalized");
        require(!oracleClaimed[marketId][msg.sender], "already claimed");
        Outcome v = oracleVote[marketId][msg.sender];
        require(v != Outcome.UNRESOLVED, "did not vote");
        require(v == m.result, "not winning vote");
        require(m.winningVoteWeight > 0, "no winners");

        uint256 weight = oracleVoteWeight[marketId][msg.sender];
        uint256 reward = (m.slashPool / 2) * weight / m.winningVoteWeight;

        oracleClaimed[marketId][msg.sender] = true;
        if (reward > 0) {
            (bool ok, ) = msg.sender.call{value: reward}("");
            require(ok, "transfer failed");
        }
        emit OracleRewardClaimed(marketId, msg.sender, reward);
    }

    // ---------------------------------------------------------------------
    // Views
    // ---------------------------------------------------------------------

    function getVoters(uint256 marketId) external view returns (address[] memory) { return _voters[marketId]; }
    function voterCount(uint256 marketId) external view returns (uint256) { return _voters[marketId].length; }
}
