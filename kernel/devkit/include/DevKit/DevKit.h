#pragma once

#include <sys/queue.h>
#include <ndk/obj.h>

void devkit_init();

typedef struct dev_node {
	obj_header_t hdr;

	char *name;

	struct dev_node *parent;
	TAILQ_ENTRY(dev_node) entry;
	TAILQ_HEAD(children, dev_node) children;
	size_t childcount;
} dev_node_t;

typedef struct dev_plane {
	char *plane_name;
	dev_node_t *plane_root;
} dev_plane_t;

void dev_node_init(dev_node_t *node, const char *name);
dev_node_t *dev_node_create(const char *name);
void dev_node_attach(dev_node_t *node, dev_node_t *parent);

void dev_print_tree(dev_node_t *root);

extern dev_plane_t gDeviceTree;
