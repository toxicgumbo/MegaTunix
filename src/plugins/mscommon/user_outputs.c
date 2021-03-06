/*
 * Copyright (C) 2002-2012 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * Most of this file contributed by Perry Harrington
 * slight changes applied (naming, addition ofbspot 1-3 vars)
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

/*!
  \file src/plugins/mscommon/user_outputs.c
  \ingroup MSCommonPlugin,Plugins
  \brief MS personality common functions for User outputs
  \author David Andruczyk
  */

#include <assert.h>
#include <config.h>
#include <datamgmt.h>
#include <defines.h>
#include <enums.h>
#include <lookuptables.h>
#include <mscommon_comms.h>
#include <mscommon_plugin.h>
#include <debugging.h>
#include <multi_expr_loader.h>
#include <rtv_map_loader.h>
#include <stdlib.h>
#include <user_outputs.h>

/*!
  \brief enumerations for the user outputs for MS-1 firmwares
  */
enum
{
	COL_OBJECT = 0,
	COL_NAME,
	COL_RANGE,
	COL_ENTRY,
	COL_HYS,
	COL_ULIMIT,
	COL_MODE,
	COL_EDITABLE,
	NUM_COLS
};

static GList *views = NULL;
extern gconstpointer *global_data;


/*!
  \brief Forces the model based on the view
  \param data is unused
  \returns FALSE
  */
G_MODULE_EXPORT gboolean force_view_recompute(gpointer data)
{
	guint i = 0;
	for (i=0;i<g_list_length(views);i++)
		update_model_from_view(g_list_nth_data(views,i));
	return FALSE;
}


/*!
 \brief build_model_and_view() is called to create the model and view for
 a preexisting textview. Currently used to generate the User controlled outputs
 lists for MSnS and MSnEDIS firmwares
 \param widget is the pointer to The textview that this model and view is to be
 created for.
 */
G_MODULE_EXPORT void build_model_and_view(GtkWidget * widget)
{
	GtkWidget *view = NULL;
	GtkTreeModel *model = NULL;


	if (!DATA_GET(global_data,"rtvars_loaded"))
	{
		MTXDBG(CRITICAL,_("Realtime Variable definitions NOT LOADED!!!\n"));
		return;
	}

	model = create_model ();

	OBJ_SET_FULL(widget,"model",model,gtk_list_store_clear);
	OBJ_SET(model,"lim_offset",OBJ_GET(widget,"lim_offset"));
	OBJ_SET(model,"src_offset",OBJ_GET(widget,"src_offset"));
	OBJ_SET(model,"hys_offset",OBJ_GET(widget,"hys_offset"));
	OBJ_SET(model,"ulimit_offset",OBJ_GET(widget,"ulimit_offset"));
	OBJ_SET(model,"page",OBJ_GET(widget,"page"));
	OBJ_SET(model,"canID",OBJ_GET(widget,"canID"));

	view = gtk_tree_view_new_with_model(model);
	views = g_list_append(views,view);
	OBJ_SET(model,"view",(gpointer)view);
	gtk_container_add(GTK_CONTAINER(widget),view);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (view), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (view),COL_NAME);
	add_columns(GTK_TREE_VIEW(view), widget);
	update_model_from_view((GtkWidget *)view);
}


/*!
 \brief create_model() Creates a TreeModel used by the user outputs treeviews
 \returns a pointer to a newly created GtkTreeModel structure.
 */
G_MODULE_EXPORT GtkTreeModel * create_model(void)
{
	GtkListStore  *model;
	GtkTreeIter    iter;
	gint i = 0;
	gchar * data = NULL;
	gchar * name = NULL;
	gint lower = 0;
	gint upper = 0;
	gchar * range = NULL;
	gconstpointer * object = NULL;
	Rtv_Map *rtv_map = NULL;

	rtv_map = DATA_GET(global_data,"rtv_map");

	model = gtk_list_store_new (NUM_COLS, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);

	/* Append a row and fill in some data */
	while ((data = rtv_map->raw_list[i])!= NULL)
	{
		i++;
		object = NULL;
		name = NULL;
		object = (gconstpointer *)g_hash_table_lookup(rtv_map->rtv_hash,data);
		if (!object)
			continue;
		name = (gchar *) DATA_GET(object,"dlog_gui_name");
		if (!name)
			continue;
		if (DATA_GET(object,"real_lower"))
			lower = (gint)strtol(DATA_GET(object,"real_lower"),NULL,10);
		else
			lower = get_extreme_from_size_f((DataSize)DATA_GET(object,"size"),LOWER);
		if (DATA_GET(object,"real_upper"))
			upper = (gint)strtol(DATA_GET(object,"real_upper"),NULL,10);
		else
			upper = get_extreme_from_size_f((DataSize)DATA_GET(object,"size"),UPPER);
		range = g_strdup_printf("%i-%i",lower,upper);
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				COL_OBJECT, object,
				COL_NAME, name,
				COL_RANGE, range,
				COL_ENTRY, NULL,
				COL_HYS, NULL,
				COL_ULIMIT,NULL,
				COL_MODE,GTK_CELL_RENDERER_MODE_EDITABLE,
				COL_EDITABLE,TRUE,
				-1);
		g_free(range);
	}
	return GTK_TREE_MODEL(model);
}


/*!
 \brief add_columns() creates the column fields for a treeview.
 \param view is the pointer to the Treeview object
 \param widget is the Widget passed to extract the needed info from
 */
G_MODULE_EXPORT void add_columns(GtkTreeView *view, GtkWidget *widget)
{
	GtkCellRenderer     *renderer;
	GtkTreeViewColumn *col;
	GtkTreeModel        *model = gtk_tree_view_get_model (view);
	gchar * tmpbuf = NULL;
	gint output = -1;

	output = (GINT)OBJ_GET(widget,"output_num");
	/* --- Column #1, name --- */
	renderer = gtk_cell_renderer_text_new ();
	OBJ_SET(renderer, "column", (gint *)COL_NAME);
	tmpbuf = g_strdup_printf("Output %i Var.",output);
	col = gtk_tree_view_column_new_with_attributes (
			tmpbuf,  
			renderer,
			"markup", COL_NAME,
			NULL);
	g_free(tmpbuf);

	gtk_tree_view_column_set_sort_column_id (col, COL_NAME);
	gtk_tree_view_append_column (view, col);

	/* --- Column #2, range --- */

	renderer = gtk_cell_renderer_text_new ();
	OBJ_SET(renderer, "column", (gint *)COL_RANGE);
	col = gtk_tree_view_column_new_with_attributes (
			"Range",  
			renderer,
			"text", COL_RANGE,
			NULL);
	gtk_tree_view_append_column (view, col);

	/* --- Column #3, user choice --- */

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect(renderer, "edited",
			G_CALLBACK(cell_edited),model);
	OBJ_SET(renderer, "column", (gint *)COL_ENTRY);
	col = gtk_tree_view_column_new_with_attributes (
			"Value",  
			renderer,
			"text", COL_ENTRY,
			"mode",COL_MODE,
			"editable",COL_EDITABLE,
			NULL);
	gtk_tree_view_append_column (view, col);

	/* --- Column #4 , Hysteresis value --- */

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect(renderer, "edited",
			G_CALLBACK(cell_edited),model);
	OBJ_SET(renderer, "column", (gint *)COL_HYS);
	col = gtk_tree_view_column_new_with_attributes (
			"Off Hysteresis",  
			renderer,
			"text", COL_HYS,
			"editable",COL_EDITABLE,
			NULL);
	gtk_tree_view_append_column (view, col);
	/* If no hysteresis params, this column should NOT show */
	if (OBJ_GET(widget,"hys_offset") == NULL)
		gtk_tree_view_column_set_visible(col,FALSE);


	/* --- Column #5, user choice --- */

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect(renderer, "edited",
			G_CALLBACK(cell_edited),model);
	OBJ_SET(renderer, "column", (gint *)COL_ULIMIT);
	col = gtk_tree_view_column_new_with_attributes (
			"Upper Limit",  
			renderer,
			"text", COL_ULIMIT,
			"editable",COL_EDITABLE,
			NULL);
	gtk_tree_view_append_column (view, col);
	/* If no upperlimit params, this column should NOT show */
	if (OBJ_GET(widget,"ulimit_offset") == NULL)
		gtk_tree_view_column_set_visible(col,FALSE);

}


/*!
 \brief cell_edited() is called whenever an editable cell on the user outputs
 treeviews are modified. This function will check and verify the user input
 is valid, and process it and send it to the ECU
 \param cell is the pointer to the cell that was edited
 \param path_string is the tree_path for the treeview (see GTK+ docs)
 \param new_text is the new text that was entered into the cell
 \param data is the pointer to the GtkTreeModel
 */
G_MODULE_EXPORT void cell_edited(GtkCellRendererText *cell, 
		const gchar * path_string,
		const gchar * new_text,
		gpointer data)
{
	GtkTreeModel *model = (GtkTreeModel *)data;
	GtkTreeView *view = (GtkTreeView *)OBJ_GET(model,"view");
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;
	gboolean temp_dep;
	gint lower = 0;
	gint upper = 0;
	gfloat new = 0;
	gint column = 0;
	gconstpointer *object = NULL;
	gint src_offset = -1;
	gint lim_offset = -1;
	gint rt_offset = -1;
	gint hys_offset = -1;
	gint ulimit_offset = -1;
	gint precision = 0;
	gfloat x = 0.0;
	gfloat tmpf = 0.0;
	gfloat *multiplier = NULL;
	gfloat *adder = NULL;
	gint result = 0;
	gint canID = 0;
	gint page = 0;
	gchar *key = NULL;
	gchar *hash_key = NULL;
	DataSize size = 0;
	MultiExpr *multi = NULL;
	GHashTable *hash = NULL;
	GHashTable *sources_hash = NULL;

	sources_hash = DATA_GET(global_data,"sources_hash");
	column = (GINT) OBJ_GET (cell, "column");
	canID = (GINT) OBJ_GET(model,"canID");
	page = (GINT) OBJ_GET(model,"page");
	size = (DataSize) OBJ_GET(model,"size");
	src_offset = (GINT) OBJ_GET(model,"src_offset");
	lim_offset = (GINT) OBJ_GET(model,"lim_offset");
	hys_offset = (GINT) OBJ_GET(model,"hys_offset");
	ulimit_offset = (GINT) OBJ_GET(model,"ulimit_offset");

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, COL_OBJECT, &object, -1);

	rt_offset = (GINT) DATA_GET(object,"offset");
	precision = (GINT) DATA_GET(object,"precision");
	new = (gfloat)g_ascii_strtod(g_strdelimit((gchar *)new_text,",.",'.'),NULL);
	if (DATA_GET(object,"multi_expr_hash"))
	{
		hash = DATA_GET(object,"multi_expr_hash");
		key = (gchar *)DATA_GET(object,"source_key");
		if (key)
		{
			hash_key = (gchar *)g_hash_table_lookup(sources_hash,key);
			if (hash_key)
				multi = (MultiExpr *)g_hash_table_lookup(hash,hash_key);
			else
				multi = (MultiExpr *)g_hash_table_lookup(hash,"DEFAULT");

		}
		else
			multi = (MultiExpr *)g_hash_table_lookup(hash,"DEFAULT");
		if (!multi)
			return;
		if (new < multi->lower_limit)
			new = multi->lower_limit;
		if (new > multi->upper_limit)
			new = multi->upper_limit;

			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,g_strdup_printf("%1$.*2$f",new,precision), -1);

		/* Then evaluate it in reverse.... */
		multiplier = multi->fromecu_mult;
		adder = multi->fromecu_add;
		if ((multiplier) && (adder))
			tmpf = (new - (*adder))/(*multiplier);
		else if (multiplier)
			tmpf = new/(*multiplier);
		else
			tmpf = new;
		/* Then if it used a lookuptable, reverse map it if possible 
		 * to determine the ADC reading we need to send to ECU
		 */
		if (multi->lookuptable)
			result = direct_reverse_lookup_f(multi->lookuptable,(gint)tmpf);
		else
			result = (gint)tmpf;
	}
	else
	{

		if (DATA_GET(object,"real_lower"))
			lower = (gint)strtol(DATA_GET(object,"real_lower"),NULL,10);
		else
			lower = get_extreme_from_size_f((DataSize)DATA_GET(object,"size"),LOWER);
		if (DATA_GET(object,"real_upper"))
			upper = (gint)strtol(DATA_GET(object,"real_upper"),NULL,10);
		else
			upper = get_extreme_from_size_f((DataSize)DATA_GET(object,"size"),UPPER);
		multiplier = DATA_GET(object,"fromecu_mult");
		adder = DATA_GET(object,"fromecu_add");
		temp_dep = (GBOOLEAN)DATA_GET(object,"temp_dep");

		if (new < lower)
			new = lower;
		if (new > upper)
			new = upper;

			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column,g_strdup_printf("%1$.*2$f",new,precision), -1);

		/* First conver to fahrenheit temp scale if temp dependant */
		if (temp_dep)
			x = temp_to_ecu_f(new);
		else
			x = new;
		/* Then evaluate it in reverse.... */
		if ((multiplier) && (adder))
			tmpf = (x - (*adder))/(*multiplier);
		else if (multiplier)
			tmpf = x/(*multiplier);
		else
			tmpf = x;
		/* Then if it used a lookuptable,  reverse map it if possible
		 * to determine the ADC reading we need to send to ECU
		 */
		if (DATA_GET(object,"lookuptable"))
			result = reverse_lookup_f(object,(gint)tmpf);
		else
			result = (gint)tmpf;
	}

	switch (column)
	{
		case COL_HYS:
			ms_send_to_ecu(canID, page, hys_offset, MTX_U08, result, TRUE);
			break;
		case COL_ULIMIT:
			ms_send_to_ecu(canID, page, ulimit_offset, MTX_U08, result, TRUE);
			break;
		case COL_ENTRY:
			ms_send_to_ecu(canID, page, src_offset, MTX_U08, rt_offset, TRUE);
			ms_send_to_ecu(canID, page, lim_offset, MTX_U08, result, TRUE);
			break;
	}
	gdk_threads_add_timeout(500,(GSourceFunc)deferred_model_update,(GtkWidget *)view);
	return;

}


/*!
 \brief update_model_from_view() is called after a cell is cuccessfully edited
 and updates the treemodel associated with the view.
 \see cell_edited
 \param widget is the pointer to the TreeView widget.
 */
G_MODULE_EXPORT void update_model_from_view(GtkWidget * widget)
{
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW(widget));
	GtkTreeIter    iter;
	gconstpointer *object = NULL;
	GtkTreePath *treepath = NULL;
	gint src_offset = 0;
	gint lim_offset = 0;
	gint hys_offset = -1;
	gint ulimit_offset = -1;
	gint canID = 0;
	gint page = 0;
	gint offset = 0;
	gint cur_val = 0;
	gint hys_val = 0;
	gint ulimit_val = 0;
	gint x = 0;
	gfloat tmpf = 0.0;
	gfloat result = 0.0;
	gfloat *multiplier = NULL;
	gfloat *adder = NULL;
	gint precision = 0;
	gboolean temp_dep = FALSE;
	gboolean looptest = FALSE;
	gint mtx_temp_units;
	gchar * tmpbuf = NULL;
	gchar * key = NULL;
	gchar * hash_key = NULL;
	GHashTable *hash = NULL;
	MultiExpr * multi = NULL;
	GHashTable *sources_hash = NULL;

	if (!gtk_tree_model_get_iter_first(model,&iter))
		return;
	sources_hash = DATA_GET(global_data,"sources_hash");
	mtx_temp_units = (GINT)DATA_GET(global_data,"mtx_temp_units");
	src_offset = (GINT)OBJ_GET(model,"src_offset");
	lim_offset = (GINT)OBJ_GET(model,"lim_offset");
	hys_offset = (GINT)OBJ_GET(model,"hys_offset");
	ulimit_offset = (GINT)OBJ_GET(model,"ulimit_offset");
	page = (GINT)OBJ_GET(model,"page");
	canID = (GINT)OBJ_GET(model,"canID");

	offset = ms_get_ecu_data(canID,page,src_offset,MTX_U08);
	cur_val = ms_get_ecu_data(canID,page,lim_offset,MTX_U08);
	hys_val = ms_get_ecu_data(canID,page,hys_offset,MTX_U08);
	ulimit_val = ms_get_ecu_data(canID,page,ulimit_offset,MTX_U08);

	looptest = TRUE;
	while (looptest)
	{
		gtk_tree_model_get (model, &iter, COL_OBJECT, &object, -1);
		if (offset == (GINT)DATA_GET(object,"offset"))
		{
			precision =(GINT)DATA_GET(object,"precision");
			temp_dep =(GBOOLEAN)DATA_GET(object,"temp_dep");
			if (DATA_GET(object,"multi_expr_hash"))
			{
				hash = DATA_GET(object,"multi_expr_hash");
				key = (gchar *)DATA_GET(object,"source_key");
				if (key)
				{
					hash_key = (gchar *)g_hash_table_lookup(sources_hash,key);
					if (hash_key)
						multi = (MultiExpr *)g_hash_table_lookup(hash,hash_key);
					else
						multi = (MultiExpr *)g_hash_table_lookup(hash,"DEFAULT");
				}
				else
					multi = (MultiExpr *)g_hash_table_lookup(hash,"DEFAULT");
				if (!multi)
					continue;


				/* TEXT ENTRY part */
				if (multi->lookuptable)
					x = direct_lookup_data_f(multi->lookuptable,cur_val);
				else
					x = cur_val;

				multiplier = multi->fromecu_mult;
				adder = multi->fromecu_add;
				if ((multiplier) && (adder))
					tmpf = (x * (*multiplier)) + (*adder);
				else if (multiplier)
					tmpf = (x * (*multiplier));
				else
					tmpf = x;

				if (temp_dep)
					result = temp_to_host_f(tmpf);
				else
					result = tmpf;
				tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);

				gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ENTRY,tmpbuf, -1);
				g_free(tmpbuf);

				/* HYSTERESIS VALUE */
				if (OBJ_GET(model,"hys_offset") != NULL)
				{
					if (multi->lookuptable)
						x = direct_lookup_data_f(multi->lookuptable,hys_val);
					else
						x = hys_val;
					if ((multiplier) && (adder))
						tmpf = (x * (*multiplier)) + (*adder);
					else if (multiplier)
						tmpf = (x * (*multiplier));
					else
						tmpf = x;

					if (temp_dep)
						result = temp_to_host_f(tmpf);
					else
						result = tmpf;

					tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_HYS,tmpbuf, -1);
					g_free(tmpbuf);

				}
				/* UPPER LIMIT VALUE */
				if (OBJ_GET(model,"ulimit_offset") != NULL)
				{
					if (multi->lookuptable)
						x = direct_lookup_data_f(multi->lookuptable,ulimit_val);
					else
						x = ulimit_val;
					if ((multiplier) && (adder))
						tmpf = (x * (*multiplier)) + (*adder);
					else if (multiplier)
						tmpf = (x * (*multiplier));
					else
						tmpf = x;
					if (temp_dep)
						result = temp_to_host_f(tmpf);
					else
						result = tmpf;

					tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ULIMIT,tmpbuf, -1);
					g_free(tmpbuf);
				}
			}

			else /* Non multi-expression variable */
			{
				/* TEXT ENTRY part */
				if (DATA_GET(object,"lookuptable"))
					x = lookup_data_f(object,cur_val);
				else
					x = cur_val;

				multiplier = DATA_GET(object,"fromecu_mult");
				adder = DATA_GET(object,"fromecu_add");
				if ((multiplier) && (adder))
					tmpf = (x * (*multiplier)) + (*adder);
				else if (multiplier)
					tmpf = (x * (*multiplier));
				else
					tmpf = x;

				if (temp_dep)
					result = temp_to_host_f(tmpf);
				else
					result = tmpf;
				tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);

				gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ENTRY,tmpbuf, -1);
				g_free(tmpbuf);

				/* HYSTERESIS VALUE */
				if (OBJ_GET(model,"hys_offset") != NULL)
				{
					if (DATA_GET(object,"lookuptable"))
						x = lookup_data_f(object,hys_val);
					else
						x = hys_val;
					if ((multiplier) && (adder))
						tmpf = (x * (*multiplier)) + (*adder);
					else if (multiplier)
						tmpf = (x * (*multiplier));
					else
						tmpf = x;
					if (temp_dep)
						result = temp_to_host_f(tmpf);
					else
						result = tmpf;

					tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_HYS,tmpbuf, -1);
					g_free(tmpbuf);

				}
				/* UPPER LIMIT VALUE */
				if (OBJ_GET(model,"ulimit_offset") != NULL)
				{
					if (DATA_GET(object,"lookuptable"))
						x = lookup_data_f(object,ulimit_val);
					else
						x = ulimit_val;
					if ((multiplier) && (adder))
						tmpf = (x * (*multiplier)) + (*adder);
					else if (multiplier)
						tmpf = (x * (*multiplier));
					else
						tmpf = x;
					if (temp_dep)
						result = temp_to_host_f(tmpf);
					else
						result = tmpf;

					tmpbuf =  g_strdup_printf("%1$.*2$f",result,precision);
					gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ULIMIT,tmpbuf, -1);
					g_free(tmpbuf);
				}
			}

			/* Scroll the treeview s othe current one is in view */
			treepath = gtk_tree_model_get_path (GTK_TREE_MODEL(model),&iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(widget),treepath,NULL,TRUE,1.0,0.0);
			gtk_tree_path_free(treepath);


			/*printf("offset matched for object %s\n",(gchar *)DATA_GET(object,"dlog_gui_name"));*/

		}
		else
		{
			tmpbuf = g_strdup("");
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ENTRY,tmpbuf, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_HYS,tmpbuf, -1);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, COL_ULIMIT,tmpbuf, -1);
			g_free(tmpbuf);
		}
		looptest = gtk_tree_model_iter_next(model,&iter);
	}
}


/*!
  \brief This is called from a timeout in order to trigger a model/view refresh
  \param widget is the pointer to the view
  \returns FALSE to cancel the timeout
  */
G_MODULE_EXPORT gboolean deferred_model_update(GtkWidget * widget)
{
	update_model_from_view(widget);
	return FALSE;
}
