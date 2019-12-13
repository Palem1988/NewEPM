// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2019 The Dash Core developers
// Copyright (c) 2018-2019 The PACGlobal developers
// Copyright (c) 2019 The Extreme Private MasternodeCoin developers

// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

const int32_t NEVER32 = 400000U;
const int64_t NEVER64 = 4070908800ULL;

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
	CMutableTransaction txNew;
	txNew.nVersion = 1;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
	txNew.vout[0].nValue = genesisReward;
	txNew.vout[0].scriptPubKey = genesisOutputScript;

	CBlock genesis;
	genesis.nTime = nTime;
	genesis.nBits = nBits;
	genesis.nNonce = nNonce;
	genesis.nVersion = nVersion;
	genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
	genesis.hashPrevBlock.SetNull();
	genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
	return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
	assert(!devNetName.empty());

	CMutableTransaction txNew;
	txNew.nVersion = 1;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	// put height (BIP34) and devnet name into coinbase
	txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
	txNew.vout[0].nValue = genesisReward;
	txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

	CBlock genesis;
	genesis.nTime = nTime;
	genesis.nBits = nBits;
	genesis.nNonce = nNonce;
	genesis.nVersion = 4;
	genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
	genesis.hashPrevBlock = prevBlockHash;
	genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
	return genesis;
}

/**
* Build the genesis block. Note that the output of its generation
* transaction cannot be spent since it did not originally exist in the
* database.
*
* CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
*   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
*     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
*     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
*   vMerkleTree: e0028e
*/
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward, bool fTestnet)
{
	const char* pszTimestamp = fTestnet ?
		"Wired 09/Jan/2014 The Grand Experiment Goes Live: Overstock.com Is Now Accepting Bitcoins" :
		"Study reveals lights on fishnets save turtles,dolphins";
	const CScript genesisOutputScript = fTestnet ?
		CScript() << ParseHex("040184710fa689ad5023690c80f3a49c8f13f8d45b8c857fbcbc8bc4a8e4d3eb4b10f4d4604fa08dce601aaf0f470216fe1b51850b4acf21b179c45070ac7b03a9") << OP_CHECKSIG :
		CScript() << ParseHex("0411345e927d2d3aba81541e23b271f5a9013f2c240fb9bd4b1c14234993639293846cfc74152d293a3bf7ba74592f5f358127cb062a621d3b153089d0b5bb84e5") << OP_CHECKSIG;
	return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
	std::string devNetName = GetDevNetName();
	assert(!devNetName.empty());

	CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

	arith_uint256 bnTarget;
	bnTarget.SetCompact(block.nBits);

	for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
		block.nNonce = nNonce;

		uint256 hash = block.GetHash();
		if (UintToArith256(hash) <= bnTarget)
			return block;
	}

	// This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
	// iteration of the above loop will give a result already
	error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
	assert(false);
}

// this one is for testing only
static Consensus::LLMQParams llmq5_60 = {
	.type = Consensus::LLMQ_5_60,
	.name = "llmq_5_60",
	.size = 3,
	.minSize = 3,
	.threshold = 3,

	.dkgInterval = 24, // one DKG per hour
	.dkgPhaseBlocks = 2,
	.dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
	.dkgMiningWindowEnd = 18,
	.dkgBadVotesThreshold = 8,

	.signingActiveQuorumCount = 2, // just a few ones to allow easier testing

	.keepOldConnections = 3,
};

static Consensus::LLMQParams llmq50_60 = {
	.type = Consensus::LLMQ_50_60,
	.name = "llmq_50_60",
	.size = 50,
	.minSize = 40,
	.threshold = 30,

	.dkgInterval = 24, // one DKG per hour
	.dkgPhaseBlocks = 2,
	.dkgMiningWindowStart = 10, // dkgPhaseBlocks * 5 = after finalization
	.dkgMiningWindowEnd = 18,
	.dkgBadVotesThreshold = 40,

	.signingActiveQuorumCount = 24, // a full day worth of LLMQs

	.keepOldConnections = 25,
};

static Consensus::LLMQParams llmq400_60 = {
	.type = Consensus::LLMQ_400_60,
	.name = "llmq_400_60",
	.size = 400,
	.minSize = 300,
	.threshold = 240,

	.dkgInterval = 24 * 12, // one DKG every 12 hours
	.dkgPhaseBlocks = 4,
	.dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
	.dkgMiningWindowEnd = 28,
	.dkgBadVotesThreshold = 300,

	.signingActiveQuorumCount = 4, // two days worth of LLMQs

	.keepOldConnections = 5,
};

// Used for deployment and min-proto-version signalling, so it needs a higher threshold
static Consensus::LLMQParams llmq400_85 = {
	.type = Consensus::LLMQ_400_85,
	.name = "llmq_400_85",
	.size = 400,
	.minSize = 350,
	.threshold = 340,

	.dkgInterval = 24 * 24, // one DKG every 24 hours
	.dkgPhaseBlocks = 4,
	.dkgMiningWindowStart = 20, // dkgPhaseBlocks * 5 = after finalization
	.dkgMiningWindowEnd = 48, // give it a larger mining window to make sure it is mined
	.dkgBadVotesThreshold = 300,

	.signingActiveQuorumCount = 4, // two days worth of LLMQs

	.keepOldConnections = 5,
};


/**
* Main network
*/
/**
* What makes a good checkpoint block?
* + Is surrounded by blocks with reasonable timestamps
*   (no blocks before with a timestamp after, none after with
*    timestamp before)
* + Contains no strange transactions
*/


class CMainParams : public CChainParams {
public:
	CMainParams() {
		strNetworkID = "main";
		consensus.nSubsidyHalvingInterval = NEVER32;
		consensus.nMasternodePaymentsStartBlock = 500;
		consensus.nMasternodePaymentsIncreaseBlock = NEVER32;
		consensus.nMasternodePaymentsIncreasePeriod = NEVER32;
		consensus.nInstantSendConfirmationsRequired = 6;
		consensus.nInstantSendKeepLock = 24;
		consensus.nInstantSendSigsRequired = 6;
		consensus.nInstantSendSigsTotal = 10;
		consensus.nBudgetPaymentsStartBlock = NEVER32;
		consensus.nBudgetPaymentsCycleBlocks = NEVER32;
		consensus.nBudgetPaymentsWindowBlocks = NEVER32;
		consensus.nSuperblockStartBlock = NEVER32;
		consensus.nSuperblockStartHash = uint256();
		consensus.nSuperblockCycle = NEVER32;
		consensus.nGovernanceMinQuorum = 10;
		consensus.nGovernanceFilterElements = 20000;
       
	   ///////////////////////////////////////////////
        consensus.nGenerationAmount = 50 * COIN;
        consensus.nGenerationHeight = 250;
        ///////////////////////////////////////////////	
		
		consensus.nMasternodeMinimumConfirmations = 15;
		consensus.nMasternodeCollateral = 1300 * COIN;
		consensus.BIP34Height = 1;
		consensus.BIP34Hash = uint256();
		consensus.BIP65Height = 619382;
		consensus.BIP66Height = 3;
		consensus.DIP0001Height = 2;
		consensus.DIP0003Height = 201;
		consensus.DIP0003EnforcementHeight = consensus.nGenerationHeight + 50;
		consensus.DIP0003EnforcementHash = uint256();
		consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.posLimit = uint256S("0007ffff00000000000000000000000000000000000000000000000000000000");
		consensus.nLastPoWBlock = consensus.DIP0003Height - 1;
		consensus.nPowTargetTimespan = 24 * 60 * 60;
		consensus.nPowTargetSpacing = 1 * 60;
		consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
		consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
		consensus.nMinimumStakeValue = 100 * COIN;
		consensus.nStakeMinAge = 60 * 60;
		consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
		consensus.nModifierInterval = 60 * 20;
		consensus.fPowAllowMinDifficultyBlocks = false;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = 20;
		consensus.nPowDGWHeight = 60;
		consensus.nRuleChangeActivationThreshold = 1916;
		consensus.nMinerConfirmationWindow = 2016;

		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1575370800;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = NEVER64;

		// Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1575370800;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = NEVER64;

		// Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1575370800;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = NEVER64;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50;

		// Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1575370800;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = NEVER64;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50;

		// Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1575370810;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 2000;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 1000;

		// Deployment of DIP0008
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1575370810;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 3000;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 1500;

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");  // 332500

																													   // By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

		/**
		* The message start string is designed to be unlikely to occur in normal data.
		* The characters are rarely used upper ASCII, not valid as UTF-8, and produce
		* a large 32-bit integer with any alignment.
		*/
		pchMessageStart[0] = 0xba;
		pchMessageStart[1] = 0xc3;
		pchMessageStart[2] = 0xe9;
		pchMessageStart[3] = 0x7d;
		vAlertPubKey = ParseHex("048403aa4b7052cfca5740f8a11c7390880f2ea482c9119e5a0c3fcd9ec176a295502acd665709f8abd00963667317ba5b6dd47f16a507011649922d99af1647eb");
		nDefaultPort = 1505;
		nPruneAfterHeight = 2000000;

		genesis = CreateGenesisBlock(1575878400, 427681, 0x1e0ffff0, 1, 50 * COIN, false);
		consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("0x0000052da0bf041f23e726a1e2a2e18970b6e92ec87318d15ca61935c340be45"));
		assert(genesis.hashMerkleRoot == uint256S("0xa84706ecbb981aae7a4f701cb0fb93c4536971a13ee9fcea9c2e6a3ba89b32f8"));
		
		
		vSeeds.clear();
		vSeeds.push_back(CDNSSeedData("178.63.174.59", "178.63.174.59"));
		vSeeds.push_back(CDNSSeedData("62.210.188.7", "62.210.188.7"));
		vSeeds.push_back(CDNSSeedData("95.216.19.167", "95.216.19.167"));
		vSeeds.push_back(CDNSSeedData("144.76.2.67", "144.76.2.67"));
		vSeeds.push_back(CDNSSeedData("5.9.40.169", "5.9.40.169  "));
		vSeeds.push_back(CDNSSeedData("95.216.17.97", "95.216.17.97"));
		vSeeds.push_back(CDNSSeedData("85.10.193.18", "85.10.193.18"));
		vSeeds.push_back(CDNSSeedData("95.216.0.167", "95.216.0.167"));
		vSeeds.push_back(CDNSSeedData("38.103.128.98", "38.103.128.98"));
		vSeeds.push_back(CDNSSeedData("148.251.245.229", "148.251.245.229"));
		vSeeds.push_back(CDNSSeedData("5.9.110.248", "5.9.110.248"));

		// EPM addresses start with 'P'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 55);
		// EPM script addresses start with '5'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 10);
		// EPM private keys start with '7' or 'X'
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 204);
		// EPM BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
		// EPM BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

		// EPM BIP44 coin type is '5'
		nExtCoinType = 5;

		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
		consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
		consensus.llmqChainLocks = Consensus::LLMQ_400_60;
		consensus.llmqForInstaEPM = Consensus::LLMQ_50_60;

		fMiningRequiresPeers = true;
		fDefaultConsistencyChecks = false;
		fRequireStandard = true;
		fRequireRoutableExternalIP = true;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = false;
		fAllowMultiplePorts = false;

		nPoolMinParticipants = 3;
		nPoolMaxParticipants = 5;
		nFulfilledRequestExpireTime = 60 * 60; // fulfilled requests expire in 1 hour

		vSporkAddresses = { "PBNaM8CXc6agNpXPw5KFmxf3sCEFVx1o44" };
		nMinSporkKeys = 1;
		fBIP9CheckMasternodesUpgraded = true;

				checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			(0, uint256S("0x0000052da0bf041f23e726a1e2a2e18970b6e92ec87318d15ca61935c340be45"))
			(1, uint256S("0x000000c47f69e764c87202daafcadacf341e378dda22768f0d8b231c7ba4b86f"))
			(77, uint256S("0x00000417612d76cc7f2f43d75f9f00f5a81be8c8e0ad39f9d6dcb2f92a6588bb"))
			(1451, uint256S("0x631ae9dbe12a5da57b680aa5ad95221d1598d2b7f0d78cc0ab8fe87b67cc9611"))
			(2134, uint256S("0xe64f1850596d911edf22086ee7e3eaf492e89dcd097ec1e8fa86598f70a578ee"))
			(2979, uint256S("0x1eac497e868577e99279d1681edcdef34889277c9a2d3674f11cf5092ff50d74"))
			(3663, uint256S("0xefa73709611d28ccab18213fd8bac31a678ad434afa49b825243c32636754845"))
			(4305, uint256S("0x9a7d39f8b07734e7005147f77b4f80b06c4c0cc1a28c925e99c5865badbcf51d"))
			(5019, uint256S("0x625a57347143d422a9bd762aa55a6efaa7a6b181278625281ebbbe04d9195cf5"))
		};

		chainTxData = ChainTxData{
			1576214371, // * UNIX timestamp of last known number of transactions
			16813,    // * total number of transactions between genesis and that timestamp
						//   (the tx=... number in the SetBestChain debug.log lines)
						0.1         // * estimated number of transactions per second after that timestamp
		};
	}
};
static CMainParams mainParams;

/**
* Testnet (v3)
*/
class CTestNetParams : public CChainParams {
public:
	CTestNetParams() {
		strNetworkID = "test";
		consensus.nSubsidyHalvingInterval = NEVER32;
		consensus.nMasternodePaymentsStartBlock = 50;
		consensus.nMasternodePaymentsIncreaseBlock = NEVER32;
		consensus.nMasternodePaymentsIncreasePeriod = NEVER32;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nInstantSendSigsRequired = 2;
		consensus.nInstantSendSigsTotal = 4;
		consensus.nBudgetPaymentsStartBlock = 50;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 100;
		consensus.nSuperblockStartBlock = 100;
		consensus.nSuperblockStartHash = uint256();
		consensus.nSuperblockCycle = 24;
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 500;
		///////////////////////////////////////////////
		consensus.nGenerationAmount = 700000000 * COIN;
		consensus.nGenerationHeight = 80;
		///////////////////////////////////////////////
		consensus.nMasternodeMinimumConfirmations = 15;
		consensus.nMasternodeCollateral = 1000 * COIN;
		consensus.BIP34Height = 1;
		consensus.BIP34Hash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
		consensus.BIP65Height = 0;
		consensus.BIP66Height = 0;
		consensus.DIP0001Height = 1;
		consensus.DIP0003Height = 75;
		consensus.DIP0003EnforcementHeight = consensus.nGenerationHeight + 50;
		consensus.DIP0003EnforcementHash = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");
		consensus.powLimit = uint256S("0000fffff0000000000000000000000000000000000000000000000000000000");
		consensus.posLimit = uint256S("007ffff000000000000000000000000000000000000000000000000000000000");
		consensus.nLastPoWBlock = consensus.DIP0003Height;
		consensus.nPowTargetTimespan = 60;
		consensus.nPowTargetSpacing = 60;
		consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
		consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
		consensus.nMinimumStakeValue = 100 * COIN;
		consensus.nStakeMinAge = 10 * 60;
		consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
		consensus.nModifierInterval = 60 * 20;
		consensus.fPowAllowMinDifficultyBlocks = false;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = NEVER32; // unused
		consensus.nPowDGWHeight = NEVER32; // unused
		consensus.nRuleChangeActivationThreshold = 1512;
		consensus.nMinerConfirmationWindow = 2016;

		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1573325000;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = NEVER64;

		// Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1573325000;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = NEVER64;

		// Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1573325000;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = NEVER64;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50;

		// Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1573325000;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = NEVER64;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50;

		// Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 1000;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 250;

		// Deployment of DIP0008
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 1000;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 250;

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x0000000000000000000000000000000000000000000000000000000000000000");

		pchMessageStart[0] = 0x22;
		pchMessageStart[1] = 0x44;
		pchMessageStart[2] = 0x66;
		pchMessageStart[3] = 0x88;
		vAlertPubKey = ParseHex("04517d8a699cb43d3938d7b24faaff7cda448ca4ea267723ba614784de661949bf632d6304316b244646dea079735b9a6fc4af804efb4752075b9fe2245e14e412");
		nDefaultPort = 29999;
		nPruneAfterHeight = 1000;

		uint32_t nTime = 1573325000;
		uint32_t nNonce = 0;

		if (nNonce == 0) {
			while (UintToArith256(genesis.GetHash()) >
				UintToArith256(consensus.powLimit))
			{
				nNonce++;
				genesis = CreateGenesisBlock(nTime, nNonce, 0x1f00ffff, 1, 0 * COIN, true);
				if (nNonce % 128 == 0) printf("\rgenesis %08x", nNonce);
			}
		}

		genesis = CreateGenesisBlock(nTime, nNonce, 0x1f00ffff, 1, 0 * COIN, true);
		consensus.hashGenesisBlock = genesis.GetHash();

		vFixedSeeds.clear();
		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

		vSeeds.clear();

		// Testnet EPMCoin addresses start with 'y'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 140);
		// Testnet EPMCoin script addresses start with '8' or '9'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
		// Testnet private keys start with '9' or 'c' (Bitcoin defaults)
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		// Testnet EPMCoin BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Testnet EPMCoin BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Testnet EPMCoin BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_5_60] = llmq5_60;
		consensus.llmqChainLocks = Consensus::LLMQ_5_60;
		consensus.llmqForInstaEPM = Consensus::LLMQ_5_60;

		fMiningRequiresPeers = true;
		fDefaultConsistencyChecks = false;
		fRequireStandard = true;
		fRequireRoutableExternalIP = false;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = true;
		fAllowMultiplePorts = true;

		nPoolMinParticipants = 3;
		nPoolMaxParticipants = 5;
		nFulfilledRequestExpireTime = 5 * 60; // fulfilled requests expire in 5 minutes

		vSporkAddresses = { "yTpFjxs3Rtwe7MXfC1i5XACz2K5UYi2GpL" };
		nMinSporkKeys = 1;
		fBIP9CheckMasternodesUpgraded = true;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			(0, consensus.hashGenesisBlock)
		};

		chainTxData = ChainTxData{
			1567342000, // * UNIX timestamp of last known number of transactions
			1,          // * total number of transactions between genesis and that timestamp
						//   (the tx=... number in the SetBestChain debug.log lines)
						1.0         // * estimated number of transactions per second after that timestamp
		};

	}
};
static CTestNetParams testNetParams;

/**
* Devnet
*/
class CDevNetParams : public CChainParams {
public:
	CDevNetParams() {
		strNetworkID = "dev";
		consensus.nSubsidyHalvingInterval = 210240;
		consensus.nMasternodePaymentsStartBlock = 4010; // not true, but it's ok as long as it's less then nMasternodePaymentsIncreaseBlock
		consensus.nMasternodePaymentsIncreaseBlock = 4030;
		consensus.nMasternodePaymentsIncreasePeriod = 10;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nInstantSendSigsRequired = 6;
		consensus.nInstantSendSigsTotal = 10;
		consensus.nBudgetPaymentsStartBlock = 4100;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 10;
		consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
		consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
		consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 500;
		consensus.nMasternodeMinimumConfirmations = 1;
		consensus.nMasternodeCollateral = 500000 * COIN;
		consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
		consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
		consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
		consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
		consensus.DIP0003Height = 2; // DIP0003 activated immediately on devnet
		consensus.DIP0003EnforcementHeight = 2; // DIP0003 activated immediately on devnet
		consensus.DIP0003EnforcementHash = uint256();
		consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
		consensus.posLimit = uint256S("007ffff000000000000000000000000000000000000000000000000000000000");
		consensus.nLastPoWBlock = 100;
		consensus.nPowTargetTimespan = 24 * 60 * 60; // EPMCoin: 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // EPMCoin: 2.5 minutes
		consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
		consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
		consensus.nMinimumStakeValue = 10000 * COIN;
		consensus.nStakeMinAge = 10 * 60;
		consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
		consensus.nModifierInterval = 60 * 20;
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = false;
		consensus.nPowKGWHeight = 4001; // nPowKGWHeight >= nPowDGWHeight means "no KGW"
		consensus.nPowDGWHeight = 4001;
		consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
		consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

																					   // Deployment of BIP68, BIP112, and BIP113.
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1506556800; // September 28th, 2017
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1538092800; // September 28th, 2018

																				 // Deployment of DIP0001
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 1505692800; // Sep 18th, 2017
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 1537228800; // Sep 18th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nThreshold = 50; // 50% of 100

																			   // Deployment of BIP147
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 1517792400; // Feb 5th, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 1549328400; // Feb 5th, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nThreshold = 50; // 50% of 100

																			  // Deployment of DIP0003
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 1535752800; // Sep 1st, 2018
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 1567288800; // Sep 1st, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nThreshold = 50; // 50% of 100

																			   // Deployment of DIP0008
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 1553126400; // Mar 21st, 2019
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 1584748800; // Mar 21st, 2020
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nWindowSize = 100;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nThreshold = 50; // 50% of 100

																			   // The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

		pchMessageStart[0] = 0xe2;
		pchMessageStart[1] = 0xca;
		pchMessageStart[2] = 0xff;
		pchMessageStart[3] = 0xce;
		vAlertPubKey = ParseHex("04517d8a699cb43d3938d7b24faaff7cda448ca4ea267723ba614784de661949bf632d6304316b244646dea079735b9a6fc4af804efb4752075b9fe2245e14e412");
		nDefaultPort = 19999;
		nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN, false);
		consensus.hashGenesisBlock = genesis.GetHash();
		assert(consensus.hashGenesisBlock == uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"));
		assert(genesis.hashMerkleRoot == uint256S("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

		devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
		consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

		vFixedSeeds.clear();
		vSeeds.clear();
		//vSeeds.push_back(CDNSSeedData("epmcoinevo.org",  "devnet-seed.epmcoinevo.org"));

		// Testnet EPMCoin addresses start with 'y'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 140);
		// Testnet EPMCoin script addresses start with '8' or '9'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
		// Testnet private keys start with '9' or 'c' (Bitcoin defaults)
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		// Testnet EPMCoin BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Testnet EPMCoin BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Testnet EPMCoin BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqs[Consensus::LLMQ_400_60] = llmq400_60;
		consensus.llmqs[Consensus::LLMQ_400_85] = llmq400_85;
		consensus.llmqChainLocks = Consensus::LLMQ_50_60;
		consensus.llmqForInstaEPM = Consensus::LLMQ_50_60;

		fMiningRequiresPeers = true;
		fDefaultConsistencyChecks = false;
		fRequireStandard = false;
		fMineBlocksOnDemand = false;
		fAllowMultipleAddressesFromGroup = true;
		fAllowMultiplePorts = true;

		nPoolMinParticipants = 3;
		nPoolMaxParticipants = 5;
		nFulfilledRequestExpireTime = 5 * 60; // fulfilled requests expire in 5 minutes

		vSporkAddresses = { "yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55" };
		nMinSporkKeys = 1;
		// devnets are started with no blocks and no MN, so we can't check for upgraded MN (as there are none)
		fBIP9CheckMasternodesUpgraded = false;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			(0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
				(1, devnetGenesis.GetHash())
		};

		chainTxData = ChainTxData{
			devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
			2,                            // * we only have 2 coinbase transactions when a devnet is started up
			0.01                          // * estimated number of transactions per second
		};
	}

	void UpdateSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
	{
		consensus.nMinimumDifficultyBlocks = nMinimumDifficultyBlocks;
		consensus.nHighSubsidyBlocks = nHighSubsidyBlocks;
		consensus.nHighSubsidyFactor = nHighSubsidyFactor;
	}

	void UpdateLLMQChainLocks(Consensus::LLMQType llmqType) {
		consensus.llmqChainLocks = llmqType;
	}
};
static CDevNetParams *devNetParams;


/**
* Regression test
*/
class CRegTestParams : public CChainParams {
public:
	CRegTestParams() {
		strNetworkID = "regtest";
		consensus.nSubsidyHalvingInterval = 150;
		consensus.nMasternodePaymentsStartBlock = 240;
		consensus.nMasternodePaymentsIncreaseBlock = 350;
		consensus.nMasternodePaymentsIncreasePeriod = 10;
		consensus.nInstantSendConfirmationsRequired = 2;
		consensus.nInstantSendKeepLock = 6;
		consensus.nInstantSendSigsRequired = 3;
		consensus.nInstantSendSigsTotal = 5;
		consensus.nBudgetPaymentsStartBlock = 1000;
		consensus.nBudgetPaymentsCycleBlocks = 50;
		consensus.nBudgetPaymentsWindowBlocks = 10;
		consensus.nSuperblockStartBlock = 1500;
		consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
		consensus.nSuperblockCycle = 10;
		consensus.nGovernanceMinQuorum = 1;
		consensus.nGovernanceFilterElements = 100;
		consensus.nMasternodeMinimumConfirmations = 1;
		consensus.nMasternodeCollateral = 500000 * COIN;
		consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
		consensus.BIP34Hash = uint256();
		consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
		consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
		consensus.DIP0001Height = 2000;
		consensus.DIP0003Height = 432;
		consensus.DIP0003EnforcementHeight = 500;
		consensus.DIP0003EnforcementHash = uint256();
		consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
		consensus.posLimit = uint256S("007ffff000000000000000000000000000000000000000000000000000000000");
		consensus.nLastPoWBlock = 100;
		consensus.nPowTargetTimespan = 24 * 60 * 60; // EPMCoin: 1 day
		consensus.nPowTargetSpacing = 2.5 * 60; // EPMCoin: 2.5 minutes
		consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
		consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
		consensus.nStakeMinAge = 10 * 60;
		consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
		consensus.nModifierInterval = 60 * 20;
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = true;
		consensus.nPowKGWHeight = 15200; // same as mainnet
		consensus.nPowDGWHeight = 34140; // same as mainnet
		consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
		consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].bit = 1;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0001].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].bit = 2;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_BIP147].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].bit = 3;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0003].nTimeout = 999999999999ULL;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].bit = 4;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nStartTime = 0;
		consensus.vDeployments[Consensus::DEPLOYMENT_DIP0008].nTimeout = 999999999999ULL;

		// The best chain should have at least this much work.
		consensus.nMinimumChainWork = uint256S("0x00");

		// By default assume that the signatures in ancestors of this block are valid.
		consensus.defaultAssumeValid = uint256S("0x00");

		pchMessageStart[0] = 0xfc;
		pchMessageStart[1] = 0xc1;
		pchMessageStart[2] = 0xb7;
		pchMessageStart[3] = 0xdc;
		nDefaultPort = 19994;
		nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN, false);
		consensus.hashGenesisBlock = genesis.GetHash();
		// assert(consensus.hashGenesisBlock == uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"));
		// assert(genesis.hashMerkleRoot == uint256S("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

		vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
		vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = true;
		fRequireStandard = false;
		fRequireRoutableExternalIP = false;
		fMineBlocksOnDemand = true;
		fAllowMultipleAddressesFromGroup = true;
		fAllowMultiplePorts = true;

		nFulfilledRequestExpireTime = 5 * 60; // fulfilled requests expire in 5 minutes

											  // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
		vSporkAddresses = { "yj949n1UH6fDhw6HtVE5VMj2iSTaSWBMcW" };
		nMinSporkKeys = 1;
		// regtest usually has no masternodes in most tests, so don't check for upgraged MNs
		fBIP9CheckMasternodesUpgraded = false;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			(0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
		};

		chainTxData = ChainTxData{
			0,
			0,
			0
		};

		// Regtest EPMCoin addresses start with 'y'
		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 140);
		// Regtest EPMCoin script addresses start with '8' or '9'
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
		// Regtest private keys start with '9' or 'c' (Bitcoin defaults)
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
		// Regtest EPMCoin BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		// Regtest EPMCoin BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

		// Regtest EPMCoin BIP44 coin type is '1' (All coin's testnet default)
		nExtCoinType = 1;

		// long living quorum params
		consensus.llmqs[Consensus::LLMQ_5_60] = llmq5_60;
		consensus.llmqs[Consensus::LLMQ_50_60] = llmq50_60;
		consensus.llmqChainLocks = Consensus::LLMQ_5_60;
		consensus.llmqForInstaEPM = Consensus::LLMQ_5_60;
	}

	void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nWindowSize, int64_t nThreshold)
	{
		consensus.vDeployments[d].nStartTime = nStartTime;
		consensus.vDeployments[d].nTimeout = nTimeout;
		if (nWindowSize != -1) {
			consensus.vDeployments[d].nWindowSize = nWindowSize;
		}
		if (nThreshold != -1) {
			consensus.vDeployments[d].nThreshold = nThreshold;
		}
	}

	void UpdateDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
	{
		consensus.DIP0003Height = nActivationHeight;
		consensus.DIP0003EnforcementHeight = nEnforcementHeight;
	}

	void UpdateBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
	{
		consensus.nMasternodePaymentsStartBlock = nMasternodePaymentsStartBlock;
		consensus.nBudgetPaymentsStartBlock = nBudgetPaymentsStartBlock;
		consensus.nSuperblockStartBlock = nSuperblockStartBlock;
	}
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
	assert(pCurrentParams);
	return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
	if (chain == CBaseChainParams::MAIN)
		return mainParams;
	else if (chain == CBaseChainParams::TESTNET)
		return testNetParams;
	else if (chain == CBaseChainParams::DEVNET) {
		assert(devNetParams);
		return *devNetParams;
	}
	else if (chain == CBaseChainParams::REGTEST)
		return regTestParams;
	else
		throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
	if (network == CBaseChainParams::DEVNET) {
		devNetParams = (CDevNetParams*)new uint8_t[sizeof(CDevNetParams)];
		memset(devNetParams, 0, sizeof(CDevNetParams));
		new (devNetParams) CDevNetParams();
	}

	SelectBaseParams(network);
	pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int64_t nWindowSize, int64_t nThreshold)
{
	regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout, nWindowSize, nThreshold);
}

void UpdateRegtestDIP3Parameters(int nActivationHeight, int nEnforcementHeight)
{
	regTestParams.UpdateDIP3Parameters(nActivationHeight, nEnforcementHeight);
}

void UpdateRegtestBudgetParameters(int nMasternodePaymentsStartBlock, int nBudgetPaymentsStartBlock, int nSuperblockStartBlock)
{
	regTestParams.UpdateBudgetParameters(nMasternodePaymentsStartBlock, nBudgetPaymentsStartBlock, nSuperblockStartBlock);
}

void UpdateDevnetSubsidyAndDiffParams(int nMinimumDifficultyBlocks, int nHighSubsidyBlocks, int nHighSubsidyFactor)
{
	assert(devNetParams);
	devNetParams->UpdateSubsidyAndDiffParams(nMinimumDifficultyBlocks, nHighSubsidyBlocks, nHighSubsidyFactor);
}

void UpdateDevnetLLMQChainLocks(Consensus::LLMQType llmqType)
{
	assert(devNetParams);
	devNetParams->UpdateLLMQChainLocks(llmqType);
}
