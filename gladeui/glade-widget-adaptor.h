/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GLADE_WIDGET_ADAPTOR_H__
#define __GLADE_WIDGET_ADAPTOR_H__

#include <gladeui/glade-xml-utils.h>
#include <gladeui/glade-property-class.h>
#include <glib-object.h>
#include <gmodule.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GLADE_TYPE_WIDGET_ADAPTOR            (glade_widget_adaptor_get_type())
#define GLADE_WIDGET_ADAPTOR(obj)            \
        (G_TYPE_CHECK_INSTANCE_CAST ((obj), GLADE_TYPE_WIDGET_ADAPTOR, GladeWidgetAdaptor))
#define GLADE_WIDGET_ADAPTOR_CLASS(klass)    \
        (G_TYPE_CHECK_CLASS_CAST ((klass), GLADE_TYPE_WIDGET_ADAPTOR, GladeWidgetAdaptorClass))
#define GLADE_IS_WIDGET_ADAPTOR(obj)         \
        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GLADE_TYPE_WIDGET_ADAPTOR))
#define GLADE_IS_WIDGET_ADAPTOR_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE ((klass), GLADE_TYPE_WIDGET_ADAPTOR))
#define GLADE_WIDGET_ADAPTOR_GET_CLASS(o)    \
        (G_TYPE_INSTANCE_GET_CLASS ((o), GLADE_WIDGET_ADAPTOR, GladeWidgetAdaptorClass))

typedef struct _GladeWidgetAdaptor       GladeWidgetAdaptor;
typedef struct _GladeWidgetAdaptorClass  GladeWidgetAdaptorClass;
typedef struct _GladeWidgetAdaptorPriv   GladeWidgetAdaptorPriv;
typedef struct _GladeSignalClass         GladeSignalClass;

/**
 * GWA_IS_FIXED:
 * @obj: A #GladeWidgetAdaptor
 *
 * Checks whether this widget adaptor should be handled 
 * as a free-form container
 */
#define GWA_IS_FIXED(obj) \
        ((obj) ? GLADE_WIDGET_ADAPTOR_GET_CLASS(obj)->fixed : FALSE)

/**
 * GWA_IS_TOPLEVEL:
 * @obj: A #GladeWidgetAdaptor
 *
 * Checks whether this widget class has been marked as
 * a toplevel one.
 */
#define GWA_IS_TOPLEVEL(obj) \
        ((obj) ? GLADE_WIDGET_ADAPTOR_GET_CLASS(obj)->toplevel : FALSE)

/**
 * GWA_USE_PLACEHOLDERS:
 * @obj: A #GladeWidgetAdaptor
 *
 * Checks whether this widget class has been marked to
 * use placeholders in child widget operations
 */
#define GWA_USE_PLACEHOLDERS(obj) \
        ((obj) ? GLADE_WIDGET_ADAPTOR_GET_CLASS(obj)->use_placeholders : FALSE)


/**
 * GWA_DEFAULT_WIDTH:
 * @obj: A #GladeWidgetAdaptor
 *
 * Returns the default width to be used when this widget
 * is toplevel in the GladeDesignLayout
 */
#define GWA_DEFAULT_WIDTH(obj) \
        ((obj) ? GLADE_WIDGET_ADAPTOR_GET_CLASS(obj)->default_width : -1)


/**
 * GWA_DEFAULT_HEIGHT:
 * @obj: A #GladeWidgetAdaptor
 *
 * Returns the default width to be used when this widget
 * is toplevel in the GladeDesignLayout
 */
#define GWA_DEFAULT_HEIGHT(obj) \
        ((obj) ? GLADE_WIDGET_ADAPTOR_GET_CLASS(obj)->default_height : -1)

/**
 * GWA_GET_CLASS:
 * @type: A #GType
 *
 * Shorthand for referencing glade adaptor classes from
 * the plugin eg. GWA_GET_CLASS (GTK_TYPE_CONTAINER)->post_create (adaptor...
 */
#define GWA_GET_CLASS(type)                                                      \
    (((type) == G_TYPE_OBJECT) ?                                                 \
     (GladeWidgetAdaptorClass *)g_type_class_peek (GLADE_TYPE_WIDGET_ADAPTOR) :  \
     GLADE_WIDGET_ADAPTOR_GET_CLASS (glade_widget_adaptor_get_by_type(type)))


#define GLADE_VALID_CREATE_REASON(reason) (reason >= 0 && reason < GLADE_CREATE_REASONS)

/**
 * GladeCreateReason:
 * @GLADE_CREATE_USER: Was created at the user's request
 *                     (this is a good time to set any properties
 *                     or add children to the project; like GtkFrame's 
 *                     label for example).
 * @GLADE_CREATE_COPY: Was created as a result of the copy/paste 
 *                     mechanism, at this point you can count on glade
 *                     to follow up with properties and children on 
 *                     its own.
 * @GLADE_CREATE_LOAD: Was created during the load process.
 * @GLADE_CREATE_REBUILD: Was created as a replacement for another project 
 *                        object; this only happens when the user is 
 *                        changing a property that is marked by the type 
 *                        system as G_PARAM_SPEC_CONSTRUCT_ONLY.
 * @GLADE_CREATE_REASONS: Never used.
 *
 * These are the reasons your #GladePostCreateFunc can be called.
 */
typedef enum _GladeCreateReason
{
	GLADE_CREATE_USER = 0,
	GLADE_CREATE_COPY,
	GLADE_CREATE_LOAD,
	GLADE_CREATE_REBUILD,
	GLADE_CREATE_REASONS
} GladeCreateReason;

#define GLADE_CREATE_REASON (glade_create_reason_get_type())

/**
 * GladeSetPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @object: The #GObject
 * @property_name: The property identifier
 * @value: The #GValue
 *
 * This delagate function is used to apply the property value on
 * the runtime object.
 *
 * Sets @value on @object for a given #GladePropertyClass
 */
typedef void     (* GladeSetPropertyFunc)    (GladeWidgetAdaptor *adaptor,
					      GObject            *object,
					      const gchar        *property_name,
					      const GValue       *value);

/**
 * GladeGetPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @object: The #GObject
 * @property_name: The property identifier
 * @value: The #GValue
 *
 * Gets @value on @object for a given #GladePropertyClass
 */
typedef void     (* GladeGetPropertyFunc)    (GladeWidgetAdaptor *adaptor,
					      GObject            *object,
					      const gchar        *property_name,
					      GValue             *value);

/**
 * GladeVerifyPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @object: The #GObject
 * @property_name: The property identifier
 * @value: The #GValue
 *
 * This delagate function is always called whenever setting any
 * properties with the exception of load time, and copy/paste time
 * (basicly the two places where we recreate a hierarchy that we
 * already know "works") its basicly an optional backend provided
 * boundry checker for properties.
 *
 * Returns: whether or not its OK to set @value on @object
 */
typedef gboolean (* GladeVerifyPropertyFunc)      (GladeWidgetAdaptor *adaptor,
						   GObject            *object,
						   const gchar        *property_name,
						   const GValue       *value);

/**
 * GladeChildSetPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @container: The #GObject container
 * @child: The #GObject child
 * @property_name: The property name
 * @value: The #GValue
 *
 * Called to set the packing property @property_name to @value
 * on the @child object of @container.
 */
typedef void   (* GladeChildSetPropertyFunc)      (GladeWidgetAdaptor *adaptor,
						   GObject            *container,
						   GObject            *child,
						   const gchar        *property_name,
						   const GValue       *value);

/**
 * GladeChildGetPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @container: The #GObject container
 * @child: The #GObject child
 * @property_name: The property name
 * @value: The #GValue
 *
 * Called to get the packing property @property_name
 * on the @child object of @container into @value.
 */
typedef void   (* GladeChildGetPropertyFunc)      (GladeWidgetAdaptor *adaptor,
						   GObject            *container,
						   GObject            *child,
						   const gchar        *property_name,
						   GValue             *value);

/**
 * GladeChildVerifyPropertyFunc:
 * @adaptor: A #GladeWidgetAdaptor
 * @container: The #GObject container
 * @child: The #GObject child
 * @property_name: The property name
 * @value: The #GValue
 *
 * This delagate function is always called whenever setting any
 * properties with the exception of load time, and copy/paste time
 * (basicly the two places where we recreate a hierarchy that we
 * already know "works") its basicly an optional backend provided
 * boundry checker for properties.
 *
 * Returns: whether or not its OK to set @value on @object
 */
typedef gboolean (* GladeChildVerifyPropertyFunc) (GladeWidgetAdaptor *adaptor,
						   GObject            *container,
						   GObject            *child,
						   const gchar        *property_name,
						   GValue             *value);


/**
 * GladeGetChildrenFunc:
 * @container: A #GObject container
 * @Returns: A #GList of #GObject children.
 *
 * A function called to get @containers children.
 */
typedef GList   *(* GladeGetChildrenFunc)         (GladeWidgetAdaptor *adaptor,
						   GObject            *container);

/**
 * GladeAddChildFunc:
 * @parent: A #GObject container
 * @child: A #GObject child
 *
 * Called to add @child to @parent.
 */
typedef void     (* GladeAddChildFunc)            (GladeWidgetAdaptor *adaptor,
						   GObject            *parent,
						   GObject            *child);
/**
 * GladeRemoveChildFunc:
 * @parent: A #GObject container
 * @child: A #GObject child
 *
 * Called to remove @child from @parent.
 */
typedef void     (* GladeRemoveChildFunc)         (GladeWidgetAdaptor *adaptor,
						   GObject            *parent,
						   GObject            *child);

/**
 * GladeReplaceChildFunc:
 * @container: A #GObject container
 * @old: The old #GObject child
 * @new: The new #GObject child to take its place
 * 
 * Called to swap placeholders with project objects
 * in containers.
 */
typedef void     (* GladeReplaceChildFunc)        (GladeWidgetAdaptor *adaptor,
						   GObject            *container,  
						   GObject            *old_obj,
						   GObject            *new_obj);

/**
 * GladePostCreateFunc:
 * @object: a #GObject
 * @reason: a #GladeCreateReason
 *
 * This function is called exactly once for any project object
 * instance and can be for any #GladeCreateReason.
 */
typedef void     (* GladePostCreateFunc)          (GladeWidgetAdaptor *adaptor,
						   GObject            *object,
						   GladeCreateReason   reason);

/**
 * GladeGetInternalFunc:
 * @parent: A #GObject composite object
 * @name: A string identifier
 * @child: A return location for a #GObject
 *
 * Called to lookup @child in composite object @parent by @name.
 */
typedef GObject *(* GladeGetInternalFunc)         (GladeWidgetAdaptor *adaptor,
						   GObject            *parent,
						   const gchar        *name);


/**
 * GladeEditorLaunchFunc:
 * @object: A #GObject
 *
 * Called to launch a custom editor for @object
 */
typedef void     (* GladeEditorLaunchFunc)        (GladeWidgetAdaptor *adaptor,
						   GObject            *object);


/* GladeSignalClass contains all the info we need for a given signal, such as
 * the signal name, and maybe more in the future 
 */
struct _GladeSignalClass
{
	GSignalQuery query;

	const gchar *name;         /* Name of the signal, eg clicked */
	gchar       *type;         /* Name of the object class that this signal belongs to
				    * eg GtkButton */

};

typedef struct _GWAAction GWAAction;
struct _GWAAction
{
	gchar *id, *label, *stock;
	gboolean is_a_group;
	GList *actions;
};

/* Note that everything that must be processed at the creation of
 * every instance is managed on the instance structure, and everywhere
 * that we want to take advantage of inheritance is handled in the class
 * structure.
 */
struct _GladeWidgetAdaptor
{
	GObject      parent_instance;

	GType        type;         /* GType of the widget */

	gchar       *name;         /* Name of the widget, for example GtkButton */


	gchar       *generic_name; /* Used to generate names of new widgets, for
				    * example "button" so that we generate button1,
				    * button2, buttonX ..
				    */

	gchar       *title;        /* Translated class name used in the UI */

	GList       *properties;   /* List of GladePropertyClass objects.
				    * [see glade-property.h] this list contains
				    * properties about the widget that we are going
				    * to modify. Like "title", "label", "rows" .
				    * Each property creates an input in the propety
				    * editor.
				    */
	GList       *packing_props; /* List of GladePropertyClass objects that describe
				     * properties for child objects packing definition -
				     * note there may be more than one type of child supported
				     * by a widget and thus they may have different sets
				     * of properties for each type - this association is
				     * managed on the GladePropertyClass proper.
				     */
  
	GList       *signals;        /* List of GladeSignalClass objects */

        GList       *child_packings; /* Default packing property values */

	GList       *actions;        /* A list of GWAAction */

	GladeWidgetAdaptorPriv *priv;

};

struct _GladeWidgetAdaptorClass
{
	GObjectClass               parent_class;

	gboolean                   fixed;      /* If this is a GtkContainer, use free-form
						* placement with drag/resize/paste at mouse...
						*/
	gboolean                   toplevel;   /* If this class is toplevel */

	gboolean                   use_placeholders; /* Whether or not to use placeholders
						      * to interface with child widgets.
						      */

	gint                       default_width;  /* Default width in GladeDesignLayout */
	gint                       default_height; /* Default height in GladeDesignLayout */

	GladePostCreateFunc        post_create;   /* Executed after widget creation: 
						   * plugins use this to setup various
						   * support codes.
						   */

	GladeGetInternalFunc       get_internal_child; /* Retrieves the the internal child
							* of the given name.
							*/
	
	GladeEditorLaunchFunc      launch_editor; /* Entry point for custom editors. */


	/* Delagate to verify if this is a valid value for this property,
	 * if this function exists and returns FALSE, then glade_property_set
	 * will abort before making any changes
	 */
	GladeVerifyPropertyFunc verify_property;
	
	/* An optional backend function used instead of g_object_set()
	 * virtual properties must be handled with this function.
	 */
	GladeSetPropertyFunc set_property;

	/* An optional backend function used instead of g_object_get()
	 * virtual properties must be handled with this function.
	 *
	 * Note that since glade knows what the property values are 
	 * at all times regardless of the objects copy, this is currently
	 * only used to obtain the values of packing properties that are
	 * set by the said object's parent at "container_add" time.
	 */
	GladeGetPropertyFunc get_property;


	GladeAddChildFunc          add;              /* Adds a new child of this type */
	GladeRemoveChildFunc       remove;           /* Removes a child from the container */
	GladeGetChildrenFunc       get_children;     /* Returns a list of direct children for
						      * this support type.
						      */


	
	GladeChildVerifyPropertyFunc child_verify_property; /* A boundry checker for 
							     * packing properties 
							     */
	GladeChildSetPropertyFunc    child_set_property; /* Sets/Gets a packing property */
	GladeChildGetPropertyFunc    child_get_property; /* for this child */
	
	GladeReplaceChildFunc      replace_child;  /* This method replaces a 
						    * child widget with
						    * another one: it's used to
						    * replace a placeholder with
						    * a widget and viceversa.
						    */
	/* Signals */
	gboolean                   (*action_activated) (GladeWidgetAdaptor *, GladeWidget *, const gchar *);
};

#define glade_widget_adaptor_create_widget(adaptor, query, ...) \
    (glade_widget_adaptor_create_widget_real (query, "adaptor", adaptor, __VA_ARGS__));

#define glade_widget_adaptor_from_pclass(pclass) \
    ((pclass) ? (GladeWidgetAdaptor *)((GladePropertyClass *)(pclass))->handle : NULL)

#define glade_widget_adaptor_from_pspec(pspec) \
    ((pspec) ? glade_widget_adaptor_get_by_type (((GParamSpec *)(pspec))->owner_type) : NULL)

LIBGLADEUI_API
GType                glade_widget_adaptor_get_type         (void) G_GNUC_CONST;
 
LIBGLADEUI_API
GType glade_create_reason_get_type          (void) G_GNUC_CONST;

LIBGLADEUI_API
GladeWidgetAdaptor  *glade_widget_adaptor_from_catalog     (GladeXmlNode         *class_node,
							    const gchar          *catname,
							    GModule              *module,
							    const gchar          *domain,
							    const gchar          *book);
LIBGLADEUI_API
void                 glade_widget_adaptor_register         (GladeWidgetAdaptor   *adaptor);
LIBGLADEUI_API 
GladeWidget         *glade_widget_adaptor_create_internal  (GladeWidget          *parent,
							    GObject              *internal_object,
							    const gchar          *internal_name,
							    const gchar          *parent_name,
							    gboolean              anarchist,
							    GladeCreateReason     reason);
LIBGLADEUI_API
GladeWidget         *glade_widget_adaptor_create_widget_real (gboolean            query, 
							      const gchar        *first_property,
							      ...);

LIBGLADEUI_API
GladeWidgetAdaptor  *glade_widget_adaptor_get_by_name        (const gchar        *name);
LIBGLADEUI_API
GladeWidgetAdaptor  *glade_widget_adaptor_get_by_type        (GType               type);
LIBGLADEUI_API
GList               *glade_widget_adaptor_get_derived_adaptors (GType             type);
LIBGLADEUI_API
GladePropertyClass  *glade_widget_adaptor_get_property_class (GladeWidgetAdaptor *adaptor,
							      const gchar        *name);
LIBGLADEUI_API
GladePropertyClass  *glade_widget_adaptor_get_pack_property_class (GladeWidgetAdaptor *adaptor,
								   const gchar        *name);
LIBGLADEUI_API
GParameter          *glade_widget_adaptor_default_params     (GladeWidgetAdaptor *adaptor,
							      gboolean            construct,
							      guint              *n_params);
LIBGLADEUI_API
void                 glade_widget_adaptor_post_create        (GladeWidgetAdaptor *adaptor,
							      GObject            *object,
							      GladeCreateReason   reason);
LIBGLADEUI_API
GObject             *glade_widget_adaptor_get_internal_child (GladeWidgetAdaptor *adaptor,
							      GObject            *object,
							      const gchar        *internal_name);
LIBGLADEUI_API
void                 glade_widget_adaptor_launch_editor      (GladeWidgetAdaptor *adaptor,
							      GObject            *object);
LIBGLADEUI_API
void                 glade_widget_adaptor_set_property       (GladeWidgetAdaptor *adaptor,
							      GObject            *object,
							      const gchar        *property_name,
							      const GValue       *value);
LIBGLADEUI_API
void                 glade_widget_adaptor_get_property       (GladeWidgetAdaptor *adaptor,
							      GObject            *object,
							      const gchar        *property_name,
							      GValue             *value);
LIBGLADEUI_API
gboolean             glade_widget_adaptor_verify_property    (GladeWidgetAdaptor *adaptor,
							      GObject            *object,
							      const gchar        *property_name,
							      const GValue       *value);
LIBGLADEUI_API
void                 glade_widget_adaptor_add                (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *child);
LIBGLADEUI_API
void                 glade_widget_adaptor_remove             (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *child);
LIBGLADEUI_API
GList               *glade_widget_adaptor_get_children       (GladeWidgetAdaptor *adaptor,
							      GObject            *container);
LIBGLADEUI_API
gboolean             glade_widget_adaptor_has_child          (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *child);
LIBGLADEUI_API
void                 glade_widget_adaptor_child_set_property (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *child,
							      const gchar        *property_name,
							      const GValue       *value);
LIBGLADEUI_API
void                 glade_widget_adaptor_child_get_property (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *child,
							      const gchar        *property_name,
							      GValue             *value);
LIBGLADEUI_API
gboolean             glade_widget_adaptor_child_verify_property (GladeWidgetAdaptor *adaptor,
								 GObject            *container,
								 GObject            *child,
								 const gchar        *property_name,
								 GValue             *value);
LIBGLADEUI_API
void                 glade_widget_adaptor_replace_child      (GladeWidgetAdaptor *adaptor,
							      GObject            *container,
							      GObject            *old_obj,
							      GObject            *new_obj);
LIBGLADEUI_API
gboolean             glade_widget_adaptor_contains_extra     (GladeWidgetAdaptor *adaptor);
LIBGLADEUI_API
gboolean             glade_widget_adaptor_query              (GladeWidgetAdaptor *adaptor);

LIBGLADEUI_API G_CONST_RETURN
gchar               *glade_widget_adaptor_get_packing_default(GladeWidgetAdaptor *child_adaptor,
							      GladeWidgetAdaptor *parent_adaptor,
							      const gchar        *propert_id);
LIBGLADEUI_API
void                 glade_widget_adaptor_action_activate    (GladeWidget *widget,
					    		      const gchar *action_id);

LIBGLADEUI_API
gboolean             glade_widget_adaptor_is_container       (GladeWidgetAdaptor *adaptor);

G_END_DECLS

#endif /* __GLADE_WIDGET_ADAPTOR_H__ */