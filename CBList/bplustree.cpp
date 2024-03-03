#include "bplustree.hpp"
#include "../other/type.hpp"
#include "../other/futex.hpp"
#include <errno.h>
#include <functional>
#include <stdlib.h>
#include <sys/queue.h>
#include <fcntl.h>
#include <unistd.h>

using pgno_t = uint32_t;
using indx_t = uint16_t;
using bt_cmp_func = std::function<int(const VertexID* a, const VertexID* b)>;
#define F_ISSET(w, f)	 (((w) & (f)) == (f))
#define __packed __attribute__ ((__packed__))

#ifdef PRINT_DEBUG_MESSAGES
# define DPRINTF(...)	do { fprintf(stderr, "%s:%d: ", __func__, __LINE__); \
			     fprintf(stderr, __VA_ARGS__); \
			     fprintf(stderr, "\n"); } while(0)
#else
# define DPRINTF(...)	do { } while(0)
#endif

SIMPLEQ_HEAD(dirty_queue, mpage);

struct node {
#define n_pgno		 p.np_pgno
#define n_dsize		 p.np_dsize
	union {
		pgno_t		 np_pgno;	/* child page number */
		uint32_t	 np_dsize;	/* leaf data size */
	}		 p;
	uint16_t	 ksize;			/* key size */
#define F_BIGDATA	 0x01			/* data put on overflow page */
	uint8_t		 flags;
	char		 data[1];
} __packed;

struct mpage {					/* an in-memory cached page */
	RB_ENTRY(mpage)		 entry;		/* page cache entry */
	SIMPLEQ_ENTRY(mpage)	 next;		/* queue of dirty pages */
	TAILQ_ENTRY(mpage)	 lru_next;	/* LRU queue */
	struct mpage		*parent;	/* NULL if root */
	unsigned int		 parent_index;	/* keep track of node index */
	struct btkey		 prefix;
	struct page		*page;
	pgno_t			 pgno;		/* copy of page->pgno */
	short			 ref;		/* increased by cursors */
	short			 dirty;		/* 1 if on dirty queue */
};

struct btree {
	int			 fd;
	char			*path;
#define BT_FIXPADDING		 0x01		/* internal */
	unsigned int		 flags;
	bt_cmp_func		 cmp;		/* user compare function */
	struct bt_head		 head;
	struct bt_meta		 meta;
	struct page_cache	*page_cache;
	struct lru_queue	*lru_queue;
	struct btree_txn	*txn;		/* current write transaction */
	int			 ref;		/* increased by cursors & txn */
	struct btree_stat	 stat;
	off_t			 size;		/* current file size */
    CoroGraph::Futex  lock;  //livegraph实现方式是 Futex数组放到bplustree中是否有好处? 还是lg好？ 这个好处是？
};

struct btree_txn {
	pgno_t			 root;		/* current / new root page */
	pgno_t			 next_pgno;	/* next unallocated page */
	struct btree		*bt;		/* btree is ref'd */
	dirty_queue* dirty_queue;	/* modified pages */
#define BT_TXN_RDONLY		 0x01		/* read-only transaction */
#define BT_TXN_ERROR		 0x02		/* an error has occurred */
	unsigned int		 flags;
};

static node* btree_search_node(btree* bt, mpage* mp, btval* key, int *exactp, unsigned int *kip)
{
	unsigned int i = 0;
	int low, high;
	int rc = 0;
	node *node;
	struct btval nodekey;

	DPRINTF("searching %lu keys in %s page %u with prefix [%.*s]",
	    NUMKEYS(mp),
	    IS_LEAF(mp) ? "leaf" : "branch",
	    mp->pgno, (int)mp->prefix.len, (char *)mp->prefix.str);

	assert(NUMKEYS(mp) > 0);

	memset(&nodekey, 0, sizeof(nodekey));

	low = IS_LEAF(mp) ? 0 : 1;
	high = NUMKEYS(mp) - 1;
	while (low <= high) {
		i = (low + high) >> 1;
		node = NODEPTR(mp, i);

		nodekey.size = node->ksize;
		nodekey.data = NODEKEY(node);

		if (bt->cmp)
			rc = bt->cmp(key, &nodekey);
		else
			rc = bt_cmp(bt, key, &nodekey, &mp->prefix);

		if (IS_LEAF(mp))
			DPRINTF("found leaf index %u [%.*s], rc = %i",
			    i, (int)nodekey.size, (char *)nodekey.data, rc);
		else
			DPRINTF("found branch index %u [%.*s -> %u], rc = %i",
			    i, (int)node->ksize, (char *)NODEKEY(node),
			    node->n_pgno, rc);

		if (rc == 0)
			break;
		if (rc > 0)
			low = i + 1;
		else
			high = i - 1;
	}

	if (rc > 0) {	/* Found entry is less than the key. */
		i++;	/* Skip to get the smallest entry larger than key. */
		if (i >= NUMKEYS(mp))
			/* There is no entry larger or equal to the key. */
			return NULL;
	}
	if (exactp)
		*exactp = (rc == 0);
	if (kip)	/* Store the key index if requested. */
		*kip = i;

	return NODEPTR(mp, i);
}

static void btree_ref(btree* bt)
{
	bt->ref++;
	DPRINTF("ref is now %d on btree %p", bt->ref, bt);
}

static int btree_read_meta(btree* bt, pgno_t* p_next)
{
	mpage	*mp;
	bt_meta	*meta;
	pgno_t meta_pgno, next_pgno;
	off_t size;

	assert(bt != NULL);

	if ((size = lseek(bt->fd, 0, SEEK_END)) == -1)
		goto fail;

	DPRINTF("btree_read_meta: size = %llu", size);

	if (size < bt->size) {
		DPRINTF("file has shrunk!");
		errno = EIO;
		goto fail;
	}

	if (size == bt->head.psize) {		/* there is only the header */
		if (p_next != NULL)
			*p_next = 1;
		return BT_SUCCESS;		/* new file */
	}

	next_pgno = size / bt->head.psize;
	if (next_pgno == 0) {
		DPRINTF("corrupt file");
		errno = EIO;
		goto fail;
	}

	meta_pgno = next_pgno - 1;

	if (size % bt->head.psize != 0) {
		DPRINTF("filesize not a multiple of the page size!");
		bt->flags |= BT_FIXPADDING;
		next_pgno++;
	}

	if (p_next != NULL)
		*p_next = next_pgno;

	if (size == bt->size) {
		DPRINTF("size unchanged, keeping current meta page");
		if (F_ISSET(bt->meta.flags, BT_TOMBSTONE)) {
			DPRINTF("file is dead");
			errno = ESTALE;
			return BT_FAIL;
		} else
			return BT_SUCCESS;
	}
	bt->size = size;

	while (meta_pgno > 0) {
		if ((mp = btree_get_mpage(bt, meta_pgno)) == NULL)
			break;
		if (btree_is_meta_page(mp->page)) {
			meta = METADATA(mp->page);
			DPRINTF("flags = 0x%x", meta->flags);
			if (F_ISSET(meta->flags, BT_TOMBSTONE)) {
				DPRINTF("file is dead");
				errno = ESTALE;
				return BT_FAIL;
			} else {
				/* Make copy of last meta page. */
				memcpy(&bt->meta, meta, sizeof(bt->meta));
				return BT_SUCCESS;
			}
		}
		--meta_pgno;	/* scan backwards to first valid meta page */
	}

	errno = EIO;
fail:
	if (p_next != NULL)
		*p_next = P_INVALID;
	return BT_FAIL;
}

btree_txn* btree_txn_begin(btree *bt, bool rdonly)
{
	btree_txn* txn;

    //TODO: 这里直接就放弃？ 是不是可以自旋锁或者？
	if (!rdonly && bt->txn != nullptr) {
		DPRINTF("write transaction already begun");
		errno = EBUSY;
		return nullptr;
	}

	if ((txn = (btree_txn*)calloc(1, sizeof(*txn))) == nullptr) {
		DPRINTF("calloc: %s", strerror(errno));
		return nullptr;
	}

	if (rdonly) {
		txn->flags |= BT_TXN_RDONLY;
	} else {

		DPRINTF("taking write lock on txn %p", txn);
		// TODO:lock类型？
        if (!bt->lock.try_lock_for(TIMEOUT))
            throw RollbackExcept("Deadlock on Vertex: " + std::to_string(vertex_id) + ".");
		bt->txn = txn;
	}

	txn->bt = bt;
	btree_ref(bt);

	if (btree_read_meta(bt, &txn->next_pgno) != BT_SUCCESS) {
		btree_txn_abort(txn);
		return NULL;
	}

	txn->root = bt->meta.root;
	DPRINTF("begin transaction on btree %p, root page %u", bt, txn->root);

	return txn;
}

bool btree_txn_put(btree* bt, btree_txn* txn, AdjUnit adjinfo, unsigned int flags)
{
	bool rc = BT_SUCCESS;
    int exact, close_txn = 0;
	unsigned int ki;
	node* leaf;
	mpage* mp;
	struct btval	 xkey;

    /* TODO: optimize */
    // 没啥意义 可以保留
	// if (bt != nullptr && txn != nullptr && bt != txn->bt) {
	// 	errno = EINVAL;
	// 	return BT_FAIL;
	// }

    // txn 当前的情况一直是NULL 暂时无用
	// if (txn != NULL && F_ISSET(txn->flags, BT_TXN_RDONLY)) {
	// 	errno = EINVAL;
	// 	return BT_FAIL;
	// }

    // CBList不可能为空 暂时无用
	// if (bt == NULL) {
	// 	if (txn == NULL) {
	// 		errno = EINVAL;
	// 		return BT_FAIL;
	// 	}
	// 	bt = txn->bt;
	// }
    /* TODO: optimize */

	DPRINTF("==> put key %d, data %d", adjinfo.dest, adjinfo.value);

	if (txn == nullptr) {
		close_txn = 1;
		if ((txn = btree_txn_begin(bt, false)) == nullptr)
			return BT_FAIL;
	}

	rc = btree_search_page(bt, txn, key, NULL, 1, &mp);
	if (rc == BT_SUCCESS) {
		leaf = btree_search_node(bt, mp, key, &exact, &ki);
		if (leaf && exact) {
			if (F_ISSET(flags, BT_NOOVERWRITE)) {
				DPRINTF("duplicate key %.*s",
				    (int)key->size, (char *)key->data);
				errno = EEXIST;
				rc = BT_FAIL;
				goto done;
			}
			btree_del_node(mp, ki);
		}
		if (leaf == NULL) {		/* append if not found */
			ki = NUMKEYS(mp);
			DPRINTF("appending key at index %i", ki);
		}
	} else if (errno == ENOENT) {
		/* new file, just write a root leaf page */
		DPRINTF("allocating new root leaf page");
		if ((mp = btree_new_page(bt, P_LEAF)) == NULL) {
			rc = BT_FAIL;
			goto done;
		}
		txn->root = mp->pgno;
		bt->meta.depth++;
		ki = 0;
	}
	else
		goto done;

	assert(IS_LEAF(mp));
	DPRINTF("there are %lu keys, should insert new key at index %i",
		NUMKEYS(mp), ki);

	/* Copy the key pointer as it is modified by the prefix code. The
	 * caller might have malloc'ed the data.
	 */
	xkey.data = key->data;
	xkey.size = key->size;

	if (SIZELEFT(mp) < bt_leaf_size(bt, key, data)) {
		rc = btree_split(bt, &mp, &ki, &xkey, data, P_INVALID);
	} else {
		/* There is room already in this leaf page. */
		remove_prefix(bt, &xkey, mp->prefix.len);
		rc = btree_add_node(bt, mp, ki, &xkey, data, 0, 0);
	}

	if (rc != BT_SUCCESS)
		txn->flags |= BT_TXN_ERROR;
	else
		bt->meta.entries++;

done:
	if (close_txn) {
		if (rc == BT_SUCCESS)
			rc = btree_txn_commit(txn);
		else
			btree_txn_abort(txn);
	}
	mpage_prune(bt);
	return rc;
}