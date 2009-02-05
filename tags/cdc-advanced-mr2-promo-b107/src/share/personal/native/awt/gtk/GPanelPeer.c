/*
 * @(#)GPanelPeer.c	1.18 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 *
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "jni.h"
#include "sun_awt_gtk_GPanelPeer.h"
#include "GPanelPeer.h"

/******************************Start of Custom Widget For Panel****************************************/

/** Because Java expects components added later to appear behind other components and GTK containers
    do the reverse (which I think is more logical) we need to create our own subclass of GtkContainer which adds
    widgets to the start of the list instead of the end. This also gives us more control over the layout (i.e. we don't
    want to do any layout in gtk as Java should manage this). GtkFixed is a container that essentially does no
    layout so this custom widget is designed using GtkFixed as a guide. The custom widget is called AWTGtkPanel.
    For more information on writing custom widgets please see the Gtk tutorial. */
    
typedef struct _AWTGtkPanel
{
	GtkContainer container;	/* Inherit from GtkContainer. */
	
	GList *children;		/* Stores a list of the child widgets added to the panel. */
} AWTGtkPanel;

typedef struct _AWTGtkPanelClass
{
	GtkContainerClass containerClass;
} AWTGtkPanelClass;

static guint awt_gtk_panel_get_type ();
static void awt_gtk_panel_class_init (AWTGtkPanelClass *panelClass);
static void awt_gtk_panel_init (AWTGtkPanel *panel);
static void awt_gtk_panel_add (GtkContainer *container, GtkWidget *widget);
static void awt_gtk_panel_remove (GtkContainer *container, GtkWidget *widget);
static void awt_gtk_panel_size_allocate (GtkWidget *widget, GtkAllocation *allocation);
static void awt_gtk_panel_size_request (GtkWidget *widget, GtkRequisition *requisition);
static void awt_gtk_panel_forall (GtkContainer *container,
		  gboolean	include_internals,
		  GtkCallback   callback,
		  gpointer      callback_data);
static GtkType awt_gtk_panel_child_type (GtkContainer *container);
static void awt_gtk_panel_realize (GtkWidget *widget);
static void awt_gtk_panel_map (GtkWidget *widget);
static void awt_gtk_panel_draw (GtkWidget *widget, GdkRectangle *area);
static gint awt_gtk_panel_expose (GtkWidget *widget, GdkEventExpose *event);
		  
#define GTK_TYPE_AWT_PANEL              (awt_gtk_panel_get_type ())
#define AWT_GTK_PANEL(obj)              (GTK_CHECK_CAST ((obj), GTK_TYPE_AWT_PANEL, AWTGtkPanel))
#define AWT_GTK_PANEL_CLASS(klass)      (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_AWT_PANEL, AWTGtkPanelClass))

static guint
awt_gtk_panel_get_type ()
{
	static guint type = 0;
	
	if (type == 0)
	{
		GtkTypeInfo info =
		{
			"AWTGtkPanel",
			sizeof (AWTGtkPanel),
			sizeof (AWTGtkPanelClass),
			(GtkClassInitFunc)awt_gtk_panel_class_init,
			(GtkObjectInitFunc)awt_gtk_panel_init,
			NULL,
			NULL
		};
		
		type = gtk_type_unique (gtk_container_get_type(), &info);
	}
	
	return type;
}

static GtkWidget *
awt_gtk_panel_new ()
{
	return GTK_WIDGET(gtk_type_new(awt_gtk_panel_get_type()));
}

static void
awt_gtk_panel_class_init (AWTGtkPanelClass *panelClass)
{
	GtkContainerClass *containerClass = (GtkContainerClass *)panelClass;
	GtkWidgetClass *widgetClass = (GtkWidgetClass *)panelClass;
	
	/* Override widget methods. */
	
	widgetClass->size_allocate = awt_gtk_panel_size_allocate;
	widgetClass->size_request = awt_gtk_panel_size_request;
	widgetClass->map = awt_gtk_panel_map;
  	widgetClass->realize = awt_gtk_panel_realize;
  	widgetClass->draw = awt_gtk_panel_draw;
  	widgetClass->expose_event = awt_gtk_panel_expose;
  	
	/* Override container methods. */
	
	containerClass->add = awt_gtk_panel_add;
	containerClass->remove = awt_gtk_panel_remove;
	containerClass->forall = awt_gtk_panel_forall;
	containerClass->child_type = awt_gtk_panel_child_type;
}

static void
awt_gtk_panel_init (AWTGtkPanel *panel)
{
	GTK_WIDGET_UNSET_FLAGS (panel, GTK_NO_WINDOW);

	panel->children = NULL;
}

static GtkType
awt_gtk_panel_child_type (GtkContainer *container)
{
  return GTK_TYPE_WIDGET;
}

static void
awt_gtk_panel_add (GtkContainer *container, GtkWidget *widget)
{
       AWTGtkPanel *panel = AWT_GTK_PANEL(container);

       g_return_if_fail (panel != NULL);
       g_return_if_fail (widget != NULL);

       gtk_widget_set_parent (widget, GTK_WIDGET (panel));

       /* Add at start of list using g_list_prepend to ensure widgets are added behind
          other widgets if they are added later. */

       panel->children = g_list_prepend (panel->children, widget);

       if (GTK_WIDGET_REALIZED (panel))
               gtk_widget_realize (widget);

       if (GTK_WIDGET_VISIBLE (panel) && GTK_WIDGET_VISIBLE (widget))
       {
               if (GTK_WIDGET_MAPPED (panel))
                       gtk_widget_map (widget);

               gtk_widget_queue_resize (GTK_WIDGET (panel));
       }
}


static void
awt_gtk_panel_remove (GtkContainer *container, GtkWidget *widget)
{
	AWTGtkPanel *panel = AWT_GTK_PANEL(container);
	GtkWidget *child;
	GList *children;
	
	g_return_if_fail (container != NULL);
	g_return_if_fail (widget != NULL);
	
	children = panel->children;
	
	while (children)
	{
		child = children->data;
		
		if (child == widget)
		{
			gboolean was_visible = GTK_WIDGET_VISIBLE (widget);
			
			gtk_widget_unparent (widget);
			
			panel->children = g_list_remove_link (panel->children, children);
			g_list_free (children);
			
			if (was_visible && GTK_WIDGET_VISIBLE (container))
				gtk_widget_queue_resize (GTK_WIDGET (container));
			
			break;
		}
	
		children = children->next;
	}
}

static void
awt_gtk_panel_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	AWTGtkPanel *panel = AWT_GTK_PANEL(widget);
	GtkWidget *child;
	GList *children;
	GtkAllocation childAllocation;
	
	g_return_if_fail (widget != NULL);
	
  	widget->allocation = *allocation;
  
  	if (GTK_WIDGET_REALIZED (widget))
    	gdk_window_move_resize (widget->window,
			    allocation->x,
			    allocation->y,
			    allocation->width,
			    allocation->height);

	children = panel->children;
	
	/* It is a requiremnt in Gtk that when a container is allocated a size it must call
	   gtk_widget_size_allocate for all its children. As we don't want to do layout of widgets
	   in this custom container we simply ask the child to allocate its size and position using
	   its current size and position (thus not affecting the layout). */
	
	while (children)
	{
		child = children->data;
		childAllocation = child->allocation;
		gtk_widget_size_allocate (child, &childAllocation);
		
		children = children->next;
	}
}

/* Overridden to return the current size of the panel. */

static void
awt_gtk_panel_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	AWTGtkPanel *panel = AWT_GTK_PANEL(widget);
	GtkWidget *child;
	GList *children;
	GtkRequisition childRequisition;
	
	requisition->width = widget->allocation.width;
	requisition->height = widget->allocation.height;
	
	children = panel->children;
	
	/* It is a requirement in Gtk that when a container is asked for its requested size
	   it should ask its children for their preferred sizes too. Java actually takes care of
	   this but to be consistent with Gtk we ask the children what their preferred size
	   is and simply ignore it. */
	
	while (children)
	{
		child = children->data;
		gtk_widget_size_request (child, &childRequisition);
		
		children = children->next;
	}
}

static void
awt_gtk_panel_map (GtkWidget *widget)
{
  AWTGtkPanel *panel;
  GtkWidget *child;
  GList *children;

  g_return_if_fail (widget != NULL);

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);
  panel = AWT_GTK_PANEL (widget);

  children = panel->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      if (GTK_WIDGET_VISIBLE (child) &&
	  !GTK_WIDGET_MAPPED (child))
	gtk_widget_map (child);
    }

  gdk_window_show (widget->window);
}


static void
awt_gtk_panel_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	
	g_return_if_fail (widget != NULL);
	
	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
	
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
	
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	
	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes,
				   attributes_mask);
	gdk_window_set_user_data (widget->window, widget);
	
	widget->style = gtk_style_attach (widget->style, widget->window);
	gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
}

static void
awt_gtk_panel_paint (GtkWidget    *widget,
		 GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    gdk_window_clear_area (widget->window,
			   area->x, area->y,
			   area->width, area->height);
}

static void
awt_gtk_panel_draw (GtkWidget    *widget,
		GdkRectangle *area)
{
  AWTGtkPanel *panel;
  GtkWidget *child;
  GdkRectangle child_area;
  GList *children;

  g_return_if_fail (widget != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      panel = AWT_GTK_PANEL (widget);
      awt_gtk_panel_paint (widget, area);

      children = panel->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (gtk_widget_intersect (child, area, &child_area))
	    gtk_widget_draw (child, &child_area);
	}
    }
}

static gint
awt_gtk_panel_expose (GtkWidget      *widget,
		  GdkEventExpose *event)
{
  AWTGtkPanel *panel;
  GtkWidget *child;
  GdkEventExpose child_event;
  GList *children;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      panel = AWT_GTK_PANEL (widget);

      child_event = *event;

      children = panel->children;
      while (children)
	{
	  child = children->data;
	  children = children->next;

	  if (GTK_WIDGET_NO_WINDOW (child) &&
	      gtk_widget_intersect (child, &event->area,
				    &child_event.area))
	    gtk_widget_event (child, (GdkEvent*) &child_event);
	}
    }

  return FALSE;
}

static void
awt_gtk_panel_forall (GtkContainer *container,
		  gboolean	include_internals,
		  GtkCallback   callback,
		  gpointer      callback_data)
{
	AWTGtkPanel *panel;
	GList *children;
	GtkWidget *child;
	
	g_return_if_fail (container != NULL);
	g_return_if_fail (callback != NULL);
	
	panel = AWT_GTK_PANEL(container);
	
	children = panel->children;
	
	while (children)
	{
		child = children->data;
	  	children = children->next;
	
	  	(* callback) (child, callback_data);
	}
}

/******************************End of Custom Widget For Panel****************************************/

/* Initializes the PanelPeerData. This creates the panel widget and initializes the ComponentPeerData.
   If widget, drawWidget or styleWidget are NULL then then panel widget will be used for them. If
   canAddToPanel is false then a panel widget will not be created and components cannot be added
   to the panel. This is useful for subclasses of panel for which it is not possible to add components
   to (such as FileDialog). */

jboolean
awt_gtk_GPanelPeerData_init (JNIEnv *env,
						     jobject this,
						     GPanelPeerData *data,
						     GtkWidget *widget,
						     GtkWidget *drawWidget,
						     jboolean createPanel)
{
	/* Only create the panel (a custom gtk container widget) if we need to.
	   This is necessary because some subclasses of panel cannot have components added to them.
	   For example, one cannot add components to a file dialog. */
	
	if (createPanel)
	{
		GtkWidget *panel;
	
		data->panelWidget = panel = awt_gtk_panel_new ();
                /* make panel focusable. */
                GTK_WIDGET_SET_FLAGS (panel, GTK_CAN_FOCUS);
		gtk_widget_show (panel);
		
		if (widget != NULL)
			gtk_container_add (GTK_CONTAINER(widget), panel);
		
		/* If the main widget and draw widget have not been specified then use the panel. */
		
		if (widget == NULL)
			widget = panel;
		
		if (drawWidget == NULL)
			drawWidget = panel;
	}
	
	else
		data->panelWidget = NULL;
	
	if (!awt_gtk_GComponentPeerData_init (env, this, (GComponentPeerData *)data, widget, drawWidget, widget, JNI_FALSE))
		return JNI_FALSE;
	
	return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GPanelPeer_create (JNIEnv *env, jobject this)
{
	GPanelPeerData *data = (GPanelPeerData *)calloc (1, sizeof (GPanelPeerData));
	
	if (data == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.OutOfMemoryErrorClass, NULL);
		return;
	}
	
	awt_gtk_threadsEnter();
	awt_gtk_GPanelPeerData_init (env, this, data, NULL, NULL, JNI_TRUE);
	awt_gtk_threadsLeave();
}

/* Gtk does not support inserting widgets at arbitrary indexes in a container. To overcome this
   we have written our own insertion code. It needs to use X APIs to ensure the X windows are
   in the correct stacking order. */

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GPanelPeer_insert (JNIEnv *env, jobject this, jobject peer, jint index)
{
	AWTGtkPanel *panel;
	GPanelPeerData *data;
	GComponentPeerData *peerData;
	
	data = (GPanelPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	peerData = awt_gtk_getData (env, peer);
	
	if (peerData == NULL)
		return;
	
	if ((panel = (AWTGtkPanel *)data->panelWidget) != NULL)
	{
		GtkWidget *widget;
		
		awt_gtk_threadsEnter();
		
		widget = peerData->widget;

		g_return_if_fail (widget != NULL);
		
		gtk_widget_set_parent (widget, GTK_WIDGET (panel));
		
		/* Insert the widget in the correct position in the list of children for the panel. */
		
		panel->children = g_list_insert (panel->children, widget, (gint)index);
		
		/* If the panel is realized (GdkWindow created) then we need to realize the widget too.
		   Unfortunately, the GdkWindow will always be created at the end of the list of windows
		   for the parent and therefore won't be inserted in the correct position. Gdk also does
		   not provide a means of reordering the window. To get around this we have to use X windows
		   API instead as this provides the XRestackWindows function. Gtk does not provide an API
		   for inserting widgets in a container either. It would be nice to do this before the
		   window is mapped but Gtk raises the window prior to mapping it so we must do it after
		   has been mapped. */
		
		if (GTK_WIDGET_REALIZED (panel))
			gtk_widget_realize (widget);
		
		if (GTK_WIDGET_VISIBLE (panel) && GTK_WIDGET_VISIBLE (widget))
		{
			if (GTK_WIDGET_MAPPED (panel))
			{
				GList *children;
				Window *childWindows = malloc(sizeof(Window) * g_list_length(panel->children));
				int numChildWindows = 0;
				Display *display = NULL;
				
				gtk_widget_map (widget);
				
				/* Restack windows to ensure correct stacking order. */
			
				children = g_list_last(panel->children);
				
				while (children)
				{
					GtkWidget *child = (GtkWidget *)children->data;
					GdkWindow *window = child->window;
					
					if (window != NULL)
					{
						Window xwindow = GDK_WINDOW_XWINDOW(window);
						
						if (display == NULL)
							display = GDK_WINDOW_XDISPLAY(window);
						
						childWindows[numChildWindows++] = xwindow;
					}
					
					children = g_list_previous(children);
				}
				
				if (numChildWindows > 0)
					XRestackWindows (display, childWindows, numChildWindows);
				
				free (childWindows);
			}
		  
			gtk_widget_queue_resize (GTK_WIDGET (panel));
		}
		awt_gtk_threadsLeave();
	}
}


JNIEXPORT void JNICALL
Java_sun_awt_gtk_GPanelPeer_delete (JNIEnv *env, jobject this, jobject peer)
{
	AWTGtkPanel *panel;
	GPanelPeerData *data;
	GComponentPeerData *peerData;
	
	data = (GPanelPeerData *)awt_gtk_getData (env, this);
	
	if (data == NULL)
		return;
	
	peerData = awt_gtk_getData (env, peer);
	
	if (peerData == NULL)
		return;
	
	if ((panel = (AWTGtkPanel *)data->panelWidget) != NULL)
	{
		GtkWidget *widget;
		
		awt_gtk_threadsEnter();
		
		widget = peerData->widget;
		
		g_return_if_fail (widget != NULL);
		
		gtk_widget_ref(widget);
		gtk_container_remove(GTK_CONTAINER (panel), widget);
				
		panel->children = g_list_remove (panel->children, widget);
							
		if (GTK_WIDGET_VISIBLE (panel) && GTK_WIDGET_MAPPED (panel))
		{
			GList *children;
			Window *childWindows = malloc(sizeof(Window) * g_list_length(panel->children));
			int numChildWindows = 0;
			Display *display = NULL;
						
			/* Restack windows to ensure correct stacking order. */
			
			children = g_list_last(panel->children);
			
			while (children)
			{
				GtkWidget *child = (GtkWidget *)children->data;
				GdkWindow *window = child->window;
				
				if (window != NULL)
				{
					Window xwindow = GDK_WINDOW_XWINDOW(window);
					
					if (display == NULL)
						display = GDK_WINDOW_XDISPLAY(window);
					
					childWindows[numChildWindows++] = xwindow;
				}
				
				children = g_list_previous(children);
			}
			
			if (numChildWindows > 0)
				XRestackWindows (display, childWindows, numChildWindows);
			
			free (childWindows);
			
			gtk_widget_queue_resize (GTK_WIDGET (panel));
		}
		awt_gtk_threadsLeave();
	}
}

