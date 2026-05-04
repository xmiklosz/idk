// SPDX-License-Identifier: MIT
pragma solidity ^0.8.24;

import "@openzeppelin/contracts/utils/ReentrancyGuard.sol";
import "./OracleRegistry.sol";

/// @title  Optimistic Oracle Prediction Market (Polymarket / UMA-style)
/// @notice Binary YES/NO markets resolved by an optimistic oracle:
///           1. After the trading window closes, anyone may PROPOSE an outcome
///              by posting a fixed proposal bond.
///           2. During the dispute window, anyone may DISPUTE the proposal by
///              posting a dispute bond. If undisputed, the proposal stands.
///           3. If disputed, the dispute is resolved by a stake-weighted vote
///              of registered oracles. The losing bond (proposer or disputer)
///              is paid to the winning side. Oracles voting against the final
///              outcome have their registry stake slashed; the slashPool is
///              split between winning oracles and winning stakers.
contract PredictionMarket is ReentrancyGuard {
    enum Outcome { UNRESOLVED, YES, NO, INVALID }
    enum State   { Trading, Proposed, Disputed, Resolved, Expired }

    struct Market {
        address creator;
        string  question;
        // Lifecycle deadlines
        uint256 tradingDeadline;   // staking closes; proposals open after this
        uint256 proposalDeadline;  // someone must propose by this or market expires
        uint256 disputeDeadline;   // dispute window after a proposal (set when proposed)
        uint256 voteDeadline;      // oracle vote window after a dispute (set when disputed)
        // Stakes
        uint256 totalYesStake;
        uint256 totalNoStake;
        uint256 creatorBond;
        // Resolution state
        State    state;
        Outcome  proposedOutcome;
        Outcome  result;
        address  proposer;
        address  disputer;
        // Vote tallies (stake-weighted)
        uint256 yesVoteWeight;
        uint256 noVoteWeight;
        uint256 invalidVoteWeight;
        // Slash accounting
        uint256 slashPool;          // total ETH slashed from losing oracle voters
        uint256 winningVoteWeight;  // total weight on the winning outcome
        // Bond claim tracking
        bool proposerBondClaimed;
        bool disputerBondClaimed;
    }

    uint256 public constant PROPOSAL_BOND  = 0.05 ether;
    uint256 public constant DISPUTE_BOND   = 0.05 ether;
    uint256 public constant DISPUTE_WINDOW = 1 hours;
    uint256 public constant VOTE_WINDOW    = 1 days;

    OracleRegistry public immutable registry;

    uint256 public marketCount;
    mapping(uint256 => Market) public markets;
    mapping(uint256 => mapping(address => uint256)) public yesStakes;
    mapping(uint256 => mapping(address => uint256)) public noStakes;
    mapping(uint256 => mapping(address => bool))    public claimed;

    // Oracle vote state
    mapping(uint256 => mapping(address => Outcome)) public oracleVote;
    mapping(uint256 => mapping(address => uint256)) public oracleVoteWeight; // snapshot
    mapping(uint256 => mapping(address => bool))    public oracleClaimed;
    mapping(uint256 => address[]) private _voters;

    event MarketCreated(uint256 indexed marketId, address indexed creator, string question, uint256 tradingDeadline);
    event Staked(uint256 indexed marketId, address indexed user, bool isYes, uint256 amount);
    event Proposed(uint256 indexed marketId, address indexed proposer, Outcome outcome, uint256 disputeDeadline);
    event Disputed(uint256 indexed marketId, address indexed disputer, uint256 voteDeadline);
    event OracleVoted(uint256 indexed marketId, address indexed oracle, Outcome outcome, uint256 weight);
    event MarketFinalized(uint256 indexed marketId, Outcome result);
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

    function createMarket(
        string calldata question,
        uint256 tradingDeadline,
        uint256 proposalDeadline
    ) external payable returns (uint256 id) {
        require(bytes(question).length > 0, "empty question");
        require(tradingDeadline > block.timestamp, "tradingDeadline in past");
        require(proposalDeadline > tradingDeadline, "proposalDeadline <= tradingDeadline");
        require(msg.value > 0, "no creator bond");

        id = marketCount++;
        Market storage m = markets[id];
        m.creator           = msg.sender;
        m.question          = question;
        m.tradingDeadline   = tradingDeadline;
        m.proposalDeadline  = proposalDeadline;
        m.creatorBond       = msg.value;
        m.state             = State.Trading;
        m.result            = Outcome.UNRESOLVED;
        m.proposedOutcome   = Outcome.UNRESOLVED;

        emit MarketCreated(id, msg.sender, question, tradingDeadline);
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
    // Optimistic oracle: propose / dispute / vote
    // ---------------------------------------------------------------------

    function proposeOutcome(uint256 marketId, Outcome outcome) external payable {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
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
    // Finalization (callable by anyone once the relevant window is past)
    // ---------------------------------------------------------------------

    function finalizeMarket(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.creator != address(0), "no market");
        require(m.state != State.Resolved && m.state != State.Expired, "already done");

        if (m.state == State.Trading) {
            // No proposal arrived in time → market expires as INVALID,
            // creator bond + any value sent on tx returned to stakers pro rata.
            require(block.timestamp >= m.proposalDeadline, "proposal still open");
            m.result = Outcome.INVALID;
            m.state  = State.Expired;
            emit MarketFinalized(marketId, m.result);
            return;
        }

        if (m.state == State.Proposed) {
            // Undisputed → proposed outcome stands.
            require(block.timestamp >= m.disputeDeadline, "dispute still open");
            m.result = m.proposedOutcome;
            m.state  = State.Resolved;
            emit MarketFinalized(marketId, m.result);
            return;
        }

        // m.state == State.Disputed
        require(block.timestamp >= m.voteDeadline, "vote still open");

        Outcome winner = _voteWinner(m);
        if (winner == Outcome.UNRESOLVED) {
            // Nobody voted → proposed outcome stands by default.
            winner = m.proposedOutcome;
        }
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
        return Outcome.INVALID; // ties resolve to INVALID
    }

    /// @dev Slashes 100% of each losing voter's snapshot vote-weight from the
    ///      registry. Slashed ETH is sent to this contract and becomes the
    ///      slashPool, split later between winning voters and winning stakers.
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

    /// @notice So the contract can receive slashed ETH from the registry.
    receive() external payable {}

    // ---------------------------------------------------------------------
    // Claims
    // ---------------------------------------------------------------------

    function claimWinnings(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved || m.state == State.Expired, "not finalized");
        require(!claimed[marketId][msg.sender], "already claimed");

        uint256 payout;

        if (m.state == State.Expired) {
            // Proposal never arrived: full refund + pro-rata creator bond.
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
            // Outcome.NO
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

    /// @notice Proposer reclaims bond. Earns disputer bond too if their
    ///         proposal matched the final result.
    function claimProposerBond(uint256 marketId) external nonReentrant {
        Market storage m = markets[marketId];
        require(m.state == State.Resolved || m.state == State.Expired, "not finalized");
        require(msg.sender == m.proposer, "not proposer");
        require(!m.proposerBondClaimed, "already claimed");

        uint256 payout;
        if (m.state == State.Expired) {
            // Cannot happen: Expired requires no proposal, so m.proposer == 0.
            revert("no proposal");
        }
        if (m.disputer == address(0)) {
            payout = PROPOSAL_BOND; // undisputed → bond returned
        } else if (m.proposedOutcome == m.result) {
            payout = PROPOSAL_BOND + DISPUTE_BOND; // proposer won the dispute
        } else {
            revert("proposer was wrong");
        }

        m.proposerBondClaimed = true;
        (bool ok, ) = msg.sender.call{value: payout}("");
        require(ok, "transfer failed");
        emit ProposerBondClaimed(marketId, msg.sender, payout);
    }

    /// @notice Disputer claims both bonds if the dispute changed the outcome.
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

    /// @notice Oracle voters who voted with the final outcome reclaim their
    ///         snapshot vote-weight (registry stake unchanged) plus a share of
    ///         half of the slash pool, weighted by their vote weight.
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

    function getVoters(uint256 marketId) external view returns (address[] memory) {
        return _voters[marketId];
    }

    function voterCount(uint256 marketId) external view returns (uint256) {
        return _voters[marketId].length;
    }
}
