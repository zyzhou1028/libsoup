/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Diego Escalante Urrelo
 */

#pragma once

#include "soup-cookie-jar.h"

G_BEGIN_DECLS

#define SOUP_TYPE_COOKIE_JAR_DB (soup_cookie_jar_db_get_type ())
SOUP_AVAILABLE_IN_2_42
G_DECLARE_FINAL_TYPE (SoupCookieJarDB, soup_cookie_jar_db, SOUP, COOKIE_JAR_DB, SoupCookieJar)

#define SOUP_COOKIE_JAR_DB_FILENAME  "filename"

SOUP_AVAILABLE_IN_2_42
SoupCookieJar *soup_cookie_jar_db_new (const char *filename,
				       gboolean    read_only);

G_END_DECLS
