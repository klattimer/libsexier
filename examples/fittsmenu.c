#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "fittsmenu.h"

void
menuclicked                            (GtkWidget       *widget,
                                        gpointer         user_data)
{
  fittsmenu_slice *slice;
  slice = fittsmenu_get_active(FITTSMENU(widget));
  if (slice)
    g_print("clicked-signal caught by app on %s\n", slice->label);
    
  gtk_widget_destroy(GTK_WIDGET(widget));
}

void
on_button1_released                    (GtkButton *button,
                                        gpointer *userdata)
{
  Fittsmenu *fittsmenu;
  fittsmenu = fittsmenu_new();
  g_signal_connect ((gpointer) fittsmenu, "clicked-signal",
                    G_CALLBACK (menuclicked),
                    NULL);
  
  //fittsmenu_set_animation(fittsmenu, FITTSMENU_ANIM_ISCALE);
  fittsmenu_set_menu_inner_radius(fittsmenu, 80);
  
  // Add Menu items
  fittsmenu_append(fittsmenu, 
                   fittsmenu_slice_new("Selection Tool", EXAMPLES_DATA_PATH "icon_cursor.svg"));
  
  fittsmenu_append(fittsmenu, 
                   fittsmenu_slice_new("Edit Nodes", EXAMPLES_DATA_PATH "icon_nodes.svg"));
    
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Rectangle", EXAMPLES_DATA_PATH "icon_rectangle.svg"));
    
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Ellipse", EXAMPLES_DATA_PATH "icon_ellipse.svg"));
 
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Polygon", EXAMPLES_DATA_PATH "icon_star.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Spiral", EXAMPLES_DATA_PATH "icon_spiral.svg"));
 
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Draw Freehand Lines", EXAMPLES_DATA_PATH "icon_freehand.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Draw Bezier Curves", EXAMPLES_DATA_PATH "icon_curves.svg"));
    
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Draw Caligraphic Lines", EXAMPLES_DATA_PATH "icon_calig.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Text", EXAMPLES_DATA_PATH "icon_text.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Edit Gradient", EXAMPLES_DATA_PATH "icon_gradient.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Pick Colours", EXAMPLES_DATA_PATH "icon_droplet.svg"));
                   
                   /*
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Draw Bezier Curves", EXAMPLES_DATA_PATH "icon_curves.svg"));
    
  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Draw Caligraphic Lines", EXAMPLES_DATA_PATH "icon_calig.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Create Text", EXAMPLES_DATA_PATH "icon_text.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Edit Gradient", EXAMPLES_DATA_PATH "icon_gradient.svg"));

  fittsmenu_append(fittsmenu,
                   fittsmenu_slice_new("Pick Colours", EXAMPLES_DATA_PATH "icon_droplet.svg"));

/*  */ 
  gtk_widget_show(GTK_WIDGET(fittsmenu));
}

/* Everything else is just boiler plate */

void
on_button2_released                    (GtkButton       *button,
                                        gpointer *userdata)
{
  gtk_main_quit();
}
  
GtkWidget*
create_window1 (void)
{
  GtkWidget *window1;
  GtkWidget *vbox1;
  GtkWidget *label1;
  GtkWidget *button1;
  GtkWidget *button2;

  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window1), "Fittsmenu Example");

  vbox1 = gtk_vbox_new (FALSE, 5);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window1), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 12);

  label1 = gtk_label_new ("Click open to test Fittsmenu");
  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
  gtk_label_set_line_wrap (GTK_LABEL (label1), TRUE);

  button1 = gtk_button_new_from_stock ("gtk-open");
  gtk_widget_show (button1);
  gtk_box_pack_start (GTK_BOX (vbox1), button1, TRUE, TRUE, 0);

  g_signal_connect ((gpointer) button1, "released",
                    G_CALLBACK (on_button1_released),
                    NULL);
                    
  button2 = gtk_button_new_from_stock ("gtk-quit");
  gtk_widget_show (button2);
  gtk_box_pack_start (GTK_BOX (vbox1), button2, TRUE, TRUE, 0);

  g_signal_connect ((gpointer) button2, "released",
                    G_CALLBACK (on_button2_released),
                    NULL);

  return window1;
}


int
main (int argc, char *argv[])
{
  GtkWidget *window1;

  gtk_init (&argc, &argv);
  
  window1 = create_window1 ();
  gtk_widget_show (window1);

  gtk_main ();
  return 0;
}

