/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * glade-name-context.c
 *
 * Copyright (C) 2008 Tristan Van Berkom.
 *
 * Authors:
 *   Tristan Van Berkom <tvb@gnome.org>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdlib.h>

#include "glade-displayable-values.h"


typedef struct {
	gchar *value;
	gchar *string;
} ValueTab;

static GHashTable *values_hash = NULL;


static gint
find_by_value (ValueTab *a, const gchar *b)
{
	return strcmp (a->value, b);
}


static gint
find_by_displayable (ValueTab *a, const gchar *b)
{
	return strcmp (a->string, b);
}

void
glade_register_displayable_value (GType          type, 
				  const gchar   *value, 
				  const gchar   *domain,
				  const gchar   *string)
{
	g_return_if_fail (value && value[0]);
	g_return_if_fail (domain && domain[0]);
	g_return_if_fail (string && string[0]);

	glade_register_translated_value (type, value, dgettext (domain, string));
}

void
glade_register_translated_value (GType          type, 
				 const gchar   *value, 
				 const gchar   *string)
{
	ValueTab *tab;
	gpointer  klass;
	GList    *values;

	g_return_if_fail (value && value[0]);
	g_return_if_fail (string && string[0]);
	klass = g_type_class_ref (type);
	g_return_if_fail (klass != NULL);

	if (!values_hash)
		values_hash = g_hash_table_new (NULL, NULL);

	tab = g_new0 (ValueTab, 1);
	tab->value  = g_strdup (value);
	tab->string = g_strdup (string);

	if ((values = 
	     g_hash_table_lookup (values_hash, klass)) != NULL)
	{
		if (!g_list_find_custom (values, tab->value, (GCompareFunc)find_by_value))
			values = g_list_append (values, tab);
		else
		{
			g_warning ("Already registered displayable value %s for %s (type %s)", 
				   tab->string, tab->value, g_type_name (type));
			g_free (tab->string);
			g_free (tab->value);
			g_free (tab);
		}
	}
	else
	{
		values = g_list_append (NULL, tab);
		g_hash_table_insert (values_hash, klass, values);
	}
	g_type_class_unref (klass);
}

gboolean
glade_type_has_displayable_values (GType type)
{
	gboolean has;
	gpointer klass = g_type_class_ref (type);

	has = values_hash && g_hash_table_lookup (values_hash, klass) != NULL;

	g_type_class_unref (klass);

	return has;
}

G_CONST_RETURN gchar *
glade_get_displayable_value (GType          type, 
			     const gchar   *value)
{
	ValueTab  *tab;
	gpointer   klass;
	GList     *values, *tab_iter;
	gchar     *displayable = NULL;

	g_return_val_if_fail (value && value[0], NULL);

	if (!values_hash)
		return NULL;

	klass = g_type_class_ref (type);

	g_return_val_if_fail  (klass != NULL, NULL);

	if ((values   = g_hash_table_lookup (values_hash, klass)) != NULL &&
	    (tab_iter = g_list_find_custom  (values, value, (GCompareFunc)find_by_value)) != NULL)
	{
		tab = tab_iter->data;
		displayable = tab->string;
	}
	g_type_class_unref (klass);

	return displayable;
}


G_CONST_RETURN gchar *
glade_get_value_from_displayable (GType          type, 
				  const gchar   *displayable)
{
	ValueTab  *tab;
	gpointer   klass;
	GList     *values, *tab_iter;
	gchar     *value = NULL;

	g_return_val_if_fail (displayable && displayable[0], NULL);

	if (!values_hash)
		return NULL;
	
	klass = g_type_class_ref (type);

	g_return_val_if_fail  (klass != NULL, NULL);

	if ((values   = g_hash_table_lookup (values_hash, klass)) != NULL &&
	    (tab_iter = g_list_find_custom  (values, displayable, (GCompareFunc)find_by_displayable)) != NULL)
	{
		tab = tab_iter->data;
		value = tab->value;
	}
	g_type_class_unref (klass);

	return value;
}
