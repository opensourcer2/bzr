/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GLADE_SIGNAL_EDITOR_H__
#define __GLADE_SIGNAL_EDITOR_H__

#include "glade.h"

G_BEGIN_DECLS


#define GLADE_SIGNAL_EDITOR(e) ((GladeSignalEditor *)e)
#define GLADE_IS_SIGNAL_EDITOR(e) (e != NULL)

typedef struct _GladeSignalEditor  GladeSignalEditor;


/* The GladeSignalEditor is used to house the signal editor interface and
 * associated functionality.
 */
struct _GladeSignalEditor
{
	GtkWidget *main_window;  /* A vbox where all the widgets are added */

	GladeWidget *widget;
	GladeWidgetClass *class;

	gpointer  *editor;

	GtkWidget *signals_list;
	GtkTreeStore *model;
	GtkTreeView *tree_view;
};


LIBGLADEUI_API
GtkWidget *glade_signal_editor_get_widget (GladeSignalEditor *editor);

LIBGLADEUI_API
GladeSignalEditor *glade_signal_editor_new (gpointer *editor);

LIBGLADEUI_API
void glade_signal_editor_load_widget (GladeSignalEditor *editor, GladeWidget *widget);


G_END_DECLS

#endif /* __GLADE_SIGNAL_EDITOR_H__ */