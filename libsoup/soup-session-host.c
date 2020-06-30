/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * soup-session-host.c
 *
 * Copyright 2013 Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "soup-session-host.h"
#include "soup.h"
#include "soup-connection.h"
#include "soup-misc.h"
#include "soup-session-private.h"
#include "soup-socket-private.h"

struct _SoupSessionHost
{
	GObject parent_instance;
};

typedef struct {
	GMutex       mutex;

	SoupURI     *uri;
	SoupAddress *addr;

	GSList      *connections;      /* CONTAINS: SoupConnection */
	guint        num_conns;
	guint        max_conns;

	guint        num_messages;

	GSource     *keep_alive_src;
	SoupSession *session;
} SoupSessionHostPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (SoupSessionHost, soup_session_host, G_TYPE_OBJECT)

enum {
	UNUSED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

#define HOST_KEEP_ALIVE 5 * 60 * 1000 /* 5 min in msecs */

static void
soup_session_host_init (SoupSessionHost *host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);

	g_mutex_init (&priv->mutex);
}

static void
soup_session_host_finalize (GObject *object)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private ((SoupSessionHost*)object);

	g_warn_if_fail (priv->connections == NULL);

	if (priv->keep_alive_src) {
		g_source_destroy (priv->keep_alive_src);
		g_source_unref (priv->keep_alive_src);
	}

	soup_uri_free (priv->uri);
	g_object_unref (priv->addr);

	g_mutex_clear (&priv->mutex);

	G_OBJECT_CLASS (soup_session_host_parent_class)->finalize (object);
}

static void
soup_session_host_class_init (SoupSessionHostClass *host_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (host_class);

	object_class->finalize = soup_session_host_finalize;

	signals[UNUSED] =
		g_signal_new ("unused",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      NULL,
			      G_TYPE_NONE, 0);

}

SoupSessionHost *
soup_session_host_new (SoupSession *session,
		       SoupURI     *uri)
{
	SoupSessionHost *host;
	SoupSessionHostPrivate *priv;

	host = g_object_new (SOUP_TYPE_SESSION_HOST, NULL);
	priv = soup_session_host_get_instance_private (host);

	priv->uri = soup_uri_copy_host (uri);
	priv->addr = g_object_new (SOUP_TYPE_ADDRESS,
				   SOUP_ADDRESS_NAME, priv->uri->host,
				   SOUP_ADDRESS_PORT, priv->uri->port,
				   SOUP_ADDRESS_PROTOCOL, priv->uri->scheme,
				   NULL);
	priv->keep_alive_src = NULL;
	priv->session = session;

	g_object_get (G_OBJECT (session),
		      SOUP_SESSION_MAX_CONNS_PER_HOST, &priv->max_conns,
		      NULL);

	return host;
}

SoupURI *
soup_session_host_get_uri (SoupSessionHost *host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	return priv->uri;
}

SoupAddress *
soup_session_host_get_address (SoupSessionHost *host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	return priv->addr;
}

void
soup_session_host_add_message (SoupSessionHost *host,
			       SoupMessage     *msg)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	priv->num_messages++;
}

void
soup_session_host_remove_message (SoupSessionHost *host,
				  SoupMessage     *msg)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	priv->num_messages--;
}

static gboolean
emit_unused (gpointer host)
{
	g_signal_emit (host, signals[UNUSED], 0);
	return FALSE;
}

static void
connection_disconnected (SoupConnection *conn, gpointer host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);

	g_mutex_lock (&priv->mutex);

	priv->connections = g_slist_remove (priv->connections, conn);
	priv->num_conns--;

	if (priv->num_conns == 0) {
		g_assert (priv->keep_alive_src == NULL);
		priv->keep_alive_src = soup_add_timeout_reffed (soup_session_get_async_context (priv->session),
								HOST_KEEP_ALIVE,
								emit_unused,
								host);
	}

	g_mutex_unlock (&priv->mutex);
}

SoupConnection *
soup_session_host_get_connection (SoupSessionHost *host,
				  gboolean need_new_connection,
				  gboolean at_max_conns,
                                  gboolean ignore_connection_limits,
				  gboolean *try_cleanup)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	SoupConnection *conn;
	GSList *conns;
	int num_pending = 0;
	SoupSocketProperties *socket_props;

	g_mutex_lock (&priv->mutex);

	for (conns = priv->connections; conns; conns = conns->next) {
		conn = conns->data;

		if (!need_new_connection && soup_connection_get_state (conn) == SOUP_CONNECTION_IDLE) {
			soup_connection_set_state (conn, SOUP_CONNECTION_IN_USE);
			g_mutex_unlock (&priv->mutex);
			return conn;
		} else if (soup_connection_get_state (conn) == SOUP_CONNECTION_CONNECTING)
			num_pending++;
	}

	/* Limit the number of pending connections; num_messages / 2
	 * is somewhat arbitrary...
	 */
	if (num_pending > priv->num_messages / 2) {
		g_mutex_unlock (&priv->mutex);
		return NULL;
	}

        if (!ignore_connection_limits) {
                if (priv->num_conns >= priv->max_conns) {
                        if (need_new_connection)
                                *try_cleanup = TRUE;
                        g_mutex_unlock (&priv->mutex);
                        return NULL;
                }

                if (at_max_conns) {
                        *try_cleanup = TRUE;
                        g_mutex_unlock (&priv->mutex);
                        return NULL;
                }
        }

	g_object_get (G_OBJECT (priv->session),
		      SOUP_SESSION_SOCKET_PROPERTIES, &socket_props,
		      NULL);
	conn = g_object_new (SOUP_TYPE_CONNECTION,
			     SOUP_CONNECTION_REMOTE_URI, priv->uri,
                             SOUP_CONNECTION_SSL, priv->uri->scheme == SOUP_URI_SCHEME_HTTPS,
			     SOUP_CONNECTION_SOCKET_PROPERTIES, socket_props,
			     NULL);
	soup_socket_properties_unref (socket_props);

	priv->num_conns++;
	priv->connections = g_slist_prepend (priv->connections, conn);

	g_signal_connect (conn, "disconnected",
			  G_CALLBACK (connection_disconnected),
			  host);

	if (priv->keep_alive_src) {
		g_source_destroy (priv->keep_alive_src);
		g_source_unref (priv->keep_alive_src);
		priv->keep_alive_src = NULL;
	}

	g_mutex_unlock (&priv->mutex);
	return conn;
}

int
soup_session_host_get_num_connections (SoupSessionHost *host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	return priv->num_conns;
}

GSList *
soup_session_host_get_connections (SoupSessionHost *host)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	GSList *conns, *c;

	g_mutex_lock (&priv->mutex);

	conns = NULL;
	for (c = priv->connections; c; c = c->next)
		conns = g_slist_prepend (conns, g_object_ref (c->data));
	conns = g_slist_reverse (conns);

	g_mutex_unlock (&priv->mutex);
	return conns;
}

void
soup_session_host_steal_connection (SoupSessionHost *host,
                                    SoupConnection  *conn)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);

	g_return_if_fail (SOUP_IS_SESSION_HOST (host));
	g_warn_if_fail (g_slist_find (priv->connections, conn) != NULL);

	priv->connections = g_slist_remove (priv->connections, conn);
}

gboolean
soup_session_host_cleanup_connections (SoupSessionHost *host,
				       gboolean cleanup_idle)
{
	SoupSessionHostPrivate *priv = soup_session_host_get_instance_private (host);
	GSList *c, *disconnect_conns;
	SoupConnection *conn;
	SoupConnectionState state;

	disconnect_conns = NULL;

	g_mutex_lock (&priv->mutex);
	for (c = priv->connections; c; c = c->next) {
		conn = c->data;
		state = soup_connection_get_state (conn);
		if (state == SOUP_CONNECTION_REMOTE_DISCONNECTED ||
		    (cleanup_idle && state == SOUP_CONNECTION_IDLE))
			disconnect_conns = g_slist_prepend (disconnect_conns, g_object_ref (conn));
	}
	g_mutex_unlock (&priv->mutex);

	if (!disconnect_conns)
		return FALSE;

	for (c = disconnect_conns; c; c = c->next) {
		conn = c->data;
		soup_connection_disconnect (conn);
		g_object_unref (conn);
	}
	g_slist_free (disconnect_conns);

	return TRUE;
}
