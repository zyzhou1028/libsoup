/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Red Hat, Inc.
 */

#pragma once

#include "soup-types.h"
#include "soup-auth.h"

G_BEGIN_DECLS

#define SOUP_TYPE_AUTH_MANAGER (soup_auth_manager_get_type ())
SOUP_AVAILABLE_IN_2_4
G_DECLARE_DERIVABLE_TYPE (SoupAuthManager, soup_auth_manager, SOUP, AUTH_MANAGER, GObject)

struct _SoupAuthManagerClass {
	GObjectClass parent_class;

	void (*authenticate) (SoupAuthManager *manager, SoupMessage *msg,
			      SoupAuth *auth, gboolean retrying);
        gpointer padding[4];
};

SOUP_AVAILABLE_IN_2_4
void  soup_auth_manager_use_auth (SoupAuthManager *manager,
				  SoupURI         *uri,
				  SoupAuth        *auth);

SOUP_AVAILABLE_IN_2_58
void soup_auth_manager_clear_cached_credentials (SoupAuthManager *manager);

G_END_DECLS
