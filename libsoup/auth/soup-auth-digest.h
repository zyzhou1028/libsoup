/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2000-2003, Ximian, Inc.
 */

#pragma once

#include "soup-auth.h"

#define SOUP_TYPE_AUTH_DIGEST (soup_auth_digest_get_type ())
SOUP_AVAILABLE_IN_2_4
G_DECLARE_FINAL_TYPE (SoupAuthDigest, soup_auth_digest, SOUP, AUTH_DIGEST, SoupAuth)
