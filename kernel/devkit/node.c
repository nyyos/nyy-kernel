#include "ndk/kmem.h"
#include "ndk/ndk.h"
#include "ndk/obj.h"
#include "sys/queue.h"
#include <DevKit/DevKit.h>
#include <assert.h>
#include <string.h>

dev_node_t *dev_node_create(const char *name)
{
	dev_node_t *obj = kmalloc(sizeof(dev_node_t));
	dev_node_init(obj, name);
	return obj;
}

void dev_node_init(dev_node_t *obj, const char *name)
{
	obj_init(&obj->hdr, kObjTypeAnon);
	TAILQ_INIT(&obj->children);
	size_t namelen = strlen(name);
	obj->name = kmalloc(namelen + 1);
	assert(obj->name);
	memcpy(obj->name, name, namelen);
	obj->name[namelen] = '\0';
	obj->childcount = 0;
	obj->parent = nullptr;
}

void dev_node_attach(dev_node_t *node, dev_node_t *parent)
{
	assert(node && parent);
	assert(node->parent == nullptr);
	obj_retain(&node->hdr);
	obj_retain(&parent->hdr);
	TAILQ_INSERT_TAIL(&parent->children, node, entry);
	parent->childcount++;
	node->parent = parent;
	printk(DEBUG "insert node '%s' into parent '%s'\n", node->name,
	       parent->name);
}

// XXX: fix tree printing code
// intermediary branches dont get the |.
// not bothered to fix this rn
static void dev_print_tree_n(dev_node_t *node, int depth)
{
	if (depth > 0) {
		dev_node_t *ancestor = node->parent;
		for (int i = 0; i < depth; i++) {
			ancestor = ancestor->parent;
			if (ancestor && TAILQ_NEXT(ancestor, entry) != NULL) {
				printk("│   ");
			} else {
				if (i == 0)
					continue;
				printk("    ");
			}
		}
		if (TAILQ_NEXT(node, entry) != NULL) {
			printk("├── ");
		} else {
			printk("└── ");
		}
	}

	printk("%s\n", node->name);

	dev_node_t *elm = TAILQ_FIRST(&node->children);
	while (elm) {
		dev_print_tree_n(elm, depth + 1);
		elm = TAILQ_NEXT(elm, entry);
	}
}

void dev_print_tree(dev_node_t *node)
{
	dev_print_tree_n(node, 0);
}
