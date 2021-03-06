/*
 * Copyright (C) 2006 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * MegaTunix curve widget
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file widgets/curve.h
  \ingroup WidgetHeaders,Headers
  \brief Public header for the MtxCurve simple 2D plotter Widget
  \author David Andruczyk
  */

#ifndef MTX_CURVE_H
#define MTX_CURVE_H

#include <config.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS

#define MTX_TYPE_CURVE		(mtx_curve_get_type ())
#define MTX_CURVE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MTX_TYPE_CURVE, MtxCurve))
#define MTX_CURVE_CLASS(obj)		(G_TYPE_CHECK_CLASS_CAST ((obj), MTX_CURVE, MtxCurveClass))
#define MTX_IS_CURVE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MTX_TYPE_CURVE))
#define MTX_IS_CURVE_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), MTX_TYPE_CURVE))
#define MTX_CURVE_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), MTX_TYPE_CURVE, MtxCurveClass))


typedef struct _MtxCurve		MtxCurve;
typedef struct _MtxCurveClass	MtxCurveClass;
typedef struct _MtxCurveCoord	MtxCurveCoord;

/*! 
  \brief CurveColorIndex enum,  for indexing into the color arrays
 */
typedef enum  
{
	CURVE_COL_BG = 0,
	CURVE_COL_FG,
	CURVE_COL_SEL,
	CURVE_COL_PROX,
	CURVE_COL_GRAT,
	CURVE_COL_TEXT,
	CURVE_COL_MARKER,
	CURVE_COL_EDIT,
	CURVE_NUM_COLORS
}CurveColorIndex;

/*! 
  \brief CurveSignals enum
 */
enum
{	
	CHANGED_SIGNAL,
	VERTEX_PROXIMITY_SIGNAL,
	MARKER_PROXIMITY_SIGNAL,
	LAST_SIGNAL
};

/*! 
  \brief _MtxCurve structure
 */
struct _MtxCurve
{	/* public data */
	GtkDrawingArea parent;		/*!< Parent Widget */
};

/*! 
  \brief _MtxCurveClass structure
 */
struct _MtxCurveClass
{
	GtkDrawingAreaClass parent_class;	/*!< Parent Class */
	/* Signal for coordinate change via drag/drop edit */
	void (*coords_changed) (MtxCurve *);	/*!< Coords changed signal */
	/* Signal for vertex proximity notify */
	void (*vertex_proximity) (MtxCurve *);	/*!< Vertex Proximity signal */
	/* Signal for marker proximity notify */
	void (*marker_proximity) (MtxCurve *);	/*!< Marker Proximity signal */
};

/*! 
  \brief _MtxCurveCoord structure containing the coordinates
 */
struct _MtxCurveCoord
{
	gfloat x;		/*!< X value */
	gfloat y;		/*!< Y value */
};

GType mtx_curve_get_type (void) G_GNUC_CONST;

GtkWidget* mtx_curve_new (void);

/* Point manipulation */
gboolean mtx_curve_get_coords (MtxCurve *, gint *, MtxCurveCoord *);

/* Do NOT free array of returned points! */
gboolean mtx_curve_set_coords (MtxCurve *, gint , MtxCurveCoord *);
gboolean mtx_curve_get_coords_at_index (MtxCurve *, gint , MtxCurveCoord * );
gboolean mtx_curve_set_coords_at_index (MtxCurve *, gint , MtxCurveCoord );
gboolean mtx_curve_set_empty_array(MtxCurve *, gint);
gint mtx_curve_get_active_coord_index(MtxCurve *);

/* Precision of data */
gboolean mtx_curve_set_x_precision(MtxCurve *, gint);
gboolean mtx_curve_set_y_precision(MtxCurve *, gint);
gint mtx_curve_get_x_precision(MtxCurve *);
gint mtx_curve_get_y_precision(MtxCurve *);

/* Axis locking */
gboolean mtx_curve_set_x_axis_lock_state(MtxCurve *, gboolean);
gboolean mtx_curve_get_x_axis_lock_state(MtxCurve *);
gboolean mtx_curve_set_y_axis_lock_state(MtxCurve *, gboolean);
gboolean mtx_curve_get_y_axis_lock_state(MtxCurve *);

/* Title */
gboolean mtx_curve_set_title (MtxCurve *,gchar *);
const gchar * mtx_curve_get_title (MtxCurve *);

/* Colors */
gboolean mtx_curve_set_color (MtxCurve *, CurveColorIndex , GdkColor );
gboolean mtx_curve_get_color (MtxCurve *, CurveColorIndex , GdkColor *);

/* Rendering */
gboolean mtx_curve_set_auto_hide_vertexes (MtxCurve *, gboolean);
gboolean mtx_curve_get_get_auto_hide_vertexes (MtxCurve *);
gboolean mtx_curve_set_show_vertexes (MtxCurve *, gboolean);
gboolean mtx_curve_get_get_show_vertexes (MtxCurve *);
gboolean mtx_curve_set_show_graticule (MtxCurve *, gboolean );
gboolean mtx_curve_get_show_graticule (MtxCurve *);
gboolean mtx_curve_get_show_edit_marker (MtxCurve *);
gboolean mtx_curve_get_show_x_marker (MtxCurve *);
gboolean mtx_curve_get_show_y_marker (MtxCurve *);
gboolean mtx_curve_set_show_edit_marker (MtxCurve *, gboolean );
gboolean mtx_curve_set_show_x_marker (MtxCurve *, gboolean );
gboolean mtx_curve_set_show_y_marker (MtxCurve *, gboolean );
gboolean mtx_curve_set_x_axis_label (MtxCurve *, const gchar * );
gboolean mtx_curve_set_y_axis_label (MtxCurve *, const gchar * );
gboolean mtx_curve_get_x_marker_value (MtxCurve *, gfloat *);
gboolean mtx_curve_get_y_marker_value (MtxCurve *, gfloat *);
void mtx_curve_set_x_marker_value (MtxCurve *, gfloat );
void mtx_curve_set_y_marker_value (MtxCurve *, gfloat );
gboolean mtx_curve_set_hard_limits (MtxCurve *, gfloat, gfloat, gfloat, gfloat);
gboolean mtx_curve_get_hard_limits (MtxCurve *, gfloat *, gfloat *, gfloat *, gfloat *);

/* Retrieval of current mouse proximity vertex index */
gint mtx_curve_get_vertex_proximity_index (MtxCurve *);
/* Retrieval of current marker proximity vertex index */
gint mtx_curve_get_marker_proximity_index (MtxCurve *);

G_END_DECLS

#endif
