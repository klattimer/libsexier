#ifndef __FITTSMENU_H__
#define __FITTSMENU_H__

#include <glib.h>
#include <cairo.h>
#include <librsvg/rsvg.h>

typedef struct _fittsmenu_slice fittsmenu_slice;

struct _fittsmenu_slice 
{
  gchar *label;
  gchar *icon;
  gint button;
  RsvgHandle *icon_handle;
  cairo_surface_t *icon_buffer;
  guint index;
};

enum
{
	FITTSMENU_ANIM_NONE,
	FITTSMENU_ANIM_CROTATE,
	FITTSMENU_ANIM_ISCALE,
	FITTSMENU_ANIM_PULSE
};

G_BEGIN_DECLS

#define FITTSMENU_TYPE             (fittsmenu_get_type ())
#define FITTSMENU(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), FITTSMENU_TYPE, Fittsmenu))
#define FITTSMENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  FITTSMENU_TYPE, FittmenuClass))
#define IS_FITTSMENU(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FITTSMENU_TYPE))
#define IS_FITTSMENU_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  FITTSMENU_TYPE))

typedef struct _Fittsmenu       Fittsmenu;
typedef struct _FittsmenuClass  FittsmenuClass;

struct _Fittsmenu
{
  GtkWidget widget;
  
  GtkWidget *toplevel;
};

struct _FittsmenuClass
{
  GtkWidgetClass parent_class;
  
  void (* ticked)   (Fittsmenu * fittsmenu);
  void (* revolved) (Fittsmenu * fittsmenu);
  void (* clicked)  (Fittsmenu * fittsmenu);
};

GType      fittsmenu_get_type   (void) G_GNUC_CONST;
Fittsmenu* fittsmenu_new        (void);

/* Getters and setters */
void       fittsmenu_set_animation         (Fittsmenu *fittsmenu, gint value);
gint       fittsmenu_get_animation         (Fittsmenu *fittsmenu);
void       fittsmenu_set_menu_radius       (Fittsmenu *fittsmenu, gint value);
gint       fittsmenu_get_menu_radius       (Fittsmenu *fittsmenu);
void       fittsmenu_set_menu_inner_radius (Fittsmenu *fittsmenu, gint value);
gint       fittsmenu_get_menu_inner_radius (Fittsmenu *fittsmenu);

void       fittsmenu_append     (Fittsmenu *fittsmenu, fittsmenu_slice *slice);
void       fittsmenu_remove     (Fittsmenu *fittsmenu, fittsmenu_slice *slice);
void       fittsmenu_popup      (Fittsmenu *fittsmenu, guint button);
void       fittsmenu_popdown    (Fittsmenu *fittsmenu);
fittsmenu_slice*  fittsmenu_get_active (Fittsmenu *fittsmenu);
void       fittsmenu_set_active (Fittsmenu *fittsmenu, guint index, fittsmenu_slice *slice);
fittsmenu_slice*  fittsmenu_slice_new (const char*icon, const char* label);
void			 fittsmenu_slice_free (fittsmenu_slice *slice);

G_END_DECLS

#endif /* __FITTSMENU_H__ */
