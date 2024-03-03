#include "bplustree.hpp"
#include "../other/type.hpp"
#include <stdio.h>
#include <vector>

static bool do_writes(btree* bt, int count, std::vector<AdjUnit>& adjinfolist)
{

	for (IndexType i = 0; i < count; i++) {
		// fprintf(stderr, "INFO:put key '%s'\n", (char*)key.data);
		auto rc = btree_put(bt, adjinfolist[i], 0);
		if (!rc) {
			fprintf(stderr, "ERROR:failed to write key '%d'\n", adjinfolist[i].dest);
			return false;
		}
	}

	return true;
}