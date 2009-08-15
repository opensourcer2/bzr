/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 *   Tristan Van Berkom <tristan.van.berkom@gmail.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
 * SECTION:glade-property-class
 * @Title: GladePropertyClass
 * @Short_Description: Property Class-wide metadata.
 *
 * #GladePropertyClass is a structure based on a #GParamSpec and parameters
 * from the Glade catalog files and describes how properties are to be handled
 * in Glade; it also provides an interface to convert #GValue to strings and
 * va_lists etc (back and forth).
 */

#include <string.h>
#include <stdlib.h>
#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include "glade.h"
#include "glade-widget.h"
#include "glade-parameter.h"
#include "glade-property.h"
#include "glade-property-class.h"
#include "glade-editor-property.h"
#include "glade-displayable-values.h"
#include "glade-debug.h"

#define NUMERICAL_STEP_INCREMENT   1.0F
#define NUMERICAL_PAGE_INCREMENT   10.0F
#define NUMERICAL_PAGE_SIZE        0.0F

#define FLOATING_STEP_INCREMENT    0.01F
#define FLOATING_PAGE_INCREMENT    0.1F
#define FLOATING_PAGE_SIZE         0.00F

/**
 * glade_property_class_new:
 * @handle: A generic pointer (i.e. a #GladeWidgetClass)
 *
 * Returns: a new #GladePropertyClass
 */
GladePropertyClass *
glade_property_class_new (gpointer handle)
{
	GladePropertyClass *property_class;

	property_class = g_new0 (GladePropertyClass, 1);
	property_class->handle = handle;
	property_class->pspec = NULL;
	property_class->id = NULL;
	property_class->name = NULL;
	property_class->tooltip = NULL;
	property_class->def = NULL;
	property_class->orig_def = NULL;
	property_class->parameters = NULL;
	property_class->query = FALSE;
	property_class->optional = FALSE;
	property_class->optional_default = FALSE;
	property_class->is_modified = FALSE;
	property_class->common = FALSE;
	property_class->packing = FALSE;
	property_class->atk = FALSE;
	property_class->visible = TRUE;
	property_class->custom_layout = FALSE;
	property_class->save = TRUE;
	property_class->save_always = FALSE;
	property_class->ignore = FALSE;
	property_class->needs_sync = FALSE;
	property_class->themed_icon = FALSE;
	property_class->stock = FALSE;
	property_class->stock_icon = FALSE;
	property_class->translatable = FALSE;
	property_class->virt = TRUE;
	property_class->transfer_on_paste = FALSE;
	property_class->weight = -1.0;
	property_class->libglade_only = FALSE;
	property_class->libglade_unsupported = FALSE;
	property_class->parentless_widget = FALSE;

	/* Initialize them to the base version */
	property_class->version_since_major = GWA_VERSION_SINCE_MAJOR (handle);
	property_class->version_since_minor = GWA_VERSION_SINCE_MINOR (handle);

	property_class->builder_since_major = GWA_BUILDER_SINCE_MAJOR (handle);
	property_class->builder_since_minor = GWA_BUILDER_SINCE_MINOR (handle);

	return property_class;
}

/**
 * glade_property_class_clone:
 * @property_class: a #GladePropertyClass
 *
 * Returns: a new #GladePropertyClass cloned from @property_class
 */
GladePropertyClass *
glade_property_class_clone (GladePropertyClass *property_class)
{
	GladePropertyClass *clone;

	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (property_class), NULL);

	clone = g_new0 (GladePropertyClass, 1);

	/* copy ints over */
	memcpy (clone, property_class, sizeof(GladePropertyClass));

	/* Make sure we own our strings */
	clone->pspec = property_class->pspec;
	clone->id = g_strdup (clone->id);
	clone->name = g_strdup (clone->name);
	clone->tooltip = g_strdup (clone->tooltip);

	if (G_IS_VALUE (property_class->def))
	{
		clone->def = g_new0 (GValue, 1);
		g_value_init (clone->def, property_class->pspec->value_type);
		g_value_copy (property_class->def, clone->def);
	}

	if (G_IS_VALUE (property_class->orig_def))
	{
		clone->orig_def = g_new0 (GValue, 1);
		g_value_init (clone->orig_def, property_class->pspec->value_type);
		g_value_copy (property_class->orig_def, clone->orig_def);
	}

	if (clone->parameters)
	{
		GList *parameter;

		clone->parameters = g_list_copy (clone->parameters);

		for (parameter = clone->parameters;
		     parameter != NULL;
		     parameter = parameter->next)
			parameter->data =
				glade_parameter_clone ((GladeParameter*) parameter->data);
	}
	return clone;
}

/**
 * glade_property_class_free:
 * @property_class: a #GladePropertyClass
 *
 * Frees @klass and its associated memory.
 */
void
glade_property_class_free (GladePropertyClass *property_class)
{
	if (property_class == NULL)
		return;

	g_return_if_fail (GLADE_IS_PROPERTY_CLASS (property_class));

	g_free (property_class->id);
	g_free (property_class->tooltip);
	g_free (property_class->name);
	if (property_class->orig_def)
	{
		if (G_VALUE_TYPE (property_class->orig_def) != 0)
			g_value_unset (property_class->orig_def);
		g_free (property_class->orig_def);
	}
	if (property_class->def)
	{
		if (G_VALUE_TYPE (property_class->def) != 0)
			g_value_unset (property_class->def);
		g_free (property_class->def);
	}
	g_list_foreach (property_class->parameters, (GFunc) glade_parameter_free, NULL);
	g_list_free (property_class->parameters);
	g_free (property_class);
}


static GValue *
glade_property_class_get_default_from_spec (GParamSpec *spec)
{
	GValue *value;
	value = g_new0 (GValue, 1);
	g_value_init (value, spec->value_type);
	g_param_value_set_default (spec, value);
	return value;
}


static gchar *
glade_property_class_make_string_from_enum (GType etype, gint eval)
{
	GEnumClass *eclass;
	gchar      *string = NULL;
	guint       i;

	g_return_val_if_fail ((eclass = g_type_class_ref (etype)) != NULL, NULL);
	for (i = 0; i < eclass->n_values; i++)
	{
		if (eval == eclass->values[i].value)
		{
			string = g_strdup (eclass->values[i].value_nick);
			break;
		}
	}
	g_type_class_unref (eclass);
	return string;
}

static gchar *
glade_property_class_make_string_from_flags (GladePropertyClass *klass, guint fvals, gboolean displayables)
{
	GFlagsClass *fclass;
	GFlagsValue *fvalue;
	GString *string;
	gchar *retval;
	
	g_return_val_if_fail ((fclass = g_type_class_ref (klass->pspec->value_type)) != NULL, NULL);
	
	string = g_string_new("");
	
	while ((fvalue = g_flags_get_first_value(fclass, fvals)) != NULL)
	{
		const gchar *val_str = NULL;
		
		fvals &= ~fvalue->value;
		
		if (displayables)
			val_str = glade_get_displayable_value (klass->pspec->value_type, 
							       fvalue->value_name);
		
		if (string->str[0])
			g_string_append(string, " | ");
		
		g_string_append (string, (val_str) ? val_str : fvalue->value_name);
		
		/* If one of the flags value is 0 this loop become infinite :) */
		if (fvalue->value == 0) break;
	}
	
	retval = string->str;
	
	g_type_class_unref (fclass);
	g_string_free(string, FALSE);
	
	return retval;
}

static gchar *
glade_property_class_make_string_from_object (GladePropertyClass *property_class,
					      GObject            *object,
					      GladeProjectFormat  fmt)
{
	GladeWidget *gwidget;
	gchar       *string = NULL, *filename;

	if (!object) return NULL;

	if (property_class->pspec->value_type == GDK_TYPE_PIXBUF)
	{
		if ((filename = g_object_get_data (object, "GladeFileName")) != NULL)
			string = g_strdup (filename);
	}
	else if (fmt == GLADE_PROJECT_FORMAT_LIBGLADE &&
		 property_class->pspec->value_type == GTK_TYPE_ADJUSTMENT)
	{
		GtkAdjustment *adj = GTK_ADJUSTMENT (object);
		GString       *str = g_string_sized_new (G_ASCII_DTOSTR_BUF_SIZE * 6 + 6);
		gchar          buff[G_ASCII_DTOSTR_BUF_SIZE];

		g_ascii_dtostr (buff, sizeof (buff), adj->value);
		g_string_append (str, buff);

		g_string_append_c (str, ' ');
		g_ascii_dtostr (buff, sizeof (buff), adj->lower);
		g_string_append (str, buff);

		g_string_append_c (str, ' ');
		g_ascii_dtostr (buff, sizeof (buff), adj->upper);
		g_string_append (str, buff);

		g_string_append_c (str, ' ');
		g_ascii_dtostr (buff, sizeof (buff), adj->step_increment);
		g_string_append (str, buff);

		g_string_append_c (str, ' ');
		g_ascii_dtostr (buff, sizeof (buff), adj->page_increment);
		g_string_append (str, buff);

		g_string_append_c (str, ' ');
		g_ascii_dtostr (buff, sizeof (buff), adj->page_size);
		g_string_append (str, buff);

		string = g_string_free (str, FALSE);
	}
	else if ((gwidget = glade_widget_get_from_gobject (object)) != NULL)
		string = g_strdup (gwidget->name);
	else
		g_critical ("Object type property refers to an object "
			    "outside the project");
	
	return string;
}

static gchar *
glade_property_class_make_string_from_objects (GladePropertyClass *property_class,
					       GList              *objects,
					       GladeProjectFormat  fmt)
{
	GObject *object;
	GList   *list;
	gchar   *string = NULL, *obj_str, *tmp;

	for (list = objects; list; list = list->next)
	{
		object = list->data;

		obj_str = glade_property_class_make_string_from_object 
			(property_class, object, fmt);

		if (string == NULL)
			string = obj_str;
		else if (obj_str != NULL)
		{
			tmp = g_strdup_printf ("%s%s%s", string, GPC_OBJECT_DELIMITER, obj_str);
			string = (g_free (string), tmp);
			g_free (obj_str);
		}
	}
	return string;
}

/**
 * glade_property_class_make_string_from_gvalue:
 * @property_class: A #GladePropertyClass
 * @value: A #GValue
 * @fmt: The #GladeProjectFormat the string should conform to
 *
 * Returns: A newly allocated string representation of @value
 */
gchar *
glade_property_class_make_string_from_gvalue (GladePropertyClass *property_class,
					      const GValue       *value,
					      GladeProjectFormat  fmt)
{
	gchar    *string = NULL, **strv, str[G_ASCII_DTOSTR_BUF_SIZE];
	GObject  *object;
	GdkColor *color;
	GList    *objects;

	if (G_IS_PARAM_SPEC_ENUM (property_class->pspec))
	{
		gint eval = g_value_get_enum (value);
		string = glade_property_class_make_string_from_enum
			(property_class->pspec->value_type, eval);
	}
	else if (G_IS_PARAM_SPEC_FLAGS (property_class->pspec))
	{
		guint flags = g_value_get_flags (value);
		string = glade_property_class_make_string_from_flags
			(property_class, flags, FALSE);
	}
	else if (G_IS_PARAM_SPEC_VALUE_ARRAY (property_class->pspec))
	{
		GValueArray *value_array = g_value_get_boxed (value);
		
		if (value_array && value_array->n_values &&
		   G_VALUE_HOLDS (&value_array->values [0], G_TYPE_STRING))
		{
			gint i, n_values = value_array->n_values;
			GString *gstring = g_string_new (NULL);
			
			for (i = 0; i < n_values; i++)
			{
				g_string_append (gstring, g_value_get_string (&value_array->values [i]));
				g_string_append_c (gstring, '\n');
			}
			string = gstring->str;
			g_string_free (gstring, FALSE);
		}
	}
	else if (G_IS_PARAM_SPEC_BOXED (property_class->pspec))
	{
		if (property_class->pspec->value_type == GDK_TYPE_COLOR)
		{
			color  = g_value_get_boxed (value);
			if (color)
				string = g_strdup_printf ("#%04x%04x%04x", 
							  color->red, 
							  color->green,
							  color->blue);
		}
		else if (property_class->pspec->value_type == G_TYPE_STRV)
		{
			strv = g_value_get_boxed (value);
			if (strv) string = g_strjoinv ("\n", strv);
		}
	}
	else if (G_IS_PARAM_SPEC_INT(property_class->pspec))
		string = g_strdup_printf ("%d", g_value_get_int (value));
	else if (G_IS_PARAM_SPEC_UINT(property_class->pspec))
		string = g_strdup_printf ("%u", g_value_get_uint (value));
	else if (G_IS_PARAM_SPEC_LONG(property_class->pspec))
		string = g_strdup_printf ("%ld", g_value_get_long (value));
	else if (G_IS_PARAM_SPEC_ULONG(property_class->pspec))
		string = g_strdup_printf ("%lu", g_value_get_ulong (value));
	else if (G_IS_PARAM_SPEC_INT64(property_class->pspec))
		string = g_strdup_printf ("%" G_GINT64_FORMAT, g_value_get_int64 (value));
	else if (G_IS_PARAM_SPEC_UINT64(property_class->pspec))
		string = g_strdup_printf ("%" G_GUINT64_FORMAT, g_value_get_uint64 (value));
	else if (G_IS_PARAM_SPEC_FLOAT(property_class->pspec))
	{
		g_ascii_dtostr (str, sizeof (str), g_value_get_float (value));
		string = g_strdup (str);
	}
	else if (G_IS_PARAM_SPEC_DOUBLE(property_class->pspec))
	{
		g_ascii_dtostr (str, sizeof (str), g_value_get_double (value));
		string = g_strdup (str);
	}
	else if (G_IS_PARAM_SPEC_STRING(property_class->pspec))
	{
		string = g_value_dup_string (value);
	}
	else if (G_IS_PARAM_SPEC_CHAR(property_class->pspec))
		string = g_strdup_printf ("%c", g_value_get_char (value));
	else if (G_IS_PARAM_SPEC_UCHAR(property_class->pspec))
		string = g_strdup_printf ("%c", g_value_get_uchar (value));
	else if (G_IS_PARAM_SPEC_UNICHAR(property_class->pspec))
	{
                int len;
                string = g_malloc (7);
                len = g_unichar_to_utf8 (g_value_get_uint (value), string);
                string[len] = '\0';
	}
	else if (G_IS_PARAM_SPEC_BOOLEAN(property_class->pspec))
		string = g_strdup_printf ("%s", g_value_get_boolean (value) ?
					  GLADE_TAG_TRUE : GLADE_TAG_FALSE);
	else if (G_IS_PARAM_SPEC_OBJECT(property_class->pspec))
	{
		object = g_value_get_object (value);
		string = glade_property_class_make_string_from_object
			(property_class, object, fmt);
	}
	else if (GLADE_IS_PARAM_SPEC_OBJECTS (property_class->pspec))
	{
		objects = g_value_get_boxed (value);
		string = glade_property_class_make_string_from_objects
			(property_class, objects, fmt);
	}
	else
		g_critical ("Unsupported pspec type %s (value -> string)",
			    g_type_name (G_PARAM_SPEC_TYPE (property_class->pspec)));

	return string;
}

/* This is copied exactly from libglade. I've just renamed the function.
 */
static guint
glade_property_class_make_flags_from_string (GType type, const char *string)
{
    GFlagsClass *fclass;
    gchar *endptr, *prevptr;
    guint i, j, ret = 0;
    char *flagstr;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    fclass = g_type_class_ref(type);


    flagstr = g_strdup (string);
    for (ret = i = j = 0; ; i++) {
	gboolean eos;

	eos = flagstr [i] == '\0';
	
	if (eos || flagstr [i] == '|') {
	    GFlagsValue *fv;
	    const char  *flag;
	    gunichar ch;

	    flag = &flagstr [j];
            endptr = &flagstr [i];

	    if (!eos) {
		flagstr [i++] = '\0';
		j = i;
	    }

            /* trim spaces */
	    for (;;)
	      {
		ch = g_utf8_get_char (flag);
		if (!g_unichar_isspace (ch))
		  break;
		flag = g_utf8_next_char (flag);
	      }

	    while (endptr > flag)
	      {
		prevptr = g_utf8_prev_char (endptr);
		ch = g_utf8_get_char (prevptr);
		if (!g_unichar_isspace (ch))
		  break;
		endptr = prevptr;
	      }

	    if (endptr > flag)
	      {
		*endptr = '\0';
		fv = g_flags_get_value_by_name (fclass, flag);

		if (!fv)
		  fv = g_flags_get_value_by_nick (fclass, flag);

		if (fv)
		  ret |= fv->value;
		else
		  g_warning ("Unknown flag: '%s'", flag);
	      }

	    if (eos)
		break;
	}
    }
    
    g_free (flagstr);

    g_type_class_unref(fclass);

    return ret;
}

/* This is copied exactly from libglade. I've just renamed the function.
 */
static gint
glade_property_class_make_enum_from_string (GType type, const char *string)
{
    GEnumClass *eclass;
    GEnumValue *ev;
    gchar *endptr;
    gint ret = 0;

    ret = strtoul(string, &endptr, 0);
    if (endptr != string) /* parsed a number */
	return ret;

    eclass = g_type_class_ref(type);
    ev = g_enum_get_value_by_name(eclass, string);
    if (!ev) ev = g_enum_get_value_by_nick(eclass, string);
    if (ev)  ret = ev->value;

    g_type_class_unref(eclass);

    return ret;
}

static GObject *
glade_property_class_make_object_from_string (GladePropertyClass *property_class,
					      const gchar        *string,
					      GladeProject       *project,
					      GladeWidget        *widget)
{
	GObject *object = NULL;
	gchar   *fullpath;

	if (string == NULL) return NULL;
	
	if (property_class->pspec->value_type == GDK_TYPE_PIXBUF && project)
	{
		GdkPixbuf *pixbuf;
		
		if (*string == '\0') return NULL;
		
		fullpath = glade_project_resource_fullpath (project, string);

		if ((pixbuf = gdk_pixbuf_new_from_file (fullpath, NULL)) == NULL)
		{
			static GdkPixbuf *icon = NULL;

			if (icon == NULL)
			{
				GtkWidget *widget = gtk_label_new ("");
				icon = gtk_widget_render_icon (widget,
							       GTK_STOCK_MISSING_IMAGE,
							       GTK_ICON_SIZE_MENU, NULL);
				gtk_widget_destroy (widget);
			}
			
			pixbuf = gdk_pixbuf_copy (icon);
		}
 
		if (pixbuf)
		{
			object = G_OBJECT (pixbuf);
			g_object_set_data_full (object, "GladeFileName",
						g_strdup (string), g_free);
		}
 
		g_free (fullpath);
	}
	if (project && glade_project_get_format (project) == GLADE_PROJECT_FORMAT_LIBGLADE &&
	    property_class->pspec->value_type == GTK_TYPE_ADJUSTMENT)
	{
		gdouble value, lower, upper, step_increment, page_increment, page_size;
                gchar *pstring = (gchar*) string;

                value = g_ascii_strtod (pstring, &pstring);
                lower = g_ascii_strtod (pstring, &pstring);
                upper = g_ascii_strtod (pstring, &pstring);
                step_increment = g_ascii_strtod (pstring, &pstring);
                page_increment = g_ascii_strtod (pstring, &pstring);
                page_size = g_ascii_strtod (pstring, &pstring);

		object = G_OBJECT (gtk_adjustment_new (value, lower, upper, step_increment, page_increment, page_size));
	}
	else if (project)
	{
		GladeWidget *gwidget;
		if ((gwidget = glade_project_get_widget_by_name 
		     (project, widget, string)) != NULL)
			object = gwidget->object;
	}
	
	return object;
}

static GList *
glade_property_class_make_objects_from_string (GladePropertyClass *property_class,
					       const gchar        *string,
					       GladeProject       *project,
					       GladeWidget        *widget)
{
	GList    *objects = NULL;
	GObject  *object;
	gchar   **split;
	guint     i;
	
	if ((split = g_strsplit (string, GPC_OBJECT_DELIMITER, 0)) != NULL)
	{
		for (i = 0; split[i]; i++)
		{
			if ((object = glade_property_class_make_object_from_string
			     (property_class, split[i], project, widget)) != NULL)
				objects = g_list_prepend (objects, object);
		}
		g_strfreev (split);
	}
	return objects;
}

/**
 * glade_property_class_make_gvalue_from_string:
 * @property_class: A #GladePropertyClass
 * @string: a string representation of this property
 * @widget: the #GladeWidget that the associated property belongs to.
 *
 * Returns: A #GValue created based on the @property_class
 *          and @string criteria.
 */
GValue *
glade_property_class_make_gvalue_from_string (GladePropertyClass *property_class,
					      const gchar        *string,
					      GladeProject       *project,
					      GladeWidget        *widget)
{
	GValue    *value = g_new0 (GValue, 1);
	gchar    **strv;
	GdkColor   color = { 0, };

	g_value_init (value, property_class->pspec->value_type);
	
	if (G_IS_PARAM_SPEC_ENUM(property_class->pspec))
	{
		gint eval = glade_property_class_make_enum_from_string
			(property_class->pspec->value_type, string);
		g_value_set_enum (value, eval);
	}
	else if (G_IS_PARAM_SPEC_FLAGS(property_class->pspec))
	{
		guint flags = glade_property_class_make_flags_from_string
			(property_class->pspec->value_type, string);
		g_value_set_flags (value, flags);
	}
	else if (G_IS_PARAM_SPEC_VALUE_ARRAY (property_class->pspec))
	{
		GValueArray *value_array = g_value_array_new (0);
		GValue str_value = {0, };
		gint i;
			
		g_value_init (&str_value, G_TYPE_STRING);
		strv   = g_strsplit (string, "\n", 0);
						
		for (i = 0; strv[i]; i++)
		{
			g_value_set_static_string (&str_value, strv[i]);
			value_array = g_value_array_append (value_array, &str_value);
		}
		g_value_set_boxed (value, value_array);
		g_strfreev (strv);
	}
	else if (G_IS_PARAM_SPEC_BOXED(property_class->pspec))
	{
		if (property_class->pspec->value_type == GDK_TYPE_COLOR)
		{
			if (gdk_color_parse(string, &color) &&
			    gdk_colormap_alloc_color(gtk_widget_get_default_colormap(),
						     &color, FALSE, TRUE))
				g_value_set_boxed(value, &color);
			else
				g_warning ("could not parse colour name `%s'", string);
		}
		else if (property_class->pspec->value_type == G_TYPE_STRV)
		{
			strv   = g_strsplit (string, "\n", 0);
			g_value_take_boxed (value, strv);
		}
	}
	else if (G_IS_PARAM_SPEC_INT(property_class->pspec))
		g_value_set_int (value, g_ascii_strtoll (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_UINT(property_class->pspec))
		g_value_set_uint (value, g_ascii_strtoull (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_LONG(property_class->pspec))
		g_value_set_long (value, g_ascii_strtoll (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_ULONG(property_class->pspec))
		g_value_set_ulong (value, g_ascii_strtoull (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_INT64(property_class->pspec))
		g_value_set_int64 (value, g_ascii_strtoll (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_UINT64(property_class->pspec))
		g_value_set_uint64 (value, g_ascii_strtoull (string, NULL, 10));
	else if (G_IS_PARAM_SPEC_FLOAT(property_class->pspec))
		g_value_set_float (value, (float) g_ascii_strtod (string, NULL));
	else if (G_IS_PARAM_SPEC_DOUBLE(property_class->pspec))
		g_value_set_double (value, g_ascii_strtod (string, NULL));
	else if (G_IS_PARAM_SPEC_STRING(property_class->pspec))
		g_value_set_string (value, string);
	else if (G_IS_PARAM_SPEC_CHAR(property_class->pspec))
		g_value_set_char (value, string[0]);
	else if (G_IS_PARAM_SPEC_UCHAR(property_class->pspec))
		g_value_set_uchar (value, string[0]);
	else if (G_IS_PARAM_SPEC_UNICHAR(property_class->pspec))
		g_value_set_uint (value, g_utf8_get_char (string));
	else if (G_IS_PARAM_SPEC_BOOLEAN(property_class->pspec))
	{
		if (strcmp (string, GLADE_TAG_TRUE) == 0)
			g_value_set_boolean (value, TRUE);
		else
			g_value_set_boolean (value, FALSE);
	}
	else if (G_IS_PARAM_SPEC_OBJECT(property_class->pspec))
	{
		GObject *object = glade_property_class_make_object_from_string
			(property_class, string, project, widget);
		g_value_set_object (value, object);
	}
	else if (GLADE_IS_PARAM_SPEC_OBJECTS (property_class->pspec))
	{
		GList *objects = glade_property_class_make_objects_from_string
			(property_class, string, project, widget);
		g_value_set_boxed (value, objects);
	}
	else
		g_critical ("Unsupported pspec type %s (string -> value)",
			    g_type_name(G_PARAM_SPEC_TYPE (property_class->pspec)));

	return value;
}

/**
 * glade_property_class_make_gvalue_from_vl:
 * @property_class: A #GladePropertyClass
 * @vl: a #va_list holding one argument of the correct type
 *      specified by @property_class
 *
 * Returns: A #GValue created based on the @property_class
 *          and a @vl arg of the correct type.
 */
GValue *
glade_property_class_make_gvalue_from_vl (GladePropertyClass  *klass,
					  va_list              vl)
{
	GValue   *value;

	g_return_val_if_fail (klass != NULL, NULL);

	value = g_new0 (GValue, 1);
	g_value_init (value, klass->pspec->value_type);
	
	if (G_IS_PARAM_SPEC_ENUM(klass->pspec))
		g_value_set_enum (value, va_arg (vl, gint));
	else if (G_IS_PARAM_SPEC_FLAGS(klass->pspec))
		g_value_set_flags (value, va_arg (vl, gint));
	else if (G_IS_PARAM_SPEC_INT(klass->pspec))
		g_value_set_int (value, va_arg (vl, gint));
	else if (G_IS_PARAM_SPEC_UINT(klass->pspec))
		g_value_set_uint (value, va_arg (vl, guint));
	else if (G_IS_PARAM_SPEC_LONG(klass->pspec))
		g_value_set_long (value, va_arg (vl, glong));
	else if (G_IS_PARAM_SPEC_ULONG(klass->pspec))
		g_value_set_ulong (value, va_arg (vl, gulong));
	else if (G_IS_PARAM_SPEC_INT64(klass->pspec))
		g_value_set_int64 (value, va_arg (vl, gint64));
	else if (G_IS_PARAM_SPEC_UINT64(klass->pspec))
		g_value_set_uint64 (value, va_arg (vl, guint64));
	else if (G_IS_PARAM_SPEC_FLOAT(klass->pspec))
		g_value_set_float (value, (gfloat)va_arg (vl, gdouble));
	else if (G_IS_PARAM_SPEC_DOUBLE(klass->pspec))
		g_value_set_double (value, va_arg (vl, gdouble));
	else if (G_IS_PARAM_SPEC_STRING(klass->pspec))
		g_value_set_string (value, va_arg (vl, gchar *));
	else if (G_IS_PARAM_SPEC_CHAR(klass->pspec))
		g_value_set_char (value, (gchar)va_arg (vl, gint));
	else if (G_IS_PARAM_SPEC_UCHAR(klass->pspec))
		g_value_set_uchar (value, (guchar)va_arg (vl, guint));
	else if (G_IS_PARAM_SPEC_UNICHAR(klass->pspec))
		g_value_set_uint (value, va_arg (vl, gunichar));
	else if (G_IS_PARAM_SPEC_BOOLEAN(klass->pspec))
		g_value_set_boolean (value, va_arg (vl, gboolean));
	else if (G_IS_PARAM_SPEC_OBJECT(klass->pspec))
		g_value_set_object (value, va_arg (vl, gpointer));
	else if (G_VALUE_HOLDS_BOXED (value))
		g_value_set_boxed (value, va_arg (vl, gpointer));
	else
		g_critical ("Unsupported pspec type %s (vl -> string)",
			    g_type_name(G_PARAM_SPEC_TYPE (klass->pspec)));
	
	return value;
}

/**
 * glade_property_class_make_gvalue:
 * @klass: A #GladePropertyClass
 * @...: an argument of the correct type specified by @property_class
 *
 * Returns: A #GValue created based on the @property_class
 *          and the provided argument.
 */
GValue *
glade_property_class_make_gvalue (GladePropertyClass  *klass,
				  ...)
{
	GValue  *value;
	va_list  vl;

	g_return_val_if_fail (klass != NULL, NULL);

	va_start (vl, klass);
	value = glade_property_class_make_gvalue_from_vl (klass, vl);
	va_end (vl);

	return value;
}


/**
 * glade_property_class_set_vl_from_gvalue:
 * @klass: A #GladePropertyClass
 * @value: A #GValue to set
 * @vl: a #va_list holding one argument of the correct type
 *      specified by @klass
 * 
 *
 * Sets @vl from @value based on @klass criteria.
 */
void
glade_property_class_set_vl_from_gvalue (GladePropertyClass  *klass,
					 GValue              *value,
					 va_list              vl)
{
	g_return_if_fail (klass != NULL);
	g_return_if_fail (value != NULL);

	/* The argument is a pointer of the specified type, cast the pointer and assign
	 * the value using the proper g_value_get_ variation.
	 */
	if (G_IS_PARAM_SPEC_ENUM(klass->pspec))
		*(gint *)(va_arg (vl, gint *)) = g_value_get_enum (value);
	else if (G_IS_PARAM_SPEC_FLAGS(klass->pspec))
		*(gint *)(va_arg (vl, gint *)) = g_value_get_flags (value);
	else if (G_IS_PARAM_SPEC_INT(klass->pspec))
		*(gint *)(va_arg (vl, gint *)) = g_value_get_int (value);
	else if (G_IS_PARAM_SPEC_UINT(klass->pspec))
		*(guint *)(va_arg (vl, guint *)) = g_value_get_uint (value);
	else if (G_IS_PARAM_SPEC_LONG(klass->pspec))
		*(glong *)(va_arg (vl, glong *)) = g_value_get_long (value);
	else if (G_IS_PARAM_SPEC_ULONG(klass->pspec))
		*(gulong *)(va_arg (vl, gulong *)) = g_value_get_ulong (value);
	else if (G_IS_PARAM_SPEC_INT64(klass->pspec))
		*(gint64 *)(va_arg (vl, gint64 *)) = g_value_get_int64 (value);
	else if (G_IS_PARAM_SPEC_UINT64(klass->pspec))
		*(guint64 *)(va_arg (vl, guint64 *)) = g_value_get_uint64 (value);
	else if (G_IS_PARAM_SPEC_FLOAT(klass->pspec))
		*(gfloat *)(va_arg (vl, gdouble *)) = g_value_get_float (value);
	else if (G_IS_PARAM_SPEC_DOUBLE(klass->pspec))
		*(gdouble *)(va_arg (vl, gdouble *)) = g_value_get_double (value);
	else if (G_IS_PARAM_SPEC_STRING(klass->pspec))
		*(gchar **)(va_arg (vl, gchar *)) = (gchar *)g_value_get_string (value);
	else if (G_IS_PARAM_SPEC_CHAR(klass->pspec))
		*(gchar *)(va_arg (vl, gint *)) = g_value_get_char (value);
	else if (G_IS_PARAM_SPEC_UCHAR(klass->pspec))
		*(guchar *)(va_arg (vl, guint *)) = g_value_get_uchar (value);
	else if (G_IS_PARAM_SPEC_UNICHAR(klass->pspec))
		*(guint *)(va_arg (vl, gunichar *)) = g_value_get_uint (value);
	else if (G_IS_PARAM_SPEC_BOOLEAN(klass->pspec))
		*(gboolean *)(va_arg (vl, gboolean *)) = g_value_get_boolean (value);
	else if (G_IS_PARAM_SPEC_OBJECT(klass->pspec))
		*(gpointer *)(va_arg (vl, gpointer *)) = g_value_get_object (value);
	else if (G_VALUE_HOLDS_BOXED (value))
		*(gpointer *)(va_arg (vl, gpointer *)) = g_value_get_boxed (value);
	else
		g_critical ("Unsupported pspec type %s (string -> vl)",
			    g_type_name(G_PARAM_SPEC_TYPE (klass->pspec)));
}

/**
 * glade_property_class_get_from_gvalue:
 * @klass: A #GladePropertyClass
 * @value: A #GValue to set
 * @...: a return location of the correct type
 * 
 *
 * Assignes the provided return location to @value
 */
void
glade_property_class_get_from_gvalue (GladePropertyClass  *klass,
				      GValue              *value,
				      ...)
{
	va_list  vl;

	g_return_if_fail (klass != NULL);

	va_start (vl, value);
	glade_property_class_set_vl_from_gvalue (klass, value, vl);
	va_end (vl);
}


/* "need_handle": An evil trick to let us create pclasses without
 * adaptors and editors.
 */
GladePropertyClass *
glade_property_class_new_from_spec_full (gpointer     handle,
					 GParamSpec  *spec,
					 gboolean     need_handle)
{
	GObjectClass        *gtk_widget_class;
	GladePropertyClass  *property_class;
	GladeEditorProperty *eprop = NULL;

	g_return_val_if_fail (spec != NULL, NULL);
	gtk_widget_class = g_type_class_ref (GTK_TYPE_WIDGET);
	
	/* Only properties that are _new_from_spec() are 
	 * not virtual properties
	 */
	property_class          = glade_property_class_new (handle);
	property_class->virt    = FALSE;
	property_class->pspec   = spec;

	/* We only use the writable properties */
	if ((spec->flags & G_PARAM_WRITABLE) == 0)
		goto failed;

	property_class->id   = g_strdup (spec->name);
	property_class->name = g_strdup (g_param_spec_get_nick (spec));

	/* Register only editable properties.
	 */
	if (need_handle && !(eprop = glade_widget_adaptor_create_eprop
	      (GLADE_WIDGET_ADAPTOR (handle), property_class, FALSE)))
		goto failed;

	/* Just created it to see if it was supported.... destroy now... */
	if (eprop)
		gtk_widget_destroy (GTK_WIDGET (eprop));

	/* If its on the GtkWidgetClass, it goes in "common" 
	 * (unless stipulated otherwise in the xml file)
	 */
	if (g_object_class_find_property (gtk_widget_class, 
					  g_param_spec_get_name (spec)) != NULL)
		property_class->common = TRUE;

	/* Flag the construct only properties */
	if (spec->flags & G_PARAM_CONSTRUCT_ONLY)
		property_class->construct_only = TRUE;

	if (!property_class->id || !property_class->name)
	{
		g_critical ("No name or id for "
			    "glade_property_class_new_from_spec, failed.");
		goto failed;
	}

	property_class->tooltip  = g_strdup (g_param_spec_get_blurb (spec));
	property_class->orig_def = glade_property_class_get_default_from_spec (spec);
	property_class->def      = glade_property_class_get_default_from_spec (spec);

	g_type_class_unref (gtk_widget_class);
	return property_class;

  failed:
	glade_property_class_free (property_class);
	g_type_class_unref (gtk_widget_class);
	return NULL;
}

/**
 * glade_property_class_new_from_spec:
 * @handle: A generic pointer (i.e. a #GladeWidgetClass)
 * @spec: A #GParamSpec
 *
 * Returns: a newly created #GladePropertyClass based on @spec
 *          or %NULL if its unsupported.
 */
GladePropertyClass *
glade_property_class_new_from_spec (gpointer     handle,
				    GParamSpec  *spec)
{
	return glade_property_class_new_from_spec_full (handle, spec, TRUE);
}

/**
 * glade_property_class_is_visible:
 * @property_class: A #GladePropertyClass
 *
 *
 * Returns: whether or not to show this property in the editor
 */
gboolean
glade_property_class_is_visible (GladePropertyClass *klass)
{
	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (klass), FALSE);
	return klass->visible;
}

/**
 * glade_property_class_is_object:
 * @property_class: A #GladePropertyClass
 * @fmt: the #GladeProjectFormat
 *
 *
 * Returns: whether or not this is an object property 
 * that refers to another object in this project.
 */
gboolean
glade_property_class_is_object (GladePropertyClass  *klass,
				GladeProjectFormat   fmt)
{
	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (klass), FALSE);

 	return (GLADE_IS_PARAM_SPEC_OBJECTS (klass->pspec) ||
		(G_IS_PARAM_SPEC_OBJECT(klass->pspec) &&
		 klass->pspec->value_type != GDK_TYPE_PIXBUF &&
		 !(fmt == GLADE_PROJECT_FORMAT_LIBGLADE &&
		   klass->pspec->value_type == GTK_TYPE_ADJUSTMENT)));
}

/**
 * gpc_read_displayable_values_from_node:
 * @node: a GLADE_TAG_DISPLAYABLE_VALUES node
 * @values: an array of the values wich node overrides.
 * @n_values: the size of @values
 *
 * Reads and caches displayable values from the catalog
 */
static void
gpc_read_displayable_values_from_node (GladeXmlNode *node, 
				       GladePropertyClass *klass,				   
				       const gchar  *domain)
{
	gpointer       the_class = g_type_class_ref (klass->pspec->value_type);
	GladeXmlNode  *child;
	GEnumValue    *enum_values = NULL;
	GFlagsValue   *flags_values = NULL;
	gint           n_values, registered_values = 0;
	
	if (G_IS_PARAM_SPEC_ENUM (klass->pspec))
	{
		GEnumClass *eclass = the_class;
		enum_values = eclass->values;
		n_values = eclass->n_values;
	}
	else
	{
		GFlagsClass *fclass = the_class;
		flags_values = fclass->values;
		n_values = fclass->n_values;
	}
	
	if ((child = glade_xml_search_child (node, GLADE_TAG_VALUE)) == NULL)
		return;
	
	child = glade_xml_node_get_children (node);
	while (child != NULL)
	{
		gint         i;
		gchar       *id, *name;
		GEnumValue  *enum_val;
		GFlagsValue *flags_val;

		
		id = glade_xml_get_property_string_required (child, GLADE_TAG_ID, NULL);
		name = glade_xml_get_property_string_required (child, GLADE_TAG_NAME, NULL);

		if (!id || !name)
			continue;

		for (i = 0; i < n_values; i++)
		{
			/* is it a match ?? */
			if ((G_IS_PARAM_SPEC_ENUM (klass->pspec) &&
			     (strcmp (id, enum_values[i].value_name) == 0 ||
			      strcmp (id, enum_values[i].value_nick) == 0)) ||
			    (G_IS_PARAM_SPEC_FLAGS (klass->pspec) &&
			     (strcmp (id, flags_values[i].value_name) == 0 ||
			      strcmp (id, flags_values[i].value_nick) == 0)))
			{
				registered_values++;

				if (G_IS_PARAM_SPEC_ENUM (klass->pspec))
				{
					enum_val = &enum_values[i];
					glade_register_displayable_value (klass->pspec->value_type,
									  enum_val->value_nick, 
									  domain, name);
				}
				else
				{
					flags_val = &flags_values[i];
					glade_register_displayable_value (klass->pspec->value_type,
									  flags_val->value_nick, 
									  domain, name);

				}
				break;
			}
		}
		
		g_free(id);
		
		child = glade_xml_node_next (child);
	}
	
	if (n_values != registered_values)
		g_message ("%d missing displayable value for %s::%s", n_values - registered_values,
			   ((GladeWidgetAdaptor*)klass->handle)->name, klass->id);
	
	g_type_class_unref (the_class);
	
}

/**
 * glade_property_class_make_adjustment:
 * @property_class: a pointer to the property class
 *
 * Creates and appropriate GtkAdjustment for use in the editor
 *
 * Returns: An appropriate #GtkAdjustment for use in the Property editor
 */
GtkAdjustment *
glade_property_class_make_adjustment (GladePropertyClass *property_class)
{
	GtkAdjustment *adjustment;
	gdouble        min = 0, max = 0, def = 0;
	gboolean       float_range = FALSE;

	g_return_val_if_fail (property_class        != NULL, NULL);
	g_return_val_if_fail (property_class->pspec != NULL, NULL);

	if (G_IS_PARAM_SPEC_INT(property_class->pspec))
	{
		min = (gdouble)((GParamSpecInt *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecInt *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecInt *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_UINT(property_class->pspec))
	{
		min = (gdouble)((GParamSpecUInt *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecUInt *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecUInt *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_LONG(property_class->pspec))
	{
		min = (gdouble)((GParamSpecLong *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecLong *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecLong *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_ULONG(property_class->pspec))
	{
		min = (gdouble)((GParamSpecULong *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecULong *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecULong *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_INT64(property_class->pspec))
	{
		min = (gdouble)((GParamSpecInt64 *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecInt64 *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecInt64 *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_UINT64(property_class->pspec))
	{
		min = (gdouble)((GParamSpecUInt64 *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecUInt64 *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecUInt64 *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_FLOAT(property_class->pspec))
	{
		float_range = TRUE;
		min = ((GParamSpecFloat *) property_class->pspec)->minimum;
		max = ((GParamSpecFloat *) property_class->pspec)->maximum;
		def = ((GParamSpecFloat *) property_class->pspec)->default_value;
	} else if (G_IS_PARAM_SPEC_DOUBLE(property_class->pspec))
	{
		float_range = TRUE;
		min = (gdouble)((GParamSpecDouble *) property_class->pspec)->minimum;
		max = (gdouble)((GParamSpecDouble *) property_class->pspec)->maximum;
		def = (gdouble)((GParamSpecDouble *) property_class->pspec)->default_value;
	} else
	{
		g_critical ("Can't make adjustment for pspec type %s",
			    g_type_name(G_PARAM_SPEC_TYPE (property_class->pspec)));
	}

	adjustment = (GtkAdjustment *)gtk_adjustment_new (def, min, max,
							  float_range ?
							  FLOATING_STEP_INCREMENT :
							  NUMERICAL_STEP_INCREMENT,
							  float_range ? FLOATING_PAGE_INCREMENT :
							  NUMERICAL_PAGE_INCREMENT,
							  float_range ? FLOATING_PAGE_SIZE :
							  NUMERICAL_PAGE_SIZE);
	return adjustment;
}


static GParamSpec *
glade_property_class_parse_specifications (GladePropertyClass *klass, 
					   GladeXmlNode       *spec_node)
{
	gchar *string;
	GType  spec_type = 0, value_type = 0;
	GParamSpec *pspec = NULL;

	if ((string = glade_xml_get_value_string_required
	     (spec_node, GLADE_TAG_TYPE, "Need a type of GParamSpec to define")) != NULL)
	     	spec_type = glade_util_get_type_from_name (string, FALSE);

	g_free (string);
	
	g_return_val_if_fail (spec_type != 0, NULL);

	if (spec_type == G_TYPE_PARAM_ENUM   ||
	    spec_type == G_TYPE_PARAM_FLAGS  ||
	    spec_type == G_TYPE_PARAM_BOXED  ||
	    spec_type == G_TYPE_PARAM_OBJECT ||
	    spec_type == GLADE_TYPE_PARAM_OBJECTS)
	{
		if ((string = glade_xml_get_value_string_required
		     (spec_node, GLADE_TAG_VALUE_TYPE, 
		      "Need a value type to define enums flags boxed and object specs")) != NULL)
			value_type = glade_util_get_type_from_name (string, FALSE);
		
		g_free (string);
	
		g_return_val_if_fail (value_type != 0, NULL);

		if (spec_type == G_TYPE_PARAM_ENUM)
		{
			GEnumClass *eclass = g_type_class_ref (value_type);
			pspec = g_param_spec_enum ("dummy", "dummy", "dummy",
						   value_type, eclass->minimum, G_PARAM_READABLE|G_PARAM_WRITABLE);
			g_type_class_unref (eclass);
		}
		else if (spec_type == G_TYPE_PARAM_FLAGS)
			pspec = g_param_spec_flags ("dummy", "dummy", "dummy",
						    value_type, 0, G_PARAM_READABLE|G_PARAM_WRITABLE);
		else if (spec_type == G_TYPE_PARAM_OBJECT)
			pspec = g_param_spec_object ("dummy", "dummy", "dummy",
						   value_type, G_PARAM_READABLE|G_PARAM_WRITABLE);
		else if (spec_type == GLADE_TYPE_PARAM_OBJECTS)
			pspec = glade_param_spec_objects ("dummy", "dummy", "dummy",
							  value_type, G_PARAM_READABLE|G_PARAM_WRITABLE);
		else/*  if (spec_type == G_TYPE_PARAM_BOXED) */
			pspec = g_param_spec_boxed ("dummy", "dummy", "dummy",
						    value_type, G_PARAM_READABLE|G_PARAM_WRITABLE);
	}
	else if (spec_type == G_TYPE_PARAM_STRING)
		pspec = g_param_spec_string ("dummy", "dummy", "dummy",
					     NULL, G_PARAM_READABLE|G_PARAM_WRITABLE);
	else if (spec_type == G_TYPE_PARAM_BOOLEAN)
		pspec = g_param_spec_boolean ("dummy", "dummy", "dummy",
					      FALSE, G_PARAM_READABLE|G_PARAM_WRITABLE);
	else
	{
		gchar *minstr, *maxstr;

		minstr = glade_xml_get_value_string (spec_node, GLADE_TAG_MIN_VALUE);
		maxstr = glade_xml_get_value_string (spec_node, GLADE_TAG_MAX_VALUE);
		
		if (spec_type == G_TYPE_PARAM_CHAR)
		{
			gint8 min = minstr ? minstr[0]: G_MININT8;
			gint8 max = maxstr ? maxstr[0]: G_MAXINT8;

			pspec = g_param_spec_char ("dummy", "dummy", "dummy",
						   min, max, CLAMP (0, min, max), 
						   G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_UCHAR)
		{
			guint8 min = minstr ? minstr[0]: 0;
			guint8 max = maxstr ? maxstr[0]: G_MAXUINT8;

			pspec = g_param_spec_uchar ("dummy", "dummy", "dummy",
						   min, max, 0, 
						   G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_INT)
		{
			gint min = minstr ? g_ascii_strtoll (minstr, NULL, 10): G_MININT;
			gint max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXINT;

			pspec = g_param_spec_int ("dummy", "dummy", "dummy",
						  min, max, CLAMP (0, min, max), 
						  G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_UINT)
		{
			guint min = minstr ? g_ascii_strtoll (minstr, NULL, 10): 0;
			guint max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXUINT;

			pspec = g_param_spec_uint ("dummy", "dummy", "dummy",
						   min, max, CLAMP (0, min, max), 
						   G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_LONG)
		{
			glong min = minstr ? g_ascii_strtoll (minstr, NULL, 10): G_MINLONG;
			glong max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXLONG;

			pspec = g_param_spec_long ("dummy", "dummy", "dummy",
						   min, max, CLAMP (0, min, max), 
						   G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_ULONG)
		{
			gulong min = minstr ? g_ascii_strtoll (minstr, NULL, 10): 0;
			gulong max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXULONG;

			pspec = g_param_spec_ulong ("dummy", "dummy", "dummy",
						    min, max, CLAMP (0, min, max), 
						    G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_INT64)
		{
			gint64 min = minstr ? g_ascii_strtoll (minstr, NULL, 10): G_MININT64;
			gint64 max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXINT64;

			pspec = g_param_spec_int64 ("dummy", "dummy", "dummy",
						    min, max, CLAMP (0, min, max), 
						    G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_UINT64)
		{
			guint64 min = minstr ? g_ascii_strtoll (minstr, NULL, 10): 0;
			guint64 max = maxstr ? g_ascii_strtoll (maxstr, NULL, 10): G_MAXUINT64;

			pspec = g_param_spec_uint64 ("dummy", "dummy", "dummy",
						     min, max, CLAMP (0, min, max), 
						     G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_FLOAT)
		{
			gfloat min = minstr ? (float) g_ascii_strtod (minstr, NULL) : G_MINFLOAT;
			gfloat max = maxstr ? (float) g_ascii_strtod (maxstr, NULL) : G_MAXFLOAT;

			pspec = g_param_spec_float ("dummy", "dummy", "dummy",
						    min, max, CLAMP (0, min, max), 
						    G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else if (spec_type == G_TYPE_PARAM_DOUBLE)
		{
			gdouble min = minstr ? g_ascii_strtod (minstr, NULL) : G_MINFLOAT;
			gdouble max = maxstr ? g_ascii_strtod (maxstr, NULL) : G_MAXFLOAT;

			pspec = g_param_spec_float ("dummy", "dummy", "dummy",
						    min, max, CLAMP (0, min, max), 
						    G_PARAM_READABLE|G_PARAM_WRITABLE);
		}
		else
			g_critical ("Unsupported pspec type %s (value -> string)", g_type_name (spec_type));
	}
	return pspec;
}


/**
 * glade_property_class_update_from_node:
 * @node: the property node
 * @module: a #GModule to lookup symbols from the plugin
 * @object_type: the #GType of the owning object
 * @property_class: a pointer to the property class
 * @domain: the domain to translate catalog strings from
 *
 * Updates the @property_class with the contents of the node in the xml
 * file. Only the values found in the xml file are overridden.
 *
 * Returns: %TRUE on success. @property_class is set to NULL if the property
 *          has Disabled="TRUE".
 */
gboolean
glade_property_class_update_from_node (GladeXmlNode        *node,
				       GModule             *module,
				       GType                object_type,
				       GladePropertyClass **property_class,
				       const gchar         *domain)
{
	GladePropertyClass *klass;
	GParamSpec *pspec = NULL;
	gchar *buf, *translated;
	GladeXmlNode *child, *spec_node;

	g_return_val_if_fail (property_class != NULL, FALSE);

	/* for code cleanliness... */
	klass = *property_class;

	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (klass), FALSE);
	g_return_val_if_fail (glade_xml_node_verify (node, GLADE_TAG_PROPERTY), FALSE);

	/* check the id */
	buf = glade_xml_get_property_string_required (node, GLADE_TAG_ID, NULL);
	if (!buf)
		return FALSE;
	g_free (buf);
	
	if (glade_xml_get_property_boolean (node, GLADE_TAG_DISABLED, FALSE))
	{
		/* Its easier for us to keep disabled properties around and
		 * only virtually disable them */
		klass->ignore  = TRUE;
		klass->save    = FALSE;
		klass->visible = FALSE;
	}

	if ((spec_node = glade_xml_search_child (node, GLADE_TAG_SPECIFICATIONS)) != NULL)
		pspec = glade_property_class_parse_specifications (klass, spec_node);
	else if ((buf = glade_xml_get_value_string (node, GLADE_TAG_SPEC)) != NULL)
	{
		pspec = glade_utils_get_pspec_from_funcname (buf);
		g_free (buf);
	}

	/* ... get the tooltip from the pspec ... */
	if (pspec != NULL)
	{
		klass->pspec = pspec;

		/* Make sure we can tell properties apart by there 
		 * owning class.
		 */
		klass->pspec->owner_type = object_type;
		
		/* We overrode the pspec, now it *is* a virtual property. */
		klass->virt              = TRUE;
		
		if (strcmp (g_param_spec_get_blurb (klass->pspec), "dummy") != 0)
		{
			g_free (klass->tooltip);
			klass->tooltip = g_strdup (g_param_spec_get_blurb (klass->pspec));
		}

		if (klass->name == NULL || strcmp (g_param_spec_get_nick (klass->pspec), "dummy") != 0)
		{
			g_free (klass->name);
			klass->name = g_strdup (g_param_spec_get_nick (klass->pspec));
		}

		if (klass->pspec->flags & G_PARAM_CONSTRUCT_ONLY)
			klass->construct_only = TRUE;
		
		if (klass->orig_def) {
			g_value_unset (klass->orig_def);
			g_free (klass->orig_def);
		}
		klass->orig_def = glade_property_class_get_default_from_spec (klass->pspec);
		
		if (klass->def) {
			g_value_unset (klass->def);
			g_free (klass->def);
		}
		klass->def = glade_property_class_get_default_from_spec (klass->pspec);			
		
	}
	else if (!klass->pspec) 
	{
		/* If catalog file didn't specify a pspec function
		 * and this property isn't found by introspection
		 * we simply delete it from the list always.
		 */
		glade_property_class_free (klass);
		*property_class = NULL;
		return TRUE;
	}

	/* Get the default */
	if ((buf = glade_xml_get_property_string (node, GLADE_TAG_DEFAULT)) != NULL)
	{
		if (klass->def) {
			g_value_unset (klass->def);
			g_free (klass->def);
		}
		klass->def = glade_property_class_make_gvalue_from_string (klass, buf, NULL, NULL);

		if (klass->virt) {
			g_value_unset (klass->orig_def);
			g_free (klass->orig_def);
			klass->orig_def = glade_property_class_make_gvalue_from_string (klass, buf, NULL, NULL);
		}

		g_free (buf);
	}

	/* If needed, update the name... */
	if ((buf = glade_xml_get_property_string (node, GLADE_TAG_NAME)) != NULL)
	{
		g_free (klass->name);
		
		translated = dgettext (domain, buf);
		if (buf != translated)
		{
			/* translated is owned by gettext */
			klass->name = g_strdup (translated);
			g_free (buf);	
		}
		else
		{
			klass->name = buf;
		}
	}
	
	/* ...and the tooltip */
	if ((buf = glade_xml_get_value_string (node, GLADE_TAG_TOOLTIP)) != NULL)
	{
		g_free (klass->tooltip);
		
		translated = dgettext (domain, buf);
		if (buf != translated)
		{
			/* translated is owned by gettext */
			klass->tooltip = g_strdup (translated);
			g_free (buf);	
		}
		else
		{
			klass->tooltip = buf;
		}
	}

	/* Visible lines */
	glade_xml_get_value_int (node, GLADE_TAG_VISIBLE_LINES,  &klass->visible_lines);

	glade_xml_get_property_version
		(node, GLADE_TAG_VERSION_SINCE, 
		 &klass->version_since_major,
		 &klass->version_since_minor);

	glade_xml_get_property_version
		(node, GLADE_TAG_BUILDER_SINCE, 
		 &klass->builder_since_major,
		 &klass->builder_since_minor);

	/* Get the Parameters */
	if ((child = glade_xml_search_child (node, GLADE_TAG_PARAMETERS)) != NULL)
		klass->parameters = glade_parameter_list_new_from_node (klass->parameters, child);
		
	klass->construct_only = glade_xml_get_property_boolean (node, GLADE_TAG_CONSTRUCT_ONLY, klass->construct_only);
	klass->translatable   = glade_xml_get_property_boolean (node, GLADE_TAG_TRANSLATABLE,   klass->translatable);
	klass->common         = glade_xml_get_property_boolean (node, GLADE_TAG_COMMON,         klass->common);
	klass->optional       = glade_xml_get_property_boolean (node, GLADE_TAG_OPTIONAL,       klass->optional);
	klass->query          = glade_xml_get_property_boolean (node, GLADE_TAG_QUERY,          klass->query);
	klass->save           = glade_xml_get_property_boolean (node, GLADE_TAG_SAVE,           klass->save);
	klass->visible        = glade_xml_get_property_boolean (node, GLADE_TAG_VISIBLE,        klass->visible);
	klass->custom_layout  = glade_xml_get_property_boolean (node, GLADE_TAG_CUSTOM_LAYOUT,  klass->custom_layout);
	klass->ignore         = glade_xml_get_property_boolean (node, GLADE_TAG_IGNORE,         klass->ignore);
	klass->needs_sync     = glade_xml_get_property_boolean (node, GLADE_TAG_NEEDS_SYNC,     klass->needs_sync);
	klass->themed_icon    = glade_xml_get_property_boolean (node, GLADE_TAG_THEMED_ICON,    klass->themed_icon);
	klass->stock          = glade_xml_get_property_boolean (node, GLADE_TAG_STOCK,          klass->stock);
	klass->stock_icon     = glade_xml_get_property_boolean (node, GLADE_TAG_STOCK_ICON,     klass->stock_icon);
	klass->weight         = glade_xml_get_property_double  (node, GLADE_TAG_WEIGHT,         klass->weight);
	klass->transfer_on_paste = glade_xml_get_property_boolean (node, GLADE_TAG_TRANSFER_ON_PASTE, klass->transfer_on_paste);
	klass->save_always = glade_xml_get_property_boolean (node, GLADE_TAG_SAVE_ALWAYS, klass->save_always);
	klass->parentless_widget = glade_xml_get_property_boolean (node, GLADE_TAG_PARENTLESS_WIDGET, klass->parentless_widget);
	klass->libglade_only = glade_xml_get_property_boolean (node, GLADE_TAG_LIBGLADE_ONLY, klass->libglade_only);
	klass->libglade_unsupported = 
		glade_xml_get_property_boolean (node, GLADE_TAG_LIBGLADE_UNSUPPORTED, 
						klass->libglade_unsupported);


	if ((buf = glade_xml_get_property_string
	     (node, GLADE_TAG_CREATE_TYPE)) != NULL)
	{
		if (klass->create_type)
			g_free (klass->create_type);
		klass->create_type = buf;
	}

	/* If this property's value is an enumeration or flag then we try to get the displayable values */
	if ((G_IS_PARAM_SPEC_ENUM(klass->pspec) || G_IS_PARAM_SPEC_FLAGS(klass->pspec)) &&
	    (child = glade_xml_search_child (node, GLADE_TAG_DISPLAYABLE_VALUES)))
		gpc_read_displayable_values_from_node (child, klass, domain);
	
	/* Right now allowing the backend to specify that some properties
	 * go in the atk tab, ideally this shouldnt be needed.
	 */
	klass->atk = glade_xml_get_property_boolean (node, GLADE_TAG_ATK_PROPERTY, klass->atk);
	
	if (klass->optional)
		klass->optional_default =
			glade_xml_get_property_boolean (node, GLADE_TAG_OPTIONAL_DEFAULT, 
							klass->optional_default);

	/* notify that we changed the property class */
	klass->is_modified = TRUE;

	return TRUE;
}



/**
 * glade_property_class_match:
 * @klass: a #GladePropertyClass
 * @comp: a #GladePropertyClass
 *
 * Returns: whether @klass and @comp are a match or not
 *          (properties in seperate decendant heirarchies that
 *           have the same name are not matches).
 */
gboolean
glade_property_class_match (GladePropertyClass *klass,
			    GladePropertyClass *comp)
{
	g_return_val_if_fail (klass != NULL, FALSE);
	g_return_val_if_fail (comp != NULL, FALSE);

	return (strcmp (klass->id, comp->id) == 0 &&
		klass->packing           == comp->packing &&
		klass->pspec->owner_type == comp->pspec->owner_type);
}


/**
 * glade_property_class_void_value:
 * @klass: a #GladePropertyClass
 *
 * Returns: Whether @value for this @klass is voided; a voided value
 *          can be a %NULL value for boxed or object type param specs.
 */
gboolean
glade_property_class_void_value (GladePropertyClass *klass,
				 GValue             *value)
{
	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (klass), FALSE);
	
	if (G_IS_PARAM_SPEC_OBJECT (klass->pspec) &&
	    g_value_get_object (value) == NULL)
		return TRUE;
	else if (G_IS_PARAM_SPEC_BOXED (klass->pspec) &&
		 g_value_get_boxed (value) == NULL)
		return TRUE;

	return FALSE;
}

/**
 * glade_property_class_compare:
 * @klass: a #GladePropertyClass
 * @value1: a GValue of correct type for @klass
 * @value2: a GValue of correct type for @klass
 * @fmt: the #GladeProjectFormat to use
 *
 * Compares value1 with value2 according to @klass.
 *
 * Returns: -1, 0 or +1, if value1 is found to be less than,
 * equal to or greater than value2, respectively.
 */
gint
glade_property_class_compare (GladePropertyClass *klass,
			      const GValue       *value1,
			      const GValue       *value2,
			      GladeProjectFormat  fmt)
{
	gint retval;
	
	g_return_val_if_fail (GLADE_IS_PROPERTY_CLASS (klass), -1);
	
	/* GLib does not know how to compare a boxed real value */
	if (G_VALUE_HOLDS_BOXED (value1) || G_VALUE_HOLDS_BOXED (value2))
	{
		gchar *val1, *val2;

		/* So boxed types are compared by string and the backend is required to generate
		 * unique strings for values for this purpose.
		 *
		 * NOTE: We could add a pclass option to use the string compare vs. boxed compare...
		 */
		val1 = glade_widget_adaptor_string_from_value (klass->handle, klass, value1, fmt);
		val2 = glade_widget_adaptor_string_from_value (klass->handle, klass, value2, fmt);

		if (val1 && val2)
			retval = strcmp (val1, val2);
		else
			retval = val1 - val2;
		
		g_free (val1);
		g_free (val2);
	}
	else
	{
		if (G_IS_PARAM_SPEC_STRING (klass->pspec))
		{
			const gchar *value_str1, *value_str2;

			/* in string specs; NULL and '\0' are 
			 * treated as equivalent.
			 */
			value_str1 = g_value_get_string (value1);
			value_str2 = g_value_get_string (value2);

			if (value_str1 == NULL && value_str2 && value_str2[0] == '\0')
				return 0;
			else if (value_str2 == NULL && value_str1 && value_str1[0] == '\0')
				return 0;
		}
		retval = g_param_values_cmp (klass->pspec, value1, value2);
	}
	
	return retval;
}
