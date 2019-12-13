// Copyright (c) 2019 The PACGlobal developers
// Copyright (c) 2019 The Extreme Private MasternodeCoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validation.h>

//! For exclusive use of the header event
extern volatile bool fPoSTrigger;

CFeeRate FallbackFee();
CFeeRate MinTxFee();
CFeeRate MinRelayFee();
