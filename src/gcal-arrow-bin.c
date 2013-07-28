/* -*- mode: c; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * gcal-arrow-bin.c
 * Copyright (C) 2012 Erick Pérez Castellanos <erickpc@gnome.org>
 *
 * gnome-calendar is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-calendar is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gcal-arrow-bin.h"

struct _GPoint
{
  gdouble x;
  gdouble y;
};

typedef struct _GPoint GPoint;

typedef struct
{
  GtkPositionType arrow_position;
  gdouble         arrow_align;
} GcalArrowBinPrivate;

enum
{
  PROP_0,
  PROP_ARROW_POSITION,
  PROP_ARROW_ALIGN
};

static void     get_padding_and_border                        (GtkWidget       *widget,
                                                               GtkBorder       *border,
                                                               guint           *arrow_size);

static void     draw_arrow                                    (GtkWidget       *widget,
                                                               cairo_t         *cr);

static void     gcal_arrow_bin_set_property                   (GObject         *object,
                                                               guint            property_id,
                                                               const GValue    *value,
                                                               GParamSpec      *pspec);

static void     gcal_arrow_bin_get_property                   (GObject         *object,
                                                               guint            property_id,
                                                               GValue          *value,
                                                               GParamSpec      *pspec);

static void     gcal_arrow_bin_get_preferred_width            (GtkWidget       *widget,
                                                               gint            *minimum_width,
                                                               gint            *natural_width);

static void     gcal_arrow_bin_get_preferred_height_for_width (GtkWidget       *widget,
                                                               gint             width,
                                                               gint            *minimum_height,
                                                               gint            *natural_height);

static void     gcal_arrow_bin_get_preferred_height           (GtkWidget       *widget,
                                                               gint            *minimum_height,
                                                               gint            *natural_height);

static void     gcal_arrow_bin_get_preferred_width_for_height (GtkWidget       *widget,
                                                               gint             height,
                                                               gint            *minimum_width,
                                                               gint            *natural_width);

static void     gcal_arrow_bin_size_allocate                  (GtkWidget       *widget,
                                                               GtkAllocation   *allocation);

static gboolean gcal_arrow_bin_draw                           (GtkWidget       *widget,
                                                               cairo_t         *cr);

G_DEFINE_TYPE_WITH_PRIVATE (GcalArrowBin, gcal_arrow_bin, GTK_TYPE_BIN)

static void
get_padding_and_border (GtkWidget *widget,
                        GtkBorder *border,
                        guint     *arrow_size)
{
  GtkBorder tmp;

  if (border != NULL)
    {
      guint bw = gtk_container_get_border_width (GTK_CONTAINER (widget));
      gtk_style_context_get_padding (gtk_widget_get_style_context (widget),
                                     gtk_widget_get_state_flags (widget),
                                     border);

      gtk_style_context_get_border (gtk_widget_get_style_context (widget),
                                    gtk_widget_get_state_flags (widget),
                                    &tmp);

      border->top += tmp.top + bw;
      border->right += tmp.right + bw;
      border->bottom += tmp.bottom + bw;
      border->left += tmp.left + bw;
    }

  if (arrow_size != NULL)
    {
      gtk_style_context_get_style (
          gtk_widget_get_style_context (widget),
          "arrow-size", arrow_size,
          NULL);
    }
}

static void
draw_arrow (GtkWidget *widget,
            cairo_t   *cr)
{
  GcalArrowBinPrivate *priv;

  GdkRGBA color;
  GtkBorder border;
  guint arrow_size;
  gdouble selected_border;
  gint selected_border_radius;
  guint width;
  guint height;
  guint bw;
  gdouble arrow_start_point;

  GPoint p0 = { 0, 0 };
  GPoint p1 = { 0, 0 };
  GPoint p2 = { 0, 0 };
  GPoint p3 = { 0, 0 };
  GPoint p4 = { 0, 0 };

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  gtk_style_context_get_border (gtk_widget_get_style_context (widget),
                                gtk_widget_get_state_flags (widget),
                                &border);

  gtk_style_context_get (gtk_widget_get_style_context (widget),
                         gtk_widget_get_state_flags (widget),
                         "border-radius", &selected_border_radius,
                         NULL);

  gtk_style_context_get_style (gtk_widget_get_style_context (widget),
                               "arrow-size", &arrow_size, NULL);

  bw = gtk_container_get_border_width (GTK_CONTAINER (widget));
  width = gtk_widget_get_allocated_width (widget) - (border.left + border.right + 2 * bw);
  height = gtk_widget_get_allocated_height (widget) - (border.top + border.bottom + 2 * bw);

  selected_border = bw;
  switch (priv->arrow_position)
    {
      case GTK_POS_TOP:
        selected_border += border.top;
        break;
      case GTK_POS_BOTTOM:
        selected_border += border.bottom;
        break;
      case GTK_POS_LEFT:
        selected_border += border.left;
        break;
      case GTK_POS_RIGHT:
        selected_border += border.right;
        break;
    }

  if (priv->arrow_position == GTK_POS_LEFT ||
      priv->arrow_position == GTK_POS_RIGHT)
    {
      width -= arrow_size;
      arrow_start_point = arrow_size + selected_border / 2 + selected_border_radius +
          (height - 2 * (arrow_size + selected_border / 2 + selected_border_radius)) * priv->arrow_align;
    }
  else
    {
      height -= arrow_size;
      arrow_start_point = arrow_size + selected_border / 2 + selected_border_radius +
          (width - 2 * (arrow_size + selected_border / 2 + selected_border_radius)) * priv->arrow_align;
    }

  p0.x = p0.y = bw;
  switch (priv->arrow_position)
    {
      case GTK_POS_TOP:
        p0.x += arrow_start_point - arrow_size;
        p0.y += border.top + arrow_size;
        p1.x = p0.x;
        p1.y = p0.y - border.top;
        p2.x = p1.x + arrow_size;
        p2.y = p1.y - arrow_size;
        p3.x = p2.x + arrow_size;
        p3.y = p1.y;
        p4.x = p3.x;
        p4.y = p0.y;
        break;
      case GTK_POS_BOTTOM:
        p0.x += arrow_start_point - arrow_size;
        p0.y += height - border.bottom;
        p1.x = p0.x;
        p1.y = p0.y + border.bottom;
        p2.x = p1.x + arrow_size;
        p2.y = p1.y + arrow_size;
        p3.x = p2.x + arrow_size;
        p3.y = p1.y;
        p4.x = p3.x;
        p4.y = p0.y;
        break;
      case GTK_POS_LEFT:
        p0.x += arrow_size + border.left;
        p0.y += arrow_start_point - arrow_size;
        p1.x = p0.x - border.left;
        p1.y = p0.y;
        p2.x = p1.x - arrow_size;
        p2.y = p1.y + arrow_size;
        p3.x = p1.x;
        p3.y = p2.y + arrow_size;
        p4.x = p0.x;
        p4.y = p3.y;
        break;
      case GTK_POS_RIGHT:
        p0.x += width - border.left;
        p0.y += arrow_start_point - arrow_size;
        p1.x = p0.x + border.left;
        p1.y = p0.y;
        p2.x = p1.x + arrow_size;
        p2.y = p1.y + arrow_size;
        p3.x = p1.x;
        p3.y = p2.y + arrow_size;
        p4.x = p0.x;
        p4.y = p3.y;
        break;
    }

  cairo_save (cr);
  cairo_set_line_width (cr, border.bottom);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  cairo_move_to (cr, p0.x, p0.y);
  cairo_line_to (cr, p1.x, p1.y);
  cairo_line_to (cr, p2.x, p2.y);
  cairo_line_to (cr, p3.x, p3.y);
  cairo_line_to (cr, p4.x, p4.y);

  gtk_style_context_get_background_color (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget),
      &color);
  cairo_set_source_rgba (cr, color.red, color.green, color.blue, color.alpha);
  cairo_fill (cr);

  cairo_move_to (cr, p1.x, p1.y);
  cairo_line_to (cr, p2.x, p2.y);
  cairo_line_to (cr, p3.x, p3.y);

  gtk_style_context_get_border_color (
      gtk_widget_get_style_context (widget),
      gtk_widget_get_state_flags (widget),
      &color);
  cairo_set_source_rgb (cr, color.red, color.green, color.blue);
  cairo_stroke (cr);
  cairo_restore (cr);
}

static void
gcal_arrow_bin_class_init (GcalArrowBinClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass* widget_class;

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = gcal_arrow_bin_set_property;
  object_class->get_property = gcal_arrow_bin_get_property;

  widget_class->get_preferred_width =
      gcal_arrow_bin_get_preferred_width;
  widget_class->get_preferred_height_for_width =
      gcal_arrow_bin_get_preferred_height_for_width;
  widget_class->get_preferred_height =
      gcal_arrow_bin_get_preferred_height;
  widget_class->get_preferred_width_for_height =
      gcal_arrow_bin_get_preferred_width_for_height;
  widget_class->size_allocate = gcal_arrow_bin_size_allocate;
  widget_class->draw = gcal_arrow_bin_draw;

  /* Properties */
  g_object_class_install_property (
      object_class,
      PROP_ARROW_POSITION,
      g_param_spec_enum ("arrow-position",
                         "Arrow position",
                         "The position of the arrow in the container",
                         GTK_TYPE_POSITION_TYPE,
                         GTK_POS_BOTTOM,
                         G_PARAM_CONSTRUCT |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_NAME |
                         G_PARAM_STATIC_NICK |
                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property (
      object_class,
      PROP_ARROW_ALIGN,
      g_param_spec_double ("arrow-align",
                           "Arrow alignment",
                           "The alignment of the arrow in its side",
                           0.0,
                           1.0,
                           0.5,
                           G_PARAM_CONSTRUCT |
                           G_PARAM_READWRITE |
                           G_PARAM_STATIC_NAME |
                           G_PARAM_STATIC_NICK |
                           G_PARAM_STATIC_BLURB));

  /* Style properties */
  gtk_widget_class_install_style_property (
      widget_class,
      g_param_spec_uint ("arrow-size",
                         "Arrow width and height",
                         "The width and height of the arrow",
                         0,
                         G_MAXUINT32,
                         0,
                         G_PARAM_READABLE));
}

static void
gcal_arrow_bin_init (GcalArrowBin *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
}

static void
gcal_arrow_bin_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GcalArrowBinPrivate *priv;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (object));

  switch (property_id)
    {
      case PROP_ARROW_POSITION:
        priv->arrow_position = g_value_get_enum (value);
        break;
      case PROP_ARROW_ALIGN:
        priv->arrow_align = g_value_get_double (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        return;
    }
}

static void
gcal_arrow_bin_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GcalArrowBinPrivate *priv;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (object));

  switch (property_id)
    {
      case PROP_ARROW_POSITION:
        g_value_set_enum (value, priv->arrow_position);
        break;
      case PROP_ARROW_ALIGN:
        g_value_set_double (value, priv->arrow_align);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        return;
    }
}

static void
gcal_arrow_bin_get_preferred_width (GtkWidget *widget,
                                    gint      *minimum_width,
                                    gint      *natural_width)
{
  GcalArrowBinPrivate *priv;
  gint child_min, child_nat;
  GtkWidget *child;
  GtkBorder padding;
  gint minimum, natural;
  guint arrow_size;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));
  get_padding_and_border (widget, &padding, &arrow_size);

  minimum = 0;
  natural = 0;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_width (child,
                                      &child_min,
                                      &child_nat);
      minimum += child_min;
      natural += child_nat;
    }

  minimum += padding.left + padding.right;
  natural += padding.left + padding.right;

  if (priv->arrow_position == GTK_POS_LEFT ||
      priv->arrow_position == GTK_POS_RIGHT)
    {
      minimum += arrow_size;
      natural += arrow_size;
    }

  if (minimum_width)
    *minimum_width = minimum;

  if (natural_width)
    *natural_width = natural;
}

static void
gcal_arrow_bin_get_preferred_height_for_width (GtkWidget *widget,
                                               gint       width,
                                               gint      *minimum_height,
                                               gint      *natural_height)
{
  GcalArrowBinPrivate *priv;
  gint child_min, child_nat, child_width;
  GtkWidget *child;
  GtkBorder padding;
  guint arrow_size;
  gint minimum;
  gint natural;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  get_padding_and_border (widget, &padding, &arrow_size);

  minimum = 0;
  natural = 0;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    {
      child_width = width - (padding.left + padding.right);

      gtk_widget_get_preferred_height_for_width (child,
                                                 child_width,
                                                 &child_min,
                                                 &child_nat);
      minimum = MAX (minimum, child_min);
      natural = MAX (natural, child_nat);
    }

  minimum += padding.top + padding.bottom;
  natural += padding.top + padding.bottom;

  if (priv->arrow_position == GTK_POS_TOP ||
      priv->arrow_position == GTK_POS_BOTTOM)
    {
      minimum += arrow_size;
      natural += arrow_size;
    }

  if (minimum_height)
    *minimum_height = minimum;

  if (natural_height)
    *natural_height = natural;
}

static void
gcal_arrow_bin_get_preferred_height (GtkWidget *widget,
                                     gint      *minimum_height,
                                     gint      *natural_height)
{
  GcalArrowBinPrivate *priv;
  gint child_min, child_nat;
  GtkWidget *child;
  GtkBorder padding;
  guint arrow_size;
  gint minimum;
  gint natural;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  get_padding_and_border (widget, &padding, &arrow_size);

  minimum = 0;
  natural = 0;

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_height (child,
                                       &child_min,
                                       &child_nat);
      minimum = child_min;
      natural = child_nat;
    }

  minimum += padding.top + padding.bottom;
  natural += padding.top + padding.bottom;

  if (priv->arrow_position == GTK_POS_TOP ||
      priv->arrow_position == GTK_POS_BOTTOM)
    {
      minimum += arrow_size;
      natural += arrow_size;
    }

  if (minimum_height)
    *minimum_height = minimum;

  if (natural_height)
    *natural_height = natural;
}

static void
gcal_arrow_bin_get_preferred_width_for_height (GtkWidget *widget,
                                               gint       height,
                                               gint      *minimum_width,
                                               gint      *natural_width)
{
  GcalArrowBinPrivate *priv;

  gint child_min, child_nat, child_height;
  GtkWidget *child;
  GtkBorder padding;
  guint arrow_size;
  gint minimum;
  gint natural;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  get_padding_and_border (widget, &padding, &arrow_size);

  minimum = 0;
  natural = 0;

  child_height = height - (padding.top + padding.bottom);

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    {
      gtk_widget_get_preferred_width_for_height (child,
                                                 child_height,
                                                 &child_min,
                                                 &child_nat);
      minimum += child_min;
      natural += child_nat;
    }

  minimum += padding.left + padding.right;
  natural += padding.left + padding.right;

  if (priv->arrow_position == GTK_POS_LEFT ||
      priv->arrow_position == GTK_POS_RIGHT)
    {
      minimum += arrow_size;
      natural += arrow_size;
    }

 if (minimum_width)
    *minimum_width = minimum;

  if (natural_width)
    *natural_width = natural;
}

static void
gcal_arrow_bin_size_allocate (GtkWidget     *widget,
                              GtkAllocation *allocation)
{
  GcalArrowBinPrivate *priv;
  GtkAllocation child_allocation;
  GtkBorder padding;
  GtkWidget *child;
  guint arrow_size;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  gtk_widget_set_allocation (widget, allocation);

  child = gtk_bin_get_child (GTK_BIN (widget));
  if (child && gtk_widget_get_visible (child))
    {
      get_padding_and_border (widget, &padding, &arrow_size);

      child_allocation.x = allocation->x + padding.left +
        (priv->arrow_position == GTK_POS_LEFT ? arrow_size : 0);
      child_allocation.y = allocation->y + padding.top +
        (priv->arrow_position == GTK_POS_TOP ? arrow_size : 0);

      child_allocation.height =
        MAX (1,
             allocation->height - padding.top - padding.bottom);

      child_allocation.width =
        MAX (1,
             allocation->width - padding.left - padding.right);

      if (priv->arrow_position == GTK_POS_LEFT ||
          priv->arrow_position == GTK_POS_RIGHT)
        {
          child_allocation.width -= arrow_size;
        }
      else
        {
          child_allocation.height -= arrow_size;
        }

      gtk_widget_size_allocate (child, &child_allocation);
    }
}

static gboolean
gcal_arrow_bin_draw (GtkWidget *widget,
                     cairo_t   *cr)
{
  GcalArrowBinPrivate *priv;

  GtkBorder border;
  gdouble selected_border;
  gint selected_border_radius;
  guint width;
  guint height;
  guint bw;
  guint arrow_size;
  gdouble arrow_start_point;
  gdouble x, y;

  priv = gcal_arrow_bin_get_instance_private (GCAL_ARROW_BIN (widget));

  gtk_style_context_get_border (gtk_widget_get_style_context (widget),
                                gtk_widget_get_state_flags (widget),
                                &border);

  gtk_style_context_get (gtk_widget_get_style_context (widget),
                         gtk_widget_get_state_flags (widget),
                         "border-radius", &selected_border_radius,
                         NULL);

  gtk_style_context_get_style (gtk_widget_get_style_context (widget),
                               "arrow-size", &arrow_size, NULL);

  bw = gtk_container_get_border_width (GTK_CONTAINER (widget));
  width = gtk_widget_get_allocated_width (widget) - (border.left + border.right + 2 * bw);
  height = gtk_widget_get_allocated_height (widget) - (border.top + border.bottom + 2 * bw);

  selected_border = bw;

  x = border.left + bw;
  y = border.top + bw;
  if (priv->arrow_position == GTK_POS_LEFT)
    x += arrow_size;
  if (priv->arrow_position == GTK_POS_TOP)
    y += arrow_size;

  switch (priv->arrow_position)
    {
      case GTK_POS_TOP:
        selected_border += border.top;
        break;
      case GTK_POS_BOTTOM:
        selected_border += border.bottom;
        break;
      case GTK_POS_LEFT:
        selected_border += border.left;
        break;
      case GTK_POS_RIGHT:
        selected_border += border.right;
        break;
    }

  if (priv->arrow_position == GTK_POS_LEFT ||
      priv->arrow_position == GTK_POS_RIGHT)
    {
      width -= arrow_size;
      arrow_start_point = arrow_size + selected_border / 2 + selected_border_radius +
          (height - 2 * (arrow_size + selected_border / 2 + selected_border_radius)) * priv->arrow_align;
    }
  else
    {
      height -= arrow_size;
      arrow_start_point = arrow_size + selected_border / 2 + selected_border_radius +
          (width - 2 * (arrow_size + selected_border / 2 + selected_border_radius)) * priv->arrow_align;
    }

  gtk_render_background (gtk_widget_get_style_context (widget),
                         cr,
                         x, y,
                         width, height);
  gtk_render_frame_gap (gtk_widget_get_style_context (widget),
                        cr,
                        x, y,
                        width, height,
                        priv->arrow_position,
                        arrow_start_point - arrow_size,
                        arrow_start_point + arrow_size);

  draw_arrow (widget, cr);

  GTK_WIDGET_CLASS (gcal_arrow_bin_parent_class)->draw (widget, cr);

  return FALSE;
}

GtkWidget*
gcal_arrow_bin_new (void)
{
  return g_object_new (GCAL_TYPE_ARROW_BIN, NULL);
}

void
gcal_arrow_bin_set_arrow_pos (GcalArrowBin    *arrow,
                              GtkPositionType  arrow_pos)
{
  g_return_if_fail (GCAL_IS_ARROW_BIN (arrow));

  g_object_set (arrow, "arrow-position", arrow_pos, NULL);
}

GtkPositionType
gcal_arrow_bin_get_arrow_pos (GcalArrowBin *arrow)
{
  GtkPositionType pos;

  g_return_val_if_fail (GCAL_IS_ARROW_BIN (arrow), GTK_POS_BOTTOM);

  g_object_get (arrow, "arrow-position", &pos, NULL);
  return pos;
}

void
gcal_arrow_bin_set_arrow_align (GcalArrowBin *arrow,
                                gdouble       align)
{
  g_return_if_fail (GCAL_IS_ARROW_BIN (arrow));

  g_object_set (arrow, "arrow-align", align, NULL);
}

gdouble
gcal_arrow_bin_get_arrow_align (GcalArrowBin *arrow)
{
  gdouble align;

  g_return_val_if_fail (GCAL_IS_ARROW_BIN (arrow), 0.5);

  g_object_get (arrow, "arrow-align", &align, NULL);
  return align;
}