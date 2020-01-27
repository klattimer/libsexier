/*******************************************************************************
 * Fittsmenu GtkWidget
 * Author: Karl Lattimer <karl@qdh.org.uk>
 * Date: August 4 2007
 * 
 *   This is the first to my knowledge radial menu widget, this popup menu came
 *   about as a result of an interest in radial menus mentioned in human 
 *   interaction studies. 
 * 
 *   With fittsmenu a little twist has been added, as the cursor approaches
 *   another icon in the menu, the menu rotates toward the cursor, therefore
 *   reducing the distance to the target and therefore the time.
 * 
 *   When crossing through the centre of the menu the rotation halts.
 * 
 * The new library: libsexier
 * 
 *   Fittsmenu has been developed as a jumping off point for a new generation
 *   of GtkWidgets based on cairo. A number of widgets have been considered and
 *   a feature list will be developed and published ASAP.
 * 
 * TODO:
 *   * Update glitz code, add nvidia detection, when nvidia - use glitz
 *   * Add some autodoc comments
 *   * Profile it (valgrind/sysprof)
 *   * Get it reviewed for accuracy
 * 
 ******************************************************************************/
#if HAVE_CONFIG
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <cairo.h>
#include <glib.h>
#include <pango/pango.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkwidget.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>
#include <string.h>

/**
 * Include glitz if available
 */
#ifdef USE_GLITZ
#include <gdk/gdkx.h>
#include <glitz-glx.h>
#include <cairo-glitz.h>
#endif

#include "fittsmenu.h"

#define FITTSMENU_MIN_WIDTH 160

static void fittsmenu_class_intern_init(gpointer);
static void fittsmenu_class_init (FittsmenuClass*);
static void fittsmenu_init (GtkWidget *widget);
static void fittsmenu_get_property (GObject*, guint, GValue*, GParamSpec*);
static void fittsmenu_set_property (GObject*, guint, const GValue*, GParamSpec*);
         
static void fittsmenu_realize (GtkWidget*);
static void fittsmenu_size_request (GtkWidget*, GtkRequisition*);
static void fittsmenu_size_allocate (GtkWidget*, GtkAllocation*);
static void fittsmenu_show (GtkWidget *widget);
static gint fittsmenu_expose (GtkWidget*, GdkEventExpose*);
static gint fittsmenu_scroll (GtkWidget *widget, GdkEventExpose *event);
static gint fittsmenu_key_press (GtkWidget *widget, GdkEventKey *event);
static gint fittsmenu_button_press (GtkWidget *widget, GdkEventButton *event);
static gint fittsmenu_button_release (GtkWidget *widget, GdkEventButton *event);
static gint fittsmenu_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static void fittsmenu_show_all (GtkWidget *widget);
static void fittsmenu_hide_all (GtkWidget *widget);
static void fittsmenu_dispose (GObject *obj);
static void fittsmenu_finalize (GObject *obj);
static gboolean fittsmenu_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean fittsmenu_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean fittsmenu_grab_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean fittsmenu_focus (GtkWidget *widget, GdkEventFocus *event);
static void set_alpha (GtkWidget *widget);
static cairo_t *my_cairo_create (GdkWindow* window, Fittsmenu *fittsmenu);
static void canvas_reset(cairo_t* cr);
#ifdef USE_GLITZ
static void swap_buffers(Fittsmenu *fittsmenu);
#endif //USE_GLITZ
static void render(cairo_t* cr, Fittsmenu *fittsmenu);
static void recpol(gdouble x, gdouble y, gdouble *pr, gdouble *pa);
static void polrec(gdouble r, gdouble a, gdouble *px, gdouble *py);
long get_time (void);
static gboolean fittsmenu_window_event (GtkWidget *window, GdkEvent *event, GtkWidget *fittsmenu);
static void fittsmenu_window_size_request (GtkWidget *window, GtkRequisition *requisition, Fittsmenu *fittsmenu);
#define FITTSMENU_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), FITTSMENU_TYPE, FittsmenuPrivate))

typedef struct _FittsmenuPrivate  FittsmenuPrivate;

struct _FittsmenuPrivate 
{
  GList*         slices;
  
  fittsmenu_slice*      active;
  fittsmenu_slice*      hover;
  
  /* Widget Geometry */
  gdouble        slice_width;
  gdouble        menu_radius;
  gdouble        menu_inner_radius;
  
  gint			 animation;
  
  /* Widget State */
  gboolean       menu_over;
  gint           menu_angle;
  gint           menu_angle_offset;
  gint           menu_angle_diff;
  
  /* Last known mouse state */
  gdouble        mouse_angle;
  gdouble        mouse_distance;

  /* Window Geometry */
  gint           window_x;
  gint           window_y;
  gint           window_size; // Width/Height are the same

  /* Glitz Primitives */
#ifdef USE_GLITZ
  gboolean                  nv_use_glitz;
  glitz_drawable_format_t*  glitz_drawable_format;
  glitz_drawable_t*         glitz_drawable;
  glitz_format_t*           glitz_format;
#endif // USE_GLITZ

	gboolean dispose_has_run;
	long last_redraw;
};

enum
{
  TICK_SIGNAL,
  REVOLUTION_SIGNAL,
  CLICKED_SIGNAL,
  LAST_SIGNAL
};

static guint fittsmenu_signals[LAST_SIGNAL] = { 0 };

static gpointer fittsmenu_parent_class = NULL;

enum
{
  PROP_0,
  PROP_MENU_RADIUS,
  PROP_MENU_INNER_RADIUS,
  PROP_MENU_ANIMATION
};

/* Get a GType that corresponds to Fittsmenu. The first time this function is
 * called (on object instantiation), the type is registered. */
GType
fittsmenu_get_type () {
  static GType fittsmenu_type = 0;

  if (!fittsmenu_type) {
    static const GTypeInfo fittsmenu_info =
    {
      sizeof (FittsmenuClass),
      NULL, 
      NULL,
      (GClassInitFunc) fittsmenu_class_intern_init,
      NULL, 
      NULL,
      sizeof (Fittsmenu),
      0,
      (GInstanceInitFunc) fittsmenu_init,
    };

    fittsmenu_type = g_type_register_static (GTK_TYPE_WIDGET, "Fittsmenu", 
                                             &fittsmenu_info, 0);
  }

  return fittsmenu_type;
}

static void fittsmenu_class_intern_init(gpointer klass)
{
  fittsmenu_parent_class = g_type_class_peek_parent(klass);
  fittsmenu_class_init((FittsmenuClass*) klass);
}

/* Initialize the FittsmenuClass class by overriding standard functions,
 * registering a private class and setting up signals and properties. */
static void
fittsmenu_class_init (FittsmenuClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = (GObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  /* Override the standard functions for setting and retrieving properties. */
  gobject_class->set_property = fittsmenu_set_property;
  gobject_class->get_property = fittsmenu_get_property;

  /* Override the standard functions for realize, expose, and size changes. */
  widget_class->realize = fittsmenu_realize;
  widget_class->size_request = fittsmenu_size_request;
  widget_class->size_allocate = fittsmenu_size_allocate;  
  widget_class->show = fittsmenu_show;
  widget_class->expose_event = fittsmenu_expose;
  widget_class->key_press_event = fittsmenu_key_press;
  widget_class->button_press_event = fittsmenu_button_press;
  widget_class->button_release_event = fittsmenu_button_release;
  widget_class->motion_notify_event = fittsmenu_motion_notify;  
  widget_class->show_all = fittsmenu_show_all;
  widget_class->hide_all = fittsmenu_hide_all;
  widget_class->enter_notify_event = fittsmenu_enter_notify;
  widget_class->leave_notify_event = fittsmenu_leave_notify;
  
  
  gobject_class->dispose = fittsmenu_dispose;
  gobject_class->finalize = fittsmenu_finalize;
  
  //widget_class->style_set = fittsmenu_style_set;
  //widget_class->focus = fittsmenu_focus;
  //widget_class->can_activate_accel = fittsmenu_real_can_activate_accel;
  //widget_class->grab_notify = fittsmenu_grab_notify;
  
  /* Add FittsmenuPrivate as a private data class of FittsmenuClass. */
  g_type_class_add_private (klass, sizeof (FittsmenuPrivate));

  fittsmenu_signals[TICK_SIGNAL] = 
           g_signal_new ("tick-signal",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                         G_STRUCT_OFFSET (FittsmenuClass, ticked),
                         NULL, NULL, g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);
                         
  fittsmenu_signals[REVOLUTION_SIGNAL] = 
           g_signal_new ("revolution-signal",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                         G_STRUCT_OFFSET (FittsmenuClass, revolved),
                         NULL, NULL, g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);

  fittsmenu_signals[CLICKED_SIGNAL] = 
           g_signal_new ("clicked-signal",
                         G_TYPE_FROM_CLASS (klass),
                         G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                         G_STRUCT_OFFSET (FittsmenuClass, clicked),
                         NULL, NULL, g_cclosure_marshal_VOID__VOID,
                         G_TYPE_NONE, 0);
  
  g_object_class_install_property (gobject_class, PROP_MENU_RADIUS,
              g_param_spec_double ("menu-radius",
                                   "Menu Radius",
                                   "Radius of the entire menu",
                                   80, 300, 150,
                                   G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MENU_INNER_RADIUS,
              g_param_spec_double ("menu-inner-radius",
                                   "Menu Inner Radius",
                                   "Radius of the inner disc",
                                   20, 260, 60,
                                   G_PARAM_READWRITE));
                                   
  g_object_class_install_property (gobject_class, PROP_MENU_ANIMATION,
              g_param_spec_double ("animation",
                                   "Menu animation",
                                   "How should the menu animate",
                                   0, 5, 1,
                                   G_PARAM_READWRITE));
 }

/* Initialize the actual Fittsmenu widget. This function is used to setup
 * the initial view of the widget and set necessary properties. */
static void
fittsmenu_init (GtkWidget *widget)
{
  Fittsmenu *fittsmenu = FITTSMENU(widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  rsvg_init();
  priv->menu_radius = 120;
  priv->menu_inner_radius = 80;
  priv->window_size = priv->menu_radius * 2;
  priv->menu_angle = 0;
  priv->menu_angle_offset = 0;
  priv->mouse_angle = 0;
  priv->mouse_distance = 0;
  priv->menu_over = FALSE;
  priv->animation = FITTSMENU_ANIM_CROTATE;
  priv->dispose_has_run = FALSE;
  priv->last_redraw = get_time() - 30000;
  
#ifdef USE_GLITZ
  priv->nv_use_glitz = FALSE;
#endif

  /* Create window priv->toplevel */
  fittsmenu->toplevel = g_object_connect (g_object_new (GTK_TYPE_WINDOW,
                                                        "type", GTK_WINDOW_POPUP,
                                                        "child", fittsmenu,
                                                        NULL),
                                          "object-signal::event", fittsmenu_window_event, fittsmenu,
                                          "object-signal::size_request", fittsmenu_window_size_request, fittsmenu,
                                          "signal::destroy", gtk_widget_destroyed, &fittsmenu->toplevel,
                                          NULL);

#ifdef USE_GLITZ
  int                       screen = 0, i;
  unsigned long             mask = 0;
  glitz_drawable_format_t   tmp_gdf, *format;
  glitz_format_t            tmp_gf;
  GdkVisual                 *visual;
  GdkColormap               *colormap;
  GdkDisplay                *gdkdisplay;
  Display                   *xdisplay;
  Window                    xid;
  XVisualInfo               *vinfo = NULL;

  if (priv->nv_use_glitz) {
    tmp_gdf.doublebuffer = 1;
    mask |= GLITZ_FORMAT_DOUBLEBUFFER_MASK;
    
    gdkdisplay = gtk_widget_get_display (fittsmenu->toplevel);
    xdisplay   = gdk_x11_display_get_xdisplay (gdkdisplay);
    
    i = 0;
    do {
      format = glitz_glx_find_window_format (xdisplay, screen,
                                             mask, &tmp_gdf, i++);
      if (format) {
        vinfo = glitz_glx_get_visual_info_from_format (xdisplay,
                                                       screen,
                                                       format);
        if (vinfo->depth == 32) {
          priv->glitz_drawable_format = format;
          break;
        } else {
          if (!priv->glitz_drawable_format)
            priv->glitz_drawable_format = format;
        }
      }
    } while (format);
    
    if (!priv->glitz_drawable_format)
      fprintf (stderr, "no double buffered GLX visual\n");
    
    vinfo = glitz_glx_get_visual_info_from_format (xdisplay,
                                                   screen,
                                                   priv->glitz_drawable_format);
    
    visual = gdkx_visual_get (vinfo->visualid);
    colormap = gdk_colormap_new (visual, TRUE);
    
    gtk_widget_set_colormap (fittsmenu->toplevel, colormap);
    gtk_widget_set_double_buffered (fittsmenu->toplevel, FALSE);
  }
#else
  set_alpha(fittsmenu->toplevel);
#endif /* USE_GLITZ */

  /* Window properties */
  gtk_widget_set_app_paintable (fittsmenu->toplevel, TRUE);
  gtk_window_set_decorated (GTK_WINDOW (fittsmenu->toplevel), FALSE);
    
  gtk_window_set_title (GTK_WINDOW (fittsmenu->toplevel), "Fitts' menu");

#ifdef USE_GLITZ

  if (priv->nv_use_glitz) {
    xid = gdk_x11_drawable_get_xid (fittsmenu->toplevel->window);

    priv->glitz_drawable = glitz_glx_create_drawable_for_window (xdisplay,
                                 0,
                                 priv->glitz_drawable_format,
                                 xid,
                                 priv->window_size,
                                 priv->window_size);
    if (!priv->glitz_drawable)
      fprintf (stderr, "failed to create drawable\n");

    tmp_gf.color        = priv->glitz_drawable_format->color;
    tmp_gf.color.fourcc = GLITZ_FOURCC_RGB;

    priv->glitz_format = glitz_find_format (priv->glitz_drawable,
                               GLITZ_FORMAT_RED_SIZE_MASK   |
                               GLITZ_FORMAT_GREEN_SIZE_MASK |
                               GLITZ_FORMAT_BLUE_SIZE_MASK  |
                               GLITZ_FORMAT_ALPHA_SIZE_MASK |
                               GLITZ_FORMAT_FOURCC_MASK,
                               &tmp_gf,
                               0);
    if (!priv->glitz_format)
      fprintf (stderr, "couldn't find surface format\n");
  }
#endif
                                
}

/* This function is called when the programmer gives a new value for a widget
 * property with g_object_set(). */
static void
fittsmenu_set_property (GObject *object,
                         guint prop_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  Fittsmenu *fittsmenu = FITTSMENU (object);

  switch (prop_id)
  {
    case PROP_MENU_RADIUS:
      fittsmenu_set_menu_radius (fittsmenu, g_value_get_int (value));
      break;
    case PROP_MENU_INNER_RADIUS:
      fittsmenu_set_menu_inner_radius (fittsmenu, g_value_get_int (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* This function is called when the programmer requests the value of a widget
 * property with g_object_get(). */
static void
fittsmenu_get_property (GObject *object,
                         guint prop_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  Fittsmenu *fittsmenu = FITTSMENU (object);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);

  switch (prop_id)
  {
    case PROP_MENU_RADIUS:
      g_value_set_int (value, priv->menu_radius);
      break;
    case PROP_MENU_INNER_RADIUS:
      g_value_set_int (value, priv->menu_inner_radius);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* Create and return a new instance of the Fittsmenu widget. */
Fittsmenu*
fittsmenu_new (void)
{
  return g_object_new (fittsmenu_get_type (), NULL);
}

/* Called when the widget is realized. This usually happens when you call
 * gtk_widget_show() on the widget. */
static void
fittsmenu_realize (GtkWidget *widget)
{
  Fittsmenu *fittsmenu;
  GdkWindowAttr attributes;
  gint attr_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (IS_FITTSMENU (widget));

  /* Set the GTK_REALIZED flag so it is marked as realized. */
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  fittsmenu = FITTSMENU (widget);

  /* Create a new GtkWindowAttr object that will hold info about the GdkWindow. */
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= (GDK_EXPOSURE_MASK);
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  
  /* Create a new GdkWindow for the widget. */
  attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attr_mask);
  gdk_window_set_user_data (widget->window, fittsmenu);

  /* Attach a style to the GdkWindow and draw a background color. */
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);
  /* Not sure this should happen here...when a widget is shown it is first realized
   * then mapped. The default map function for a widget with a window calls this function*/
  //gdk_window_show (widget->window);
}

/* Handle size requests for the widget. This function forces the widget to have
 * an initial size set according to the predefined width and the font size. */
static void 
fittsmenu_size_request (GtkWidget *widget,
                        GtkRequisition *requisition)
{
  g_return_if_fail (widget != NULL || requisition != NULL);
  g_return_if_fail (IS_FITTSMENU (widget));

  requisition->width = FITTSMENU_MIN_WIDTH;
  requisition->height = FITTSMENU_MIN_WIDTH;
}

/* Handle size allocations for the widget. This does the actual resizing of the
 * widget to the requested allocation. */
static void
fittsmenu_size_allocate (GtkWidget *widget,
                          GtkAllocation *allocation)
{
  Fittsmenu *fittsmenu = FITTSMENU(widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  g_return_if_fail (widget != NULL || allocation != NULL);
  g_return_if_fail (IS_FITTSMENU (widget));

  widget->allocation = *allocation;
  fittsmenu = FITTSMENU (widget);

  if (GTK_WIDGET_REALIZED (widget))
  {
    gdk_window_move_resize (widget->window, allocation->x, allocation->y, 
                            allocation->width, allocation->height);
  }
}
/* There is a GObject parent class peek function that I think is
 * for chaining to overriden functions.
 */
static void
fittsmenu_show (GtkWidget *widget)
{
  Fittsmenu *fittsmenu = FITTSMENU (widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  gint x;
  gint y;
  
  GdkDisplay *display;
  GdkScreen  *screen;
  
  display = gdk_display_get_default();
  gdk_display_get_pointer (display, &screen, &x, &y, NULL);
  
  priv->window_x = x - priv->menu_radius;
  priv->window_y = y - priv->menu_radius;

  GTK_WIDGET_CLASS (fittsmenu_parent_class)->show (widget);
                                                         
  fittsmenu_popup(fittsmenu, 0);
}

/* Events */
static gboolean
fittsmenu_window_event (GtkWidget *window,
                        GdkEvent  *event,
                        GtkWidget *fittsmenu)
{
  gboolean handled = FALSE;

  g_object_ref (window);
  g_object_ref (fittsmenu);

  switch (event->type)
    {
    case GDK_KEY_PRESS:
    case GDK_KEY_RELEASE:
      handled = gtk_widget_event (fittsmenu, event);
      break;
    default:
      break;
    }

  g_object_unref (window);
  g_object_unref (fittsmenu);

  return handled;
}

static void
fittsmenu_window_size_request (GtkWidget      *window,
                               GtkRequisition *requisition,
                               Fittsmenu      *fittsmenu)
{/*
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  if (priv->have_position)
    {
      GdkScreen *screen = gtk_widget_get_screen (window);
      GdkRectangle monitor;
      
      gdk_screen_get_monitor_geometry (screen, private->monitor_num, &monitor);

      if (priv->window_y + requisition->height > monitor.y + monitor.height)
        requisition->height = monitor.y + monitor.height - priv->window_y;

      if (private->y < monitor.y)
        requisition->height -= monitor.y - priv->window_y;
    }
  */
}

/* This function is called when an expose-event occurs on the widget. This means
 * that a part of the widget that was previously hidden is shown. */
static gint
fittsmenu_expose (GtkWidget *widget,
                  GdkEventExpose *event)
{
  Fittsmenu *fittsmenu = FITTSMENU (widget);
  cairo_t* cr = NULL;
    
  cr = my_cairo_create (widget->window, fittsmenu);
  if (!cr)
    return FALSE;

  canvas_reset(cr);
  render (cr, fittsmenu);

#ifdef USE_GLITZ
    /* swap the buffers after redraw */
  swap_buffers(fittsmenu);
#endif // USE_GLITZ
  cairo_destroy (cr);
  
  return FALSE;
}

// On wheel rotate rotate the menu by one icon per
// wheel pulse
static gint
fittsmenu_scroll (GtkWidget *widget,
                  GdkEventExpose *event)
{
}

static gint
fittsmenu_key_press (GtkWidget *widget,
                     GdkEventKey *event)
{
}

static gint
fittsmenu_button_press (GtkWidget *widget,
                        GdkEventButton *event)
{
}

static gint
fittsmenu_button_release (GtkWidget *widget,
                          GdkEventButton *event)
{
	Fittsmenu *fittsmenu = FITTSMENU (widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);

  if (event->type == GDK_BUTTON_RELEASE) {
    switch (event->button) {
      case 1: // Left
				priv->active = priv->hover;
        fittsmenu_popdown(fittsmenu);
        g_signal_emit_by_name ((gpointer) fittsmenu, "clicked-signal");
      break;

      case 2: //Middle
      break;

      case 3: //Right
      break;
    }
  }

  return FALSE;  
}

static gint
fittsmenu_motion_notify (GtkWidget *widget,
                         GdkEventMotion *event)
{
  Fittsmenu *fittsmenu = FITTSMENU (widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  // If we've redrawn less than 30000 usec ago don't redraw (33fps)
  if (get_time() < priv->last_redraw + 30000)
  	return TRUE; 
  
  gint mouse_x, mouse_y;
  cairo_t* cr;
  gint cx, cy;
  gdouble rx, ry;
  
  gdk_window_get_pointer (widget->window, &mouse_x, &mouse_y, NULL);

  cx = priv->menu_radius;
  cy = priv->menu_radius;

  ry = (cx - mouse_x)*-1;
  rx = (cy - mouse_y);
    
  recpol(rx,ry, &priv->mouse_distance, &priv->mouse_angle);
  priv->mouse_angle = priv->mouse_angle * (180.0f/G_PI);
    
  // Left the menu area 
  if ((priv->mouse_distance > priv->menu_radius) 
      || (priv->mouse_distance < priv->menu_inner_radius - 10)) {
    // Save the current position, to ensure when we enter the menu 
    // doesn't jump around 
    if (priv->animation == FITTSMENU_ANIM_CROTATE) {
      priv->menu_angle_offset = priv->menu_angle;
      priv->menu_angle_diff = 0;
    }
    priv->menu_over = FALSE;
    gtk_widget_queue_draw (widget);
    return TRUE;
  }
    
  // Enter the menu area 
  if (!priv->menu_over) {
    // Apply the saved position on enter to the current menu angle
    priv->menu_over = TRUE;
		if (priv->animation == FITTSMENU_ANIM_CROTATE) {
      priv->menu_angle_diff = (priv->menu_angle_offset + priv->mouse_angle)*-1;
      priv->menu_angle_diff = priv->menu_angle_diff % 360;
		}
  }
  // calculate the current menu angle based on the mouse angle 
  if (priv->animation == FITTSMENU_ANIM_CROTATE)
  	priv->menu_angle = (priv->mouse_angle + priv->menu_angle_diff)*-1;

  // make sure the menuangle isn't negative or too large
  priv->menu_angle = priv->menu_angle % 360;
  if (priv->menu_angle < 0) 
    priv->menu_angle = priv->menu_angle + 360;

  priv->last_redraw = get_time();
  gtk_widget_queue_draw (widget);
  return TRUE;
}

static gboolean
fittsmenu_enter_notify (GtkWidget        *widget,
                       GdkEventCrossing *event)
{
  Fittsmenu *fittsmenu = FITTSMENU (widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
	
	priv->menu_angle_offset = priv->menu_angle;
  priv->menu_over = FALSE;
}

static gboolean
fittsmenu_leave_notify (GtkWidget        *widget,
                       GdkEventCrossing *event)
{
	Fittsmenu *fittsmenu = FITTSMENU (widget);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  priv->menu_over = FALSE;
}

static gboolean
fittsmenu_grab_notify (GtkWidget        *widget,
                       GdkEventCrossing *event)
{
}

static gboolean
fittsmenu_focus       (GtkWidget        *widget,
                       GdkEventFocus     *event)
{
}

static void
fittsmenu_show_all (GtkWidget *widget)
{
  /* Show children, but not self. */
  gtk_container_foreach (GTK_CONTAINER (widget), (GtkCallback) gtk_widget_show_all, NULL);
}


static void
fittsmenu_hide_all (GtkWidget *widget)
{
  /* Hide children, but not self. */
  gtk_container_foreach (GTK_CONTAINER (widget), (GtkCallback) gtk_widget_hide_all, NULL);
}

/* Actions */

/**
 * Cache the icon handles and buffer the svg file
 */
void
fittsmenu_append     (Fittsmenu *fittsmenu, fittsmenu_slice *slice)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);

  slice->icon_buffer = NULL;
  priv->slices = g_list_append(priv->slices, (gpointer)slice);
	slice->index = g_list_index(priv->slices, slice);
}

void
fittsmenu_remove     (Fittsmenu *fittsmenu, fittsmenu_slice *slice)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  priv->slices = g_list_remove(priv->slices, slice);
  rsvg_handle_free (slice->icon_handle);
  cairo_surface_destroy(slice->icon_buffer);
  g_free(slice);
}

void
fittsmenu_popup      (Fittsmenu *fittsmenu, guint button)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  if (g_list_length(priv->slices) < 1)
  	return;
  
  gtk_window_resize(GTK_WINDOW(fittsmenu->toplevel), priv->window_size, priv->window_size);
  gtk_window_move(GTK_WINDOW(fittsmenu->toplevel), priv->window_x, priv->window_y);
  gtk_widget_show(fittsmenu->toplevel);
  
  gdk_pointer_grab (GTK_WIDGET(fittsmenu)->window, TRUE,
		 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
		 GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK |
		 GDK_POINTER_MOTION_MASK,
		 NULL, NULL, 0);
}

void
fittsmenu_popdown    (Fittsmenu *fittsmenu)
{
  gdk_display_pointer_ungrab (gdk_display_get_default(), GDK_CURRENT_TIME);
  gdk_display_keyboard_ungrab (gdk_display_get_default(), GDK_CURRENT_TIME);
  gtk_widget_hide(fittsmenu->toplevel);
}


/* Getters/Setters */
void
fittsmenu_set_animation         (Fittsmenu *fittsmenu, gint value)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  priv->animation = value;
}

gint
fittsmenu_get_animation         (Fittsmenu *fittsmenu)
{
  return FITTSMENU_GET_PRIVATE (fittsmenu)->animation;  
}

void
fittsmenu_set_menu_radius       (Fittsmenu *fittsmenu, gint value)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
	gint x, y;
  GdkDisplay *display;
  GdkScreen  *screen;
  
  x = priv->window_x + priv->menu_radius;
  y = priv->window_y + priv->menu_radius;
  
  priv->menu_radius = value;
	priv->window_size = value * 2;
	
  priv->window_x = x - priv->menu_radius;
  priv->window_y = y - priv->menu_radius;

  gtk_window_resize(GTK_WINDOW(fittsmenu->toplevel), priv->window_size, priv->window_size);
  gtk_window_move(GTK_WINDOW(fittsmenu->toplevel), priv->window_x, priv->window_y);
}

gint
fittsmenu_get_menu_radius       (Fittsmenu *fittsmenu)
{
  return FITTSMENU_GET_PRIVATE (fittsmenu)->menu_radius;  
}

void
fittsmenu_set_menu_inner_radius (Fittsmenu *fittsmenu, gint value)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  priv->menu_inner_radius = value;
}

gint
fittsmenu_get_menu_inner_radius (Fittsmenu *fittsmenu)
{
  return FITTSMENU_GET_PRIVATE (fittsmenu)->menu_inner_radius;  
}

fittsmenu_slice*
fittsmenu_get_active (Fittsmenu *fittsmenu)
{
  return FITTSMENU_GET_PRIVATE (fittsmenu)->active;
}

void
fittsmenu_set_active (Fittsmenu *fittsmenu,
                      guint      index,
                      fittsmenu_slice *slice)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  if (index)
  	priv->active = g_list_nth_data(priv->slices, index);
	if (slice)
		priv->active = slice;
}

fittsmenu_slice* 
fittsmenu_slice_new (const char* label, const char* icon) {
	fittsmenu_slice *slice;
	slice = g_slice_new(fittsmenu_slice);
	slice->icon = g_strdup(icon);
	slice->label = g_strdup(label);
  return slice;
}

void
fittsmenu_slice_free (fittsmenu_slice *slice) {
	g_free(slice->icon);
  g_free(slice->label);
  cairo_surface_destroy(slice->icon_buffer);
  rsvg_handle_free(slice->icon_handle); 
  g_slice_free(fittsmenu_slice, slice);
}

static void 
fittsmenu_dispose (GObject *obj) {
	Fittsmenu *fittsmenu = FITTSMENU (obj);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  if (priv->dispose_has_run)
  	return;
  
  priv->active = NULL;
  
  fittsmenu_slice *slice;
  GList *list;
  for (list = priv->slices;list;list = list->next) {
  	slice = (fittsmenu_slice *)list->data;
  	fittsmenu_slice_free(slice);
  }  
  g_list_free(list);
  
  priv->dispose_has_run = TRUE;
  
  // Causes lots of problems? Tries to dispose of things already disposed of
  G_OBJECT_CLASS (fittsmenu_parent_class)->dispose (obj);
}

static void 
fittsmenu_finalize (GObject *obj) {
	Fittsmenu *fittsmenu = FITTSMENU (obj);
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  
  //g_free(priv);
  
  G_OBJECT_CLASS (fittsmenu_parent_class)->finalize (obj);
}

/* Widget utility functions not exposed */
/* Set window transparency */
static void
set_alpha(GtkWidget *widget)
{
  GdkScreen* screen = gtk_widget_get_screen (widget);
  GdkColormap* colormap = gdk_screen_get_rgba_colormap (screen);

  if (!colormap)
    colormap = gdk_screen_get_rgb_colormap (screen);

  gtk_widget_set_colormap (widget, colormap);
}

/* Glitzy cairo create */
static cairo_t *
my_cairo_create (GdkWindow* window, Fittsmenu *fittsmenu) 
{
#ifdef USE_GLITZ
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  if (priv->nv_use_glitz && priv->glitz_drawable) {
    glitz_surface_t* glitz_surface;
    cairo_surface_t* cairo_surface;
    cairo_t* cr;

    glitz_surface = glitz_surface_create (priv->glitz_drawable,
                                          priv->glitz_format,
                                          priv->window_size,
                                          priv->window_size,
                                          0,
                                          NULL);

    if (priv->glitz_drawable_format->doublebuffer)
      glitz_surface_attach (glitz_surface,
                            priv->glitz_drawable,
                            GLITZ_DRAWABLE_BUFFER_BACK_COLOR);
    else
      glitz_surface_attach (glitz_surface,
                            priv->glitz_drawable,
                            GLITZ_DRAWABLE_BUFFER_FRONT_COLOR);

    cairo_surface = cairo_glitz_surface_create (glitz_surface);
    cr = cairo_create (cairo_surface);

    cairo_surface_destroy (cairo_surface);
    glitz_surface_destroy (glitz_surface);

    return cr;
  }
#endif //USE_GLITZ
    return gdk_cairo_create (window);
}

/* set rendering-"fidelity" and clear canvas */
static void
canvas_reset (cairo_t* cr)
{
  cairo_set_tolerance (cr, 0.1);
  cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
}

/* Double buffer glitz surface */
// FIXME Need to pass the widget
#ifdef USE_GLITZ
static void
swap_buffers(Fittsmenu *fittsmenu)
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  if (priv->glitz_drawable_format && priv->glitz_drawable_format->doublebuffer)
    glitz_drawable_swap_buffers (priv->glitz_drawable);
}
#endif //USE_GLITZ


static void
render(cairo_t* cr, Fittsmenu *fittsmenu) 
{
  FittsmenuPrivate *priv = FITTSMENU_GET_PRIVATE (fittsmenu);
  gint no_of_slices = g_list_length(priv->slices);
  
  cairo_matrix_t matrix;
  gdouble arc_start, arc_end, arc_radius, arc_scale;
  gint arc_width, arc_sdeg, arc_edeg;
  gint cx, cy, i;
  fittsmenu_slice *slice;
  fittsmenu_slice *hover;
  gdouble px, py, ex, ey;
  gdouble icon_cangle, icon_cdist;
  gdouble icon_scale, this_icon_scale;
  gdouble icon_cx, icon_cy;
  gint icon_offset_x, icon_offset_y;
  gint icon_width, icon_height;
  cairo_surface_t *cr_surface;
  cairo_t *cr_buf;
  gdouble mouse_angle, icon_angle, distance;
  RsvgDimensionData icon_size = { 0, 0, 0.0, 0.0 };
  GtkIconInfo *icon_info; 
  gchar *icon_cmp;
  GError *iconerror = NULL;
  
  // Calculate the arc of a slice in radians and its width
  arc_radius = (2*G_PI) / no_of_slices;
  arc_scale = arc_radius / ((2*G_PI) / 13);
  arc_width = priv->menu_radius - priv->menu_inner_radius;
  
  arc_start = priv->menu_angle * (G_PI / 180.0f);
  arc_end = arc_start + arc_radius;
  
  // Set the centre co-ordinates 
  cx = priv->menu_radius;
  cy = priv->menu_radius;
  
  cairo_save(cr);
  
  // Draw the centre of the menu
  cairo_arc (cr, cx, cy, priv->menu_inner_radius- 10, 0., 2*G_PI);
  cairo_set_source_rgba(cr, 0, 0, 0, .65);
  cairo_fill(cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);      
  
  hover = NULL;
  for (i = 0;i < no_of_slices;i++) {
    slice = g_list_nth_data(priv->slices, i);

    // Calculate and draw the slice
    polrec(priv->menu_radius - arc_width,  arc_end + (G_PI/2), &py, &px);
    ex = px+cx;
    ey = (py * -1)+cy;// Icon width should always be 48 for svgs now
    
    cairo_arc(cr, cx, cy, priv->menu_radius - 3, arc_start, arc_end);
    cairo_line_to(cr, ex, ey);
    
    polrec(priv->menu_radius - 3, arc_start + (G_PI/2), &py, &px);
    ex = px+cx;
    ey = (py * -1)+cy;
    
    cairo_arc_negative(cr, cx, cy, priv->menu_radius - arc_width - 10, arc_end, arc_start); 
    cairo_line_to(cr, ex, ey);
    
    // Limit mouse angles to 360 degrees
    priv->mouse_angle = (int)priv->mouse_angle % 360;

    // Convert angles of the segment arcs to degrees 
    arc_sdeg = (arc_start * (180/G_PI))+90;
    arc_edeg = (arc_end * (180/G_PI))+90;
        
    // Limit arc degrees to 360 
    arc_sdeg = arc_sdeg % 360;
    arc_edeg = arc_edeg % 360;
        
    // Special cases for passing through 0 degrees
    if ((priv->mouse_angle < arc_sdeg) && (arc_sdeg > arc_edeg))
      arc_sdeg = arc_sdeg - 360;
    else if (arc_sdeg > arc_edeg)
      arc_edeg = arc_edeg + 360;

    // Fill and stroke each segment 
    if ((priv->mouse_angle > arc_sdeg) && (priv->mouse_angle < arc_edeg) 
        && (priv->menu_over == TRUE)) {
      cairo_set_source_rgba(cr, .3, .3, .3, .5);
      hover = slice;
    } else {
      cairo_set_source_rgba(cr, 0, 0, 0, .7);
    }
    
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, .0, .0, .0, .1);
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);
    
    icon_cmp = g_strdup(slice->icon);
    icon_cmp = g_ascii_strdown(icon_cmp, strlen(icon_cmp));
    icon_cmp = g_strreverse(icon_cmp);
    
    if (!g_ascii_strncasecmp("gvs", icon_cmp, 3)) {
      // Repeatedly complains the error isn't null? go figure!
      slice->icon_handle = rsvg_handle_new_from_file (slice->icon, &iconerror);
      if (iconerror != NULL) {
        g_printerr("RSVG: %s %s\n", iconerror->message, slice->icon);
        g_clear_error(&iconerror);
          
    		// Load the default missing icon instead
    		icon_info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (),
        	                                      "image-missing", 1000, 
         	                                      GTK_ICON_LOOKUP_FORCE_SVG);
          
        slice->icon = g_strdup(gtk_icon_info_get_filename (icon_info));
        slice->icon_handle = rsvg_handle_new_from_file (slice->icon, &iconerror);
          
        if (iconerror != NULL) {
        	g_printerr("RSVG: %s\n", iconerror->message);
          	
         	if (iconerror != NULL)
         		g_error_free(iconerror);
          	
         	return;
        }
      }
      
     	if (iconerror != NULL)
        g_error_free(iconerror);	
      else
        rsvg_handle_get_dimensions(slice->icon_handle, &icon_size);
      
      // Icon width/height should always be 48 for svgs now, specified on surface load
      icon_width = icon_size.width;
      icon_height = icon_size.height;
    } else {
      // We need to get image sizes for png files still lets assume
      // 48x48 for now
      icon_width = 48;
      icon_height = 48;
    }
    
    // Calculate the size/position of the current icon 
    if (icon_size.width >= icon_size.height)
    	icon_scale = (gdouble)32 / (gdouble)icon_size.width;
    else
    	icon_scale = (gdouble)32 / (gdouble)icon_size.height;
    
    icon_cangle = arc_start + (arc_radius / 2);
    
    if ((priv->menu_over) && (priv->animation == FITTSMENU_ANIM_ISCALE)) {
      /* Scaled icons */
      icon_angle  = icon_cangle * (180/G_PI);
      icon_angle  = ((int)icon_angle+90) % 360;
      mouse_angle = priv->mouse_angle;
      distance    = (icon_angle - mouse_angle);
      distance    = distance*distance;
      distance    = sqrt(distance);
      
      if (distance > 180)
        distance  = (distance * -1)+360;
      
      //g_print("%d %d %s\n", (int)mouse_angle, (int)icon_angle, slice->label);
      //g_print("Distance to %s = %d\n", slice->label, (int)distance);
      //this_icon_scale = cosf( distance * (G_PI / 180.0f ));
      //g_print("Icon scale %s %f\n", slice->label,this_icon_scale);
      //this_icon_scale = icon_scale * arc_scale * this_icon_scale;      
    } else {
      this_icon_scale = 0;
    }
    
    icon_scale = icon_scale * arc_scale;
    
    icon_width  = icon_width * icon_scale;
    icon_height = icon_height * icon_scale;
    
    icon_offset_x = icon_width / 2;
    icon_offset_y = icon_height / 2;
    
    icon_cdist = priv->menu_radius - (sqrt(icon_width*icon_width + icon_height*icon_height)/2) - 4;

    polrec(icon_cdist, icon_cangle, &icon_cx, &icon_cy);
    
    
    // Render the current icon 
    cairo_save (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    
    cairo_matrix_init_scale     (&matrix, icon_scale, icon_scale);
    cairo_transform(cr, &matrix);
    
    cairo_matrix_init_translate (&matrix, (icon_cx + cx - icon_offset_x)/icon_scale, 
                                 (icon_cy + cy - icon_offset_y)/icon_scale);
    cairo_transform (cr, &matrix);

    if (slice->icon_buffer) {
      cairo_set_source_surface (cr, slice->icon_buffer, 0.0, 0.0);
      cairo_paint (cr);
    } else {
      /* Fill buffer on first render */
      // Crypticly svg :)
      if (!g_ascii_strncasecmp("gvs", icon_cmp, 3)) {
        /* All of this code looks severely outdated, mostly because cairo has good svg support now :)*/
        cr_surface = cairo_surface_create_similar (cairo_get_target (cr), 
                                                   CAIRO_CONTENT_COLOR_ALPHA,
                                                   priv->window_size, 
                                                   priv->window_size);
        cr_buf = cairo_create(cr_surface);
        rsvg_handle_render_cairo (slice->icon_handle, cr_buf);
        cairo_destroy(cr_buf);
        //*/
        //cr_surface = cairo_svg_surface_create(slice->icon, 48, 48);
      } else {
        cr_surface = cairo_image_surface_create_from_png(slice->icon);
      }
      slice->icon_buffer = cr_surface;
      cairo_set_source_surface (cr, slice->icon_buffer, 0.0, 0.0);
      cairo_paint (cr);   
      //rsvg_handle_render_cairo (slice->icon_handle, cr);
    }
		priv->hover = hover;
    cairo_restore (cr);
    
    arc_start = arc_start + arc_radius;
    arc_end = arc_start + arc_radius;
  }
    
  cairo_restore(cr);
}

/* Cartesian co-ordinate conversion*/
static
void recpol(gdouble x, gdouble y, gdouble *pr, gdouble *pa)
{
  gdouble a;

  if (x == 0.0 && y == 0.0) { /* special case, both components 0 */
    *pa = 0.0;
    *pr = 0.0;
    return;
  }

  a = atan2(y,x);             /* angle +- pi/2 */
  if (a < 0.0)
    a = a + (2 * M_PI);       /* want it to be 0 to 2 pi   */
  *pa = a;
/**pr = hypot(x,y);*/         /* radius.  HP bug in hypot. */
  *pr = sqrt(x*x + y*y);      /* radius */
  return;
}

static 
void polrec(gdouble r, gdouble a, gdouble *px, gdouble *py)
{
  *px = r*cos(a);
  *py = r*sin(a);
  return;
}

long
get_time (void)
{
    static GTimeVal val;
    g_get_current_time (&val);

    return (val.tv_sec * G_USEC_PER_SEC) + val.tv_usec;
}
