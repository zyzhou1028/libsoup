/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright 2013 Red Hat, Inc.
 */

#pragma once

#include "soup-types.h"

G_BEGIN_DECLS

#define SOUP_TYPE_SESSION_HOST (soup_session_host_get_type ())
G_DECLARE_FINAL_TYPE (SoupSessionHost, soup_session_host, SOUP, SESSION_HOST, GObject)

GType soup_session_host_get_type (void);

SoupSessionHost *soup_session_host_new                 (SoupSession          *session,
							SoupURI              *uri);

SoupURI         *soup_session_host_get_uri             (SoupSessionHost      *host);
SoupAddress     *soup_session_host_get_address         (SoupSessionHost      *host);

void             soup_session_host_add_message         (SoupSessionHost      *host,
							SoupMessage          *msg);
void             soup_session_host_remove_message      (SoupSessionHost      *host,
							SoupMessage          *msg);

SoupConnection  *soup_session_host_get_connection      (SoupSessionHost      *host,
                                                        gboolean              need_new_connection,
                                                        gboolean              at_max_conns,
                                                        gboolean              ignore_connection_limits,
                                                        gboolean             *try_cleanup);
int              soup_session_host_get_num_connections (SoupSessionHost      *host);
GSList          *soup_session_host_get_connections     (SoupSessionHost      *host);

gboolean         soup_session_host_cleanup_connections (SoupSessionHost      *host,
							gboolean              cleanup_idle);

void             soup_session_host_steal_connection    (SoupSessionHost      *host,
                                                        SoupConnection       *conn);

G_END_DECLS
