/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-ntlm-offset: 8 -*- */
/*
 * Copyright (C) 2007 Red Hat, Inc.
 */

#pragma once

#include "soup-connection-auth.h"

G_BEGIN_DECLS

#define SOUP_TYPE_AUTH_NTLM (soup_auth_ntlm_get_type ())
SOUP_AVAILABLE_IN_2_4
G_DECLARE_FINAL_TYPE (SoupAuthNTLM, soup_auth_ntlm, SOUP, AUTH_NTLM, SoupConnectionAuth)

G_END_DECLS
