/* -*- mode: c; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * gcal-day-view.c
 *
 * Copyright (C) 2013 - Erick Pérez Castellanos
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "gcal-day-view.h"
#include "gcal-all-day-grid.h"
#include "gcal-days-grid.h"
#include "gcal-viewport.h"
#include "gcal-utils.h"
#include "gcal-view.h"
#include "gcal-event-widget.h"

#include <glib/gi18n.h>

#include <libecal/libecal.h>

enum
{
  PROP_0,
  PROP_DATE
};

typedef struct
{
  /* property */
  icaltimetype   *date;

  GtkWidget      *viewport;
  GtkWidget      *fold_button;

  /* events widgets parents */
  GtkWidget      *all_day_grid;
  GtkWidget      *day_grid;
} GcalDayViewPrivate;

static void           viewport_shown                      (GtkWidget      *widget,
                                                           gpointer        user_data);

static void           gcal_view_interface_init            (GcalViewIface  *iface);

static void           gcal_day_view_constructed           (GObject        *object);

static void           gcal_day_view_finalize              (GObject        *object);

static void           gcal_day_view_set_property          (GObject        *object,
                                                           guint           property_id,
                                                           const GValue   *value,
                                                           GParamSpec     *pspec);

static void           gcal_day_view_get_property          (GObject        *object,
                                                           guint           property_id,
                                                           GValue         *value,
                                                           GParamSpec     *pspec);

static void           gcal_day_view_add                   (GtkContainer   *container,
                                                           GtkWidget      *widget);

static void           gcal_day_view_remove                (GtkContainer   *container,
                                                           GtkWidget      *widget);

static icaltimetype*  gcal_day_view_get_initial_date      (GcalView       *view);

static icaltimetype*  gcal_day_view_get_final_date        (GcalView       *view);

static void           gcal_day_view_mark_current_unit     (GcalView       *view,
                                                           gint           *x,
                                                           gint           *y);

static void           gcal_day_view_clear_marks           (GcalView       *view);

static gchar*         gcal_day_view_get_left_header       (GcalView       *view);

static gchar*         gcal_day_view_get_right_header      (GcalView       *view);

static gboolean       gcal_day_view_draw_event            (GcalView       *view,
                                                           icaltimetype   *start_date,
                                                           icaltimetype   *end_date);

static GtkWidget*     gcal_day_view_get_by_uuid           (GcalView       *view,
                                                           const gchar    *uuid);

static void           gcal_day_view_clear                 (GcalView       *view);

G_DEFINE_TYPE_WITH_CODE (GcalDayView,
                         gcal_day_view,
                         GTK_TYPE_GRID,
                         G_ADD_PRIVATE (GcalDayView)
                         G_IMPLEMENT_INTERFACE (GCAL_TYPE_VIEW,
                                                gcal_view_interface_init));

static void
viewport_shown (GtkWidget *widget,
                gpointer   user_data)
{
  GcalDayViewPrivate *priv;
  icaltimetype date;
  gdouble value;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (user_data));

  if (priv->date == NULL)
    return;

  date = icaltime_current_time_with_zone (priv->date->zone);

  /* 0.01 is spacing to let the mark show up */
  value = (((gdouble) date.hour) / 24.0) - 0.01;

  g_debug ("Getting into here, with value: %f", value);
  gcal_viewport_scroll_to (GCAL_VIEWPORT (priv->viewport), value);
}

static void
gcal_day_view_class_init (GcalDayViewClass *klass)
{
  GObjectClass *object_class;
  GtkContainerClass *container_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->constructed = gcal_day_view_constructed;
  object_class->finalize = gcal_day_view_finalize;
  object_class->set_property = gcal_day_view_set_property;
  object_class->get_property = gcal_day_view_get_property;

  container_class = GTK_CONTAINER_CLASS (klass);
  container_class->add = gcal_day_view_add;
  container_class->remove = gcal_day_view_remove;
  /* FIXME: Uncomment stuff here */
  /* gtk_container_class_handle_border_width (container_class); */

  g_object_class_override_property (object_class, PROP_DATE, "active-date");
}

static void
gcal_day_view_init (GcalDayView *self)
{
}

static void
gcal_view_interface_init (GcalViewIface *iface)
{
  /* New API */
  iface->get_initial_date = gcal_day_view_get_initial_date;
  iface->get_final_date = gcal_day_view_get_final_date;

  iface->mark_current_unit = gcal_day_view_mark_current_unit;
  iface->clear_marks = gcal_day_view_clear_marks;

  iface->get_left_header = gcal_day_view_get_left_header;
  iface->get_right_header = gcal_day_view_get_right_header;

  iface->draw_event = gcal_day_view_draw_event;
  iface->get_by_uuid = gcal_day_view_get_by_uuid;
  iface->clear = gcal_day_view_clear;
}

static void
gcal_day_view_constructed (GObject *object)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (object));

  if (G_OBJECT_CLASS (gcal_day_view_parent_class)->constructed != NULL)
      G_OBJECT_CLASS (gcal_day_view_parent_class)->constructed (object);

  priv->fold_button = gtk_button_new_with_label (_("Fold"));
  g_object_set (priv->fold_button,
                "valign", GTK_ALIGN_END,
                "margin", 6,
                NULL);
  gtk_style_context_add_class (
      gtk_widget_get_style_context (priv->fold_button),
      "nav-button");
  gtk_grid_attach (GTK_GRID (object), priv->fold_button, 0, 0, 1, 1);

  /* event widget holders */
  priv->all_day_grid = gcal_all_day_grid_new (2);
  gcal_all_day_grid_set_column_headers (GCAL_ALL_DAY_GRID (priv->all_day_grid),
                                        _("Today"),
                                        _("Tomorrow"));
  gtk_widget_set_size_request (priv->all_day_grid, -1, 64);
  gtk_widget_set_hexpand (priv->all_day_grid, TRUE);

  priv->day_grid = gcal_days_grid_new (2);
  gcal_days_grid_set_preferred_cell_height (GCAL_DAYS_GRID (priv->day_grid), 60);

  gtk_widget_set_hexpand (priv->day_grid, TRUE);
  gtk_widget_set_vexpand (priv->day_grid, TRUE);

  gtk_grid_attach (GTK_GRID (object), priv->all_day_grid, 1, 0, 1, 1);

  priv->viewport = gcal_viewport_new ();
  gcal_viewport_add (GCAL_VIEWPORT (priv->viewport), priv->day_grid);
  gtk_grid_attach (GTK_GRID (object), priv->viewport, 0, 1, 2, 1);

  /* binding sizes */
  gtk_widget_set_size_request (
      priv->fold_button,
      gcal_days_grid_get_scale_width (GCAL_DAYS_GRID (priv->day_grid)) - 12,
      -1);

  gtk_widget_show_all (GTK_WIDGET (object));

  /* signal handling */
  g_signal_connect (object, "map", G_CALLBACK (viewport_shown), object);
}

static void
gcal_day_view_finalize (GObject       *object)
{
  GcalDayViewPrivate *priv;
  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (object));

  if (priv->date != NULL)
    g_free (priv->date);

  /* Chain up to parent's finalize() method. */
  G_OBJECT_CLASS (gcal_day_view_parent_class)->finalize (object);
}

static void
gcal_day_view_set_property (GObject       *object,
                            guint          property_id,
                            const GValue  *value,
                            GParamSpec    *pspec)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (object));

  switch (property_id)
    {
    case PROP_DATE:
      if (priv->date != NULL)
        g_free (priv->date);

      priv->date = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gcal_day_view_get_property (GObject       *object,
                            guint          property_id,
                            GValue        *value,
                            GParamSpec    *pspec)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (object));

  switch (property_id)
    {
    case PROP_DATE:
      g_value_set_boxed (value, priv->date);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/* GtkContainer API */
static void
gcal_day_view_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  GcalDayViewPrivate *priv;
  icaltimetype *tomorrow;
  icaltimetype *dt_start;
  icaltimetype *dt_end;
  gchar* summ;

  g_return_if_fail (GCAL_IS_EVENT_WIDGET (widget));
  g_return_if_fail (gtk_widget_get_parent (widget) == NULL);
  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (container));

  summ = gcal_event_widget_get_summary (GCAL_EVENT_WIDGET (widget));

  tomorrow = gcal_day_view_get_final_date (GCAL_VIEW (container));
  dt_start = gcal_event_widget_get_date (GCAL_EVENT_WIDGET (widget));
  dt_end = gcal_event_widget_get_end_date (GCAL_EVENT_WIDGET (widget));

  /* FIXME: recheck conditions to add, what, where */
  /* Maybe, should be good to add, inside days views only contained events */
  g_debug ("[processing] %s from %s to %s",
           summ,
           icaltime_as_ical_string (*dt_start),
           icaltime_as_ical_string (*dt_end));

  if (! gcal_event_widget_get_all_day (GCAL_EVENT_WIDGET (widget)) &&
      (dt_start->day == dt_end->day) &&
      (dt_start->day == priv->date->day ||
       dt_start->day == tomorrow->day))
    {
      g_debug ("[days-grid] %s from %s to %s",
               summ,
               icaltime_as_ical_string (*dt_start),
               icaltime_as_ical_string (*dt_end));

      gcal_days_grid_place (GCAL_DAYS_GRID (priv->day_grid),
                            widget,
                            priv->date->day == dt_start->day ? 0 : 1,
                            dt_start->hour * 2 + (dt_start->minute / 30),
                            dt_end->hour * 2 + (dt_end->minute / 30)); /* FIXME Include half hour and length */
    }
  else
    {
      gboolean start_tomorrow;
      guint length;
      g_debug ("[all-day-grid] %s from %s to %s",
               summ,
               icaltime_as_ical_string (*dt_start),
               icaltime_as_ical_string (*dt_end));
      start_tomorrow = icaltime_compare_date_only (*dt_start, *tomorrow) == 0;

      if (start_tomorrow)
        length = 1;
      else if (dt_start->day != dt_end->day)
        length = 2;
      else
        length = 1;
      gcal_all_day_grid_place (GCAL_ALL_DAY_GRID (priv->all_day_grid),
                               widget,
                               start_tomorrow ? 1 : 0, length);
    }

  g_free (tomorrow);
  g_free (dt_start);
  g_free (dt_end);
  g_free (summ);
}

static void
gcal_day_view_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (container));

  if (gtk_widget_get_parent (widget) == (GtkWidget*) container)
    {
      GTK_CONTAINER_CLASS (gcal_day_view_parent_class)->remove (container,
                                                                widget);
    }
  else
    {
      gtk_container_remove (GTK_CONTAINER (priv->all_day_grid), widget);
      gtk_container_remove (GTK_CONTAINER (priv->day_grid), widget);
    }
}

/* GcalView API */
static icaltimetype*
gcal_day_view_get_initial_date (GcalView *view)
{
  GcalDayViewPrivate *priv;
  icaltimetype *new_date;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));
  new_date = g_new0 (icaltimetype, 1);
  *new_date = *(priv->date);
  new_date->hour = 0;
  new_date->minute = 0;
  new_date->second = 0;

  return new_date;
}

/**
 * gcal_day_view_get_final_date:
 * @view: a #GcalDayView
 *
 * Returns the date of "tomorrow" at 23:59
 *
 * Returns:
 **/
static icaltimetype*
gcal_day_view_get_final_date (GcalView *view)
{
  GcalDayViewPrivate *priv;
  icaltimetype *new_date;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));
  new_date = g_new0 (icaltimetype, 1);
  *new_date = *(priv->date);

  icaltime_adjust (new_date, 1, 0, 0, 0);

  new_date->hour = 23;
  new_date->minute = 59;
  new_date->second = 59;

  return new_date;
}

static void
gcal_day_view_mark_current_unit (GcalView *view,
                                 gint     *x,
                                 gint     *y)
{
  GcalDayViewPrivate *priv;

  guint cell;
  gint orig_x, orig_y;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));

  cell = priv->date->hour * 2 + (priv->date->minute / 30);
  gcal_days_grid_mark_cell (GCAL_DAYS_GRID (priv->day_grid), cell);
  gcal_days_grid_get_cell_position (GCAL_DAYS_GRID (priv->day_grid),
                                    cell, &orig_x, &orig_y);
  g_debug ("Click Position: %d, %d", orig_x, orig_y);

  gtk_widget_translate_coordinates (priv->day_grid,
                                    GTK_WIDGET (view),
                                    orig_x, orig_y,
                                    x, y);
  g_debug ("Translated Click Position: %d, %d", *x, *y);
}

static void
gcal_day_view_clear_marks (GcalView *view)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));
  gcal_days_grid_clear_marks (GCAL_DAYS_GRID (priv->day_grid));
}

static gchar*
gcal_day_view_get_left_header (GcalView *view)
{
  GcalDayViewPrivate *priv;

  gchar str_date[64];

  struct tm tm_date;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));

  tm_date = icaltimetype_to_tm (priv->date);
  e_utf8_strftime_fix_am_pm (str_date, 64, "%A %B", &tm_date);

  return g_strdup_printf ("%s, %d", str_date, priv->date->day);
}

static gchar*
gcal_day_view_get_right_header (GcalView *view)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));

  return g_strdup_printf ("%d", priv->date->year);
}

static gboolean
gcal_day_view_draw_event (GcalView     *view,
                          icaltimetype *start_date,
                          icaltimetype *end_date)
{
  GcalDayViewPrivate *priv;

  icaltimetype *first_day;
  icaltimetype *last_day;
  gint left_boundary;
  gint right_boundary;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));
  first_day = gcal_day_view_get_initial_date (view);
  last_day = gcal_day_view_get_final_date (view);

  if (priv->date == NULL)
    return FALSE;

  /* XXX: Check for date_only comparison since might drop timezone info */
  left_boundary = icaltime_compare_date_only (*end_date, *first_day);
  right_boundary = icaltime_compare_date_only (*last_day, *start_date);

  if (left_boundary == -1 || right_boundary == -1)
      return FALSE;

  return TRUE;
}

static GtkWidget*
gcal_day_view_get_by_uuid (GcalView    *view,
                           const gchar *uuid)
{
  GcalDayViewPrivate *priv;
  GtkWidget *widget;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));

  widget =
    gcal_all_day_grid_get_by_uuid (GCAL_ALL_DAY_GRID (priv->all_day_grid),
                                   uuid);
  if (widget != NULL)
    return widget;

  widget =
    gcal_days_grid_get_by_uuid (GCAL_DAYS_GRID (priv->day_grid),
                                uuid);
  if (widget != NULL)
    return widget;

  return NULL;
}

static void
gcal_day_view_clear (GcalView *view)
{
  GcalDayViewPrivate *priv;

  priv = gcal_day_view_get_instance_private (GCAL_DAY_VIEW (view));

  gtk_container_foreach (GTK_CONTAINER (priv->all_day_grid),
                         (GtkCallback) gtk_widget_destroy, NULL);
  gtk_container_foreach (GTK_CONTAINER (priv->day_grid),
                         (GtkCallback) gtk_widget_destroy, NULL);
}

/* Public API */
/**
 * gcal_day_view_new:
 * @date:
 *
 * Since: 0.1
 * Return value: A new #GcalDayView
 * Returns: (transfer full):
 **/
GtkWidget*
gcal_day_view_new (void)
{
  return g_object_new (GCAL_TYPE_DAY_VIEW, NULL);
}
