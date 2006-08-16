/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* This functions are based on gnome-print/libgpa/gpa-xml.c which were in turn 
 * based on gnumeric/xml-io.c
 */
/* Authors:
 *   Daniel Veillard <Daniel.Veillard@w3.org>
 *   Miguel de Icaza <miguel@gnu.org>
 *   Chema Celorio <chema@gnome.org>
 */

#include <string.h>
#include <glib.h>
#include <string.h>

#include "glade-xml-utils.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>

struct _GladeXmlNode
{
	xmlNodePtr node;
};

struct _GladeXmlDoc
{
	xmlDoc doc;
};

struct _GladeXmlContext {
	GladeXmlDoc *doc;
	gboolean freedoc;
	xmlNsPtr  ns;
};


/* This is used inside for loops so that we skip xml comments 
 * <!-- i am a comment ->
 * also to skip whitespace between nodes
 */
#define skip_text(node) if ((xmlStrcmp ( ((xmlNodePtr)node)->name, BAD_CAST("text")) == 0) ||\
			    (xmlStrcmp ( ((xmlNodePtr)node)->name, BAD_CAST("comment")) == 0)) { \
			         node = (GladeXmlNode *)((xmlNodePtr)node)->next; continue ; };
#define skip_text_libxml(node) if ((xmlStrcmp ( ((xmlNodePtr)node)->name, BAD_CAST("text")) == 0) ||\
			           (xmlStrcmp ( ((xmlNodePtr)node)->name, BAD_CAST("comment")) == 0)) { \
                                       node = ((xmlNodePtr)node)->next; continue ; };


static gchar *
claim_string (xmlChar *string)
{
	gchar *ret;
	ret = g_strdup (CAST_BAD(string));
	xmlFree (string);
	return ret;
}

/**
 * glade_xml_set_value:
 * @node_in: a #GladeXmlNode
 * @name: a string
 * @val: a string
 *
 * Sets the property @name in @node_in to @val
 */
void
glade_xml_set_value (GladeXmlNode *node_in, const gchar *name, const gchar *val)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	xmlChar *ret;

	ret = xmlGetProp (node, BAD_CAST(name));
	if (ret)
	{
		xmlFree (ret);
		xmlSetProp (node, BAD_CAST(name), BAD_CAST(val));
		return;
	}
}

/**
 * glade_xml_get_content:
 * @node_in: a #GladeXmlNode
 *
 * Returns a string containing the content of @node_in.
 * Note: It is the caller's responsibility to free the memory used by this 
 * string.
 */
gchar *
glade_xml_get_content (GladeXmlNode *node_in)
{
	xmlNodePtr  node = (xmlNodePtr) node_in;
	xmlChar    *val  = xmlNodeGetContent(node);

	return claim_string (val);
}

/**
 * glade_xml_set_content:
 * @node_in: a #GladeXmlNode
 * @content: a string
 *
 * Sets the content of @node to @content.
 */
void
glade_xml_set_content (GladeXmlNode *node_in, const gchar *content)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	xmlNodeSetContent(node, BAD_CAST(content));
}

/*
 * Get a value for a node either carried as an attibute or as
 * the content of a child.
 *
 * Returns a g_malloc'ed string.  Caller must free.
 * (taken from gnumeric )
 *
 */
static gchar *
glade_xml_get_value (xmlNodePtr node, const gchar *name)
{
	xmlNodePtr  child;
	gchar      *ret = NULL;
	
	for (child = node->children; child; child = child->next)
		if (!xmlStrcmp (child->name, BAD_CAST(name)))
			ret = claim_string (xmlNodeGetContent(child));

	return ret;
}

/**
 * glade_xml_node_verify_silent:
 * @node_in: a #GladeXmlNode
 * @name: a string
 *
 * Returns: %TRUE if @node_in's name is equal to @name, %FALSE otherwise
 */
gboolean
glade_xml_node_verify_silent (GladeXmlNode *node_in, const gchar *name)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	g_return_val_if_fail (node != NULL, FALSE);
	
	if (xmlStrcmp (node->name, BAD_CAST(name)) != 0)
		return FALSE;
	return TRUE;
}

/**
 * glade_xml_node_verify:
 * @node_in: a #GladeXmlNode
 * @name: a string
 *
 * This is a wrapper around glade_xml_node_verify_silent(), only it emits
 * a g_warning() if @node_in has a name different than @name.
 *
 * Returns: %TRUE if @node_in's name is equal to @name, %FALSE otherwise
 */
gboolean
glade_xml_node_verify (GladeXmlNode *node_in, const gchar *name)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	if (!glade_xml_node_verify_silent (node_in, name))
	{
		g_warning  ("Expected node was \"%s\", encountered \"%s\"",
			    name, node->name);
		return FALSE;
	}

	return TRUE;
}

/**
 * glade_xml_get_value_int:
 * @node_in: a #GladeXmlNode
 * @name: a string
 * @val: a pointer to an #int
 *
 * Gets an integer value for a node either carried as an attribute or as
 * the content of a child.
 *
 * Returns: %TRUE if the node is found, %FALSE otherwise
 */
gboolean
glade_xml_get_value_int (GladeXmlNode *node_in, const gchar *name, gint *val)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	gchar *ret;
	gint i;
	gint res;

	ret = glade_xml_get_value (node, name);
	if (ret == NULL) return 0;
	res = sscanf (ret, "%d", &i);
	g_free (ret);

	if (res == 1)
	{
	        *val = i;
		return TRUE;
	}

	return FALSE;
}

/**
 * glade_xml_get_value_int_required:
 * @node: a #GladeXmlNode
 * @name: a string
 * @val: a pointer to an #int
 * 
 * This is a wrapper around glade_xml_get_value_int(), only it emits
 * a g_warning() if @node_in did not contain the requested tag
 * 
 * Returns:
 **/
gboolean
glade_xml_get_value_int_required (GladeXmlNode *node, const gchar *name, gint  *val)
{
	gboolean ret;
	
	ret = glade_xml_get_value_int (node, name, val);

	if (ret == FALSE)
		g_warning ("The file did not contained the required value \"%s\"\n"
			   "Under the \"%s\" tag.", name, glade_xml_node_get_name (node));
			
	return ret;
}

/*
 * Get a String value for a node either carried as an attibute or as
 * the content of a child.
 */
gchar *
glade_xml_get_value_string (GladeXmlNode *node_in, const gchar *name)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	return glade_xml_get_value (node, name);
}

static gchar *
glade_xml_get_property (xmlNodePtr node, const gchar *name)
{
	xmlChar *val;

	val = xmlGetProp (node, BAD_CAST(name));

	if (val)
		return claim_string (val);

	return NULL;
}

static void
glade_xml_set_property (xmlNodePtr node, const gchar *name, const gchar *value)
{
	if (value)
		xmlSetProp (node, BAD_CAST(name), BAD_CAST(value));
}

#define GLADE_TAG_TRUE   "True"
#define GLADE_TAG_FALSE  "False"
#define GLADE_TAG_TRUE2  "TRUE"
#define GLADE_TAG_FALSE2 "FALSE"
#define GLADE_TAG_TRUE3  "yes"
#define GLADE_TAG_FALSE3 "no"
/*
 * Get a String value for a node either carried as an attibute or as
 * the content of a child.
 */
gboolean
glade_xml_get_boolean (GladeXmlNode *node_in, const gchar *name, gboolean _default)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	gchar * value;
	gboolean ret = FALSE;

	value = glade_xml_get_value (node, name);
	if (value == NULL)
		return _default;
	
	if (strcmp (value, GLADE_TAG_FALSE) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_FALSE2) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_FALSE3) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_TRUE) == 0)
		ret = TRUE;
	else if (strcmp (value, GLADE_TAG_TRUE2) == 0)
		ret = TRUE;
	else if (strcmp (value, GLADE_TAG_TRUE3) == 0)
		ret = TRUE;
	else	
		g_warning ("Boolean tag unrecognized *%s*\n", value);

	g_free (value);

	return ret;
}

/*
 * Get a String value for a node either carried as an attibute or as
 * the content of a child.
 */
gboolean
glade_xml_get_property_boolean (GladeXmlNode *node_in,
				const gchar *name,
				gboolean _default)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	gchar * value;
	gboolean ret = FALSE;

	value = glade_xml_get_property (node, name);
	if (value == NULL)
		return _default;

	if (strcmp (value, GLADE_TAG_FALSE) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_FALSE2) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_FALSE3) == 0)
		ret = FALSE;
	else if (strcmp (value, GLADE_TAG_TRUE) == 0)
		ret = TRUE;
	else if (strcmp (value, GLADE_TAG_TRUE2) == 0)
		ret = TRUE;
	else if (strcmp (value, GLADE_TAG_TRUE3) == 0)
		ret = TRUE;
	else	
		g_warning ("Boolean tag unrecognized *%s*\n", value);

	g_free (value);

	return ret;
}

void
glade_xml_node_set_property_boolean (GladeXmlNode *node_in,
				     const gchar *name,
				     gboolean value)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	if (value)
		glade_xml_set_property (node, name, GLADE_TAG_TRUE);
	else
		glade_xml_set_property (node, name, GLADE_TAG_FALSE);
}

#undef GLADE_TAG_TRUE
#undef GLADE_TAG_FALSE
#undef GLADE_TAG_TRUE2
#undef GLADE_TAG_FALSE2
#undef GLADE_TAG_TRUE3
#undef GLADE_TAG_FALSE3

gchar *
glade_xml_get_value_string_required (GladeXmlNode *node_in,
				     const gchar *name,
				     const gchar *xtra)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	gchar *value = glade_xml_get_value (node, name);

	if (value == NULL)
	{
		if (xtra == NULL)
			g_warning ("The file did not contained the required value \"%s\"\n"
				   "Under the \"%s\" tag.", name, node->name);
		else
			g_warning ("The file did not contained the required value \"%s\"\n"
				   "Under the \"%s\" tag (%s).", name, node->name, xtra);
	}

	return value;
}

gchar *
glade_xml_get_property_string (GladeXmlNode *node_in, const gchar *name)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	return glade_xml_get_property (node, name);
}

void
glade_xml_node_set_property_string (GladeXmlNode *node_in,
				    const gchar *name,
				    const gchar *string)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	glade_xml_set_property (node, name, string);
}

gchar *
glade_xml_get_property_string_required (GladeXmlNode *node_in,
					const gchar *name,
					const gchar *xtra)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	gchar *value = glade_xml_get_property_string (node_in, name);

	if (value == NULL)
	{
		if (xtra == NULL)
			g_warning ("The file did not contained the required property \"%s\"\n"
				   "Under the \"%s\" tag.", name, node->name);
		else
			g_warning ("The file did not contained the required property \"%s\"\n"
				   "Under the \"%s\" tag (%s).", name, node->name, xtra);
	}
	return value;
}

/*
 * Search a child by name,
 */
GladeXmlNode *
glade_xml_search_child (GladeXmlNode *node_in, const gchar *name)
{
	xmlNodePtr node;
	xmlNodePtr child;

	g_return_val_if_fail (node_in != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	node = (xmlNodePtr) node_in;

	for (child = node->children; child; child = child->next) 
	{
		if (!xmlStrcmp (child->name, BAD_CAST(name)))
			return (GladeXmlNode *)child;
	}

	return NULL;
}

/**
 * glade_xml_search_child_required:
 * @tree: 
 * @name: 
 * 
 * just a small wrapper arround glade_xml_search_child that displays
 * an error if the child was not found
 *
 * Return Value: 
 **/
GladeXmlNode *
glade_xml_search_child_required (GladeXmlNode *node, const gchar* name)
{
	GladeXmlNode *child;
	
	child = glade_xml_search_child (node, name);

	if (child == NULL)
		g_warning ("The file did not contained the required tag \"%s\"\n"
			   "Under the \"%s\" node.", name, glade_xml_node_get_name (node));

	return child;
}

/* --------------------------- Parse Context ----------------------------*/

static GladeXmlContext *
glade_xml_context_new_real (GladeXmlDoc *doc, gboolean freedoc, xmlNsPtr ns)
{
	GladeXmlContext *context = g_new0 (GladeXmlContext, 1);

	context->doc = doc;
	context->freedoc = freedoc;
	context->ns  = ns;
	
	return context;
}

GladeXmlContext *
glade_xml_context_new (GladeXmlDoc *doc, const gchar *name_space)
{
	/* We are not using the namespace now */
	return glade_xml_context_new_real (doc, FALSE, NULL);
}

void
glade_xml_context_destroy (GladeXmlContext *context)
{
	g_return_if_fail (context != NULL);
	if (context->freedoc)
		xmlFreeDoc ((xmlDoc*)context->doc);
	g_free (context);
}

GladeXmlContext *
glade_xml_context_new_from_path (const gchar *full_path,
				 const gchar *nspace,
				 const gchar *root_name)
{
	GladeXmlContext *context;
	xmlDocPtr doc;
	xmlNsPtr  name_space;
	xmlNodePtr root;
	
	g_return_val_if_fail (full_path != NULL, NULL);
	
	doc = xmlParseFile (full_path);
	
	/* That's not an error condition.  The file is not readable, and we can't know it
	 * before we try to read it (testing for readability is a call to race conditions).
	 * So we should not print a warning */
	if (doc == NULL)
		return NULL;

	if (doc->children == NULL) {
		g_warning ("Invalid xml File, tree empty [%s]&", full_path);
		xmlFreeDoc (doc);
		return NULL;
	}

	name_space = xmlSearchNsByHref (doc, doc->children, BAD_CAST(nspace));
	if (name_space == NULL && nspace != NULL)
	{
		g_warning ("The file did not contained the expected name space\n"
			   "Expected \"%s\" [%s]",
			   nspace, full_path);
		xmlFreeDoc (doc);
		return NULL;
	}

	root = xmlDocGetRootElement(doc);
	if ((root->name == NULL) || (xmlStrcmp (root->name, BAD_CAST(root_name)) !=0 ))
	{
		g_warning ("The file did not contained the expected root name\n"
			   "Expected \"%s\", actual : \"%s\" [%s]",
			   root_name, root->name, full_path);
		xmlFreeDoc (doc);
		return NULL;
	}
	
	context = glade_xml_context_new_real ((GladeXmlDoc *)doc, TRUE, name_space);

	return context;
}

/**
 * glade_xml_context_free:
 * @context: 
 * 
 * Similar to glade_xml_context_destroy but it also frees the document set in the context
 **/
void
glade_xml_context_free (GladeXmlContext *context)
{
	g_return_if_fail (context != NULL);
	if (context->doc)
		xmlFreeDoc ((xmlDocPtr) context->doc);
	context->doc = NULL;
			
	g_free (context);
}

void
glade_xml_node_append_child (GladeXmlNode *node_in, GladeXmlNode *child_in)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	xmlNodePtr child = (xmlNodePtr) child_in;
	
	g_return_if_fail (node  != NULL);
	g_return_if_fail (child != NULL);
	
	xmlAddChild (node, child);
}

GladeXmlNode *
glade_xml_node_new (GladeXmlContext *context, const gchar *name)
{
	g_return_val_if_fail (context != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	return (GladeXmlNode *) xmlNewDocNode ((xmlDocPtr) context->doc, context->ns, BAD_CAST(name), NULL);
}
					   
void
glade_xml_node_delete (GladeXmlNode *node)
{
	xmlFreeNode ((xmlNodePtr) node);
}

GladeXmlDoc *
glade_xml_context_get_doc (GladeXmlContext *context)
{
	return context->doc;
}

static gboolean
glade_libxml_node_is_comment (xmlNodePtr node) {
	if (node == NULL)
		return FALSE;
	if ((xmlStrcmp ( node->name, BAD_CAST("text")) == 0) ||
	    (xmlStrcmp ( node->name, BAD_CAST("comment")) == 0))
		return TRUE;
	return FALSE;
}

GladeXmlNode *
glade_xml_node_get_children (GladeXmlNode *node_in)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	xmlNodePtr children;

	children = node->children;
	while (glade_libxml_node_is_comment (children))
		children = children->next;

	return (GladeXmlNode *)children;
}

GladeXmlNode *
glade_xml_node_next (GladeXmlNode *node_in)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	node = node->next;
	while (glade_libxml_node_is_comment (node))
		node = node->next;

	return (GladeXmlNode *)node;
}

const gchar *
glade_xml_node_get_name (GladeXmlNode *node_in)
{
	xmlNodePtr node = (xmlNodePtr) node_in;

	return CAST_BAD(node->name);
}

GladeXmlDoc *
glade_xml_doc_new (void)
{
	xmlDocPtr xml_doc = xmlNewDoc (BAD_CAST("1.0"));

	return (GladeXmlDoc *)xml_doc;
}

void
glade_xml_doc_set_root (GladeXmlDoc *doc_in, GladeXmlNode *node_in)
{
	xmlNodePtr node = (xmlNodePtr) node_in;
	xmlDocPtr doc = (xmlDocPtr) doc_in;

	xmlDocSetRootElement (doc, node);
}

gint
glade_xml_doc_save (GladeXmlDoc *doc_in, const gchar *full_path)
{
	xmlDocPtr doc = (xmlDocPtr) doc_in;

	xmlKeepBlanksDefault (0);
	return xmlSaveFormatFile (full_path, doc, 1);
}

void
glade_xml_doc_free (GladeXmlDoc *doc_in)
{
	xmlDocPtr doc = (xmlDocPtr) doc_in;
	
	xmlFreeDoc (doc);
}

/**
 * glade_xml_doc_get_root:
 * @doc: a #GladeXmlDoc
 *
 * Returns: the #GladeXmlNode that is the document root of @doc
 */
GladeXmlNode *
glade_xml_doc_get_root (GladeXmlDoc *doc)
{
	xmlNodePtr node;

	node = xmlDocGetRootElement((xmlDocPtr)(doc));

	return (GladeXmlNode *)node;
}

gchar *
glade_xml_alloc_string(GladeInterface *interface, const gchar *string)
{
    gchar *s;

    s = g_hash_table_lookup(interface->strings, string);
    if (!s) {
        s = g_strdup(string);
        g_hash_table_insert(interface->strings, s, s);
    }

    return s;
}

gchar *
glade_xml_alloc_propname(GladeInterface *interface, const gchar *string)
{
    static GString *norm_str;
    guint i;

    if (!norm_str)
	norm_str = g_string_new_len(NULL, 64);

    /* assign the string to norm_str */
    g_string_assign(norm_str, string);
    /* convert all dashes to underscores */
    for (i = 0; i < norm_str->len; i++)
	if (norm_str->str[i] == '-')
	    norm_str->str[i] = '_';

    return glade_xml_alloc_string(interface, norm_str->str);
}


void
glade_xml_load_sym_from_node (GladeXmlNode     *node_in,
			      GModule          *module,
			      gchar            *tagname,
			      gpointer         *sym_location)
{
	static GModule *self = NULL;
	gchar *buff;

	if (!self) 
		self = g_module_open (NULL, 0);

	if ((buff = glade_xml_get_value_string (node_in, tagname)) != NULL)
	{
		if (!module)
		{
			g_warning ("Catalog specified symbol '%s' for tag '%s', "
				   "no module available to load it from !", 
				   buff, tagname);
			g_free (buff);
			return;
		}

		/* I use here a g_warning to signal these errors instead of a dialog 
		 * box, as if there is one of this kind of errors, there will probably 
		 * a lot of them, and we don't want to inflict the user the pain of 
		 * plenty of dialog boxes.  Ideally, we should collect these errors, 
		 * and show all of them at the end of the load process.
		 *
		 * I dont know who wrote the above in glade-property-class.c, but
		 * its a good point... makeing a bugzilla entry.
		 *  -Tristan
		 *
		 * XXX http://bugzilla.gnome.org/show_bug.cgi?id=331797
		 */
		if (!g_module_symbol (module, buff, sym_location))
			if (!g_module_symbol (self, buff, sym_location))
				g_warning ("Could not find %s in %s or in global namespace\n",
					   buff, g_module_name (module));
		g_free (buff);
	}
}