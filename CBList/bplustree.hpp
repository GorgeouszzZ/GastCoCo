#pragma once
#include "../other/type.hpp"
#include <chrono>
/* return codes */
#define BT_FAIL		false
#define BT_SUCCESS	 true

struct btree;
struct btree_txn;
constexpr static auto TIMEOUT = std::chrono::milliseconds(1);

bool btree_txn_put(btree* bt, btree_txn* txn, AdjUnit adjinfo, unsigned int flags);
#define btree_put(bt, adj, flags)	 \
			 btree_txn_put(bt, NULL, adj, flags)