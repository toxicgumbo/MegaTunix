/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute etc. this as long as the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#include <config.h>
#include <defines.h>
#include <enums.h>
#include <gui_handlers.h>
#include <structures.h>
#include <warmwizard_gui.h>

const gchar *F_warmup_labels[] = {"-40","-20",  "0", "20", "40",
                                 "60", "80","100","130","160"};
extern const gchar *status_msgs[];
extern struct DynamicLabels labels;
extern struct DynamicProgress progress;
extern struct DynamicButtons buttons;
extern struct DynamicSpinners spinners;
extern struct DynamicMisc misc;
extern GList *ve_widgets[MAX_SUPPORTED_PAGES][2*MS_PAGE_SIZE];
extern GdkColor red;


void build_warmwizard(GtkWidget *parent_frame)
{
	gint i = 0;
	GtkWidget *button;
	GtkWidget *pbar;
	GtkWidget *vbox;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox2;
	GtkWidget *tmpspin;
	GtkWidget *spinner;
	GtkAdjustment *adj;
	extern GtkTooltips *tip;
	extern GList *store_controls;
	extern GList *temp_dep;

	return;
	/* MAin box inside parent frame */
	vbox = gtk_vbox_new(FALSE,0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_container_add(GTK_CONTAINER(parent_frame),vbox);

	/* splits window into left/right halves */
	hbox = gtk_hbox_new(FALSE,5);
	gtk_box_pack_start(GTK_BOX(vbox),hbox,TRUE,TRUE,0);

	/*  Box to contain the warmup entries */
	vbox2 = gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),vbox2,FALSE,TRUE,0);

	frame = gtk_frame_new("Warmup Enrichment (%)");
	gtk_box_pack_start(GTK_BOX(vbox2),frame,TRUE,TRUE,0);

	table = gtk_table_new(10,4,FALSE);
	misc.warmwizard_table = table;
	gtk_table_set_row_spacings(GTK_TABLE(table),7);
	gtk_table_set_col_spacings(GTK_TABLE(table),30);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_container_add(GTK_CONTAINER(frame),table);

	for (i=0;i<10;i++)
	{
		tmpspin = g_list_nth_data(ve_widgets[0][WARMUP_BINS_OFFSET+i],0);
		adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
		spinner = gtk_spin_button_new(adj,1,0);
		ve_widgets[0][WARMUP_BINS_OFFSET+i] = g_list_append(ve_widgets[0][WARMUP_BINS_OFFSET+i],(gpointer)spinner);
		g_object_set_data(G_OBJECT(spinner),"page",
				GINT_TO_POINTER(0));
		g_object_set_data(G_OBJECT(spinner),"offset",
				GINT_TO_POINTER(WARMUP_BINS_OFFSET+i));
		g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
				GINT_TO_POINTER(1*100));
		g_object_set_data(G_OBJECT(spinner),"conv_type",
				GINT_TO_POINTER(CONV_NOTHING));
		g_object_set_data(G_OBJECT(spinner),"dl_type",
				GINT_TO_POINTER(IMMEDIATE));
		spinners.warmwizard[i] = spinner;
		gtk_widget_set_size_request(spinner,45,-1);
		gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);

		/* SIGNAL connection is NOT required here as this widget shares
		 * a gtk_adjustment with the spinner(s) on the enrichments tab
		 * and when the adjustment changes the signal gets emitted
		 * there.  if we havethe signal handlers here it gets double
		 * emitted which although not hurting anything isn't "correct"
		 */
		gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, i, i+1,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

		label = gtk_label_new(F_warmup_labels[i]);
		labels.warmwizard_lab[i] = label;
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
		gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 1, 2, i, i+1,
				(GtkAttachOptions) (GTK_FILL),
				(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);
	}

	/*  Box to contain the cranking/afterstart frames */
	vbox2 = gtk_vbox_new(FALSE,0);
	gtk_box_pack_start(GTK_BOX(hbox),vbox2,TRUE,TRUE,0);

	frame = gtk_frame_new("Cranking Pulsewidth (ms)");
	gtk_box_pack_start(GTK_BOX(vbox2),frame,FALSE,TRUE,0);

	hbox2 = gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox2);

	table = gtk_table_new(3,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),7);
	gtk_table_set_col_spacings(GTK_TABLE(table),10);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox2),table,TRUE,TRUE,20);

	label = gtk_label_new("Priming Pulse");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Pulsewidth at -40 \302\260 F.");
	temp_dep = g_list_append(temp_dep, (gpointer)label);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("Pulsewidth at 170 \302\260 F.");
	labels.ww_cr_pulse_hightemp_lab = label;
	temp_dep = g_list_append(temp_dep, (gpointer)label);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	/* Priming pulse copy */
	tmpspin = g_list_nth_data(ve_widgets[0][119],0);
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
	spinner = gtk_spin_button_new(adj,0,1);
	ve_widgets[0][119] = g_list_append(ve_widgets[0][119],(gpointer)spinner);
	g_object_set_data(G_OBJECT(spinner),"page",
			GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(spinner),"offset",
			GINT_TO_POINTER(119));
	g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
			GINT_TO_POINTER(10*100));
	g_object_set_data(G_OBJECT(spinner),"conv_type",
			GINT_TO_POINTER(CONV_MULT));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	gtk_widget_set_size_request(spinner,55,-1);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 40, 0);

	/* Cranking Pulsewidth at -40 deg F */
	tmpspin = g_list_nth_data(ve_widgets[0][64],0);
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
	spinner = gtk_spin_button_new(adj,0,1);
	ve_widgets[0][64] = g_list_append(ve_widgets[0][64],(gpointer)spinner);
	g_object_set_data(G_OBJECT(spinner),"page",
			GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(spinner),"offset",
			GINT_TO_POINTER(64));
	g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
			GINT_TO_POINTER(10*100));
	g_object_set_data(G_OBJECT(spinner),"conv_type",
			GINT_TO_POINTER(CONV_MULT));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	gtk_widget_set_size_request(spinner,55,-1);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 40, 0);

	/* Cranking Pulsewidth at 170 deg F */
	tmpspin = g_list_nth_data(ve_widgets[0][65],0);
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
	spinner = gtk_spin_button_new(adj,0,1);
	ve_widgets[0][65] = g_list_append(ve_widgets[0][65],(gpointer)spinner);
	g_object_set_data(G_OBJECT(spinner),"page",
			GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(spinner),"offset",
			GINT_TO_POINTER(65));
	g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
			GINT_TO_POINTER(10*100));
	g_object_set_data(G_OBJECT(spinner),"conv_type",
			GINT_TO_POINTER(CONV_MULT));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	gtk_widget_set_size_request(spinner,55,-1);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 40, 0);

	/* Afterstart Enrichment frame */
	frame = gtk_frame_new("Afterstart Enrichment");
	gtk_box_pack_start(GTK_BOX(vbox2),frame,FALSE,TRUE,0);

	hbox2 = gtk_hbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),hbox2);

	table = gtk_table_new(3,3,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),7);
	gtk_table_set_col_spacings(GTK_TABLE(table),10);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_box_pack_start(GTK_BOX(hbox2),table,TRUE,TRUE,20);

	label = gtk_label_new("Enrichment (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new("# of Ignition Cycles   ");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	/* Enrichment % */
	tmpspin = g_list_nth_data(ve_widgets[0][66],0);
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
	spinner = gtk_spin_button_new(adj,1,0);
	ve_widgets[0][66] = g_list_append(ve_widgets[0][66],(gpointer)spinner);
	g_object_set_data(G_OBJECT(spinner),"page",
			GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(spinner),"offset",
			GINT_TO_POINTER(66));
	g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
			GINT_TO_POINTER(1*100));
	g_object_set_data(G_OBJECT(spinner),"conv_type",
			GINT_TO_POINTER(CONV_NOTHING));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	gtk_widget_set_size_request(spinner,55,-1);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 40, 0);

	/* Number of cycles  */
	tmpspin = g_list_nth_data(ve_widgets[0][67],0);
	adj = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(tmpspin));
	spinner = gtk_spin_button_new(adj,1,0);
	ve_widgets[0][67] = g_list_append(ve_widgets[0][67],(gpointer)spinner);
	g_object_set_data(G_OBJECT(spinner),"page",
			GINT_TO_POINTER(0));
	g_object_set_data(G_OBJECT(spinner),"offset",
			GINT_TO_POINTER(67));
	g_object_set_data(G_OBJECT(spinner),"conv_factor_x100",
			GINT_TO_POINTER(1*100));
	g_object_set_data(G_OBJECT(spinner),"conv_type",
			GINT_TO_POINTER(CONV_NOTHING));
	g_object_set_data(G_OBJECT(spinner),"dl_type",
			GINT_TO_POINTER(IMMEDIATE));
	gtk_widget_set_size_request(spinner,55,-1);
	gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (spinner), FALSE);
	gtk_table_attach (GTK_TABLE (table), spinner, 2, 3, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 40, 0);

	frame = gtk_frame_new("Real-Time Variables");
	gtk_box_pack_start(GTK_BOX(vbox2),frame,TRUE,TRUE,0);

	table = gtk_table_new(5,4,FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table),2);
	gtk_table_set_col_spacings(GTK_TABLE(table),2);
	gtk_table_set_col_spacing(GTK_TABLE(table),0,0);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_container_add(GTK_CONTAINER(frame),table);

	/* Coolant */
	label = gtk_label_new(NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 10, 0);

	label = gtk_label_new("Coolant (\302\260 F.)");
	temp_dep = g_list_append(temp_dep, (gpointer)label);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 0, 1,
			(GtkAttachOptions) (GTK_SHRINK),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);
	labels.ww_clt_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_table_attach (GTK_TABLE (table), pbar, 3, 4, 0, 1,
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK), 0, 0);
	progress.ww_clt_pbar = pbar;

	/* Warmup Correction  */
	label = gtk_label_new(NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 10, 0);

	label = gtk_label_new("Warmup (%)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 1, 2,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 1, 2,
			(GtkAttachOptions) (GTK_SHRINK),
			(GtkAttachOptions) (GTK_SHRINK), 0, 0);
	labels.ww_warmcorr_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_table_attach (GTK_TABLE (table), pbar, 3, 4, 1, 2,
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK), 0, 0);
	progress.ww_warmcorr_pbar = pbar;

	/* O2 Voltage Label*/
	label = gtk_label_new(NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 10, 0);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),"O<sub>2</sub> (Volts)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 2, 3,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);

	/* O2 Voltage value*/
	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 2, 3,
			(GtkAttachOptions) (GTK_SHRINK),
			(GtkAttachOptions) (GTK_SHRINK), 0, 0);
	labels.ww_ego_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_table_attach (GTK_TABLE (table), pbar, 3, 4, 2, 3,
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK), 0, 0);
	progress.ww_ego_pbar = pbar;

	/* MAP Sensor */       
	label = gtk_label_new(NULL);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 10, 0);

	label = gtk_label_new("MAP (kPa)");
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), label, 1, 2, 3, 4,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 0);

	label = gtk_label_new(NULL);
	gtk_widget_set_size_request(label,55,-1);
	gtk_table_attach (GTK_TABLE (table), label, 2, 3, 3, 4,
			(GtkAttachOptions) (GTK_SHRINK),
			(GtkAttachOptions) (GTK_SHRINK), 0, 0);
	labels.ww_map_lab = label;

	pbar = gtk_progress_bar_new();
	gtk_progress_bar_set_orientation(GTK_PROGRESS_BAR(pbar),
			GTK_PROGRESS_LEFT_TO_RIGHT);
	gtk_table_attach (GTK_TABLE (table), pbar, 3, 4, 3, 4,
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK),
			(GtkAttachOptions) (GTK_FILL|GTK_EXPAND|GTK_SHRINK), 0, 0);
	progress.ww_map_pbar = pbar;

	frame = gtk_frame_new("Runtime Status");
	gtk_box_pack_start(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	vbox2 = gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(frame),vbox2);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2),5);
	hbox2 = gtk_hbox_new(TRUE,3);
	gtk_box_pack_start(GTK_BOX(vbox2),hbox2,TRUE,TRUE,0);
	for (i=0;i<7;i++)
	{
		frame = gtk_frame_new(NULL);
		gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
		misc.ww_status[i] = gtk_label_new(status_msgs[i]);
		gtk_widget_set_sensitive(misc.ww_status[i],FALSE);
		gtk_container_add(GTK_CONTAINER(frame),misc.ww_status[i]);
		gtk_box_pack_start(GTK_BOX(hbox2),frame,TRUE,TRUE,0);
	}

	/* Commands box */
	frame = gtk_frame_new("Commands");
	gtk_box_pack_end(GTK_BOX(vbox),frame,FALSE,FALSE,0);

	table = gtk_table_new(1,4,TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(table),7);
	gtk_table_set_col_spacings(GTK_TABLE(table),5);
	gtk_container_set_border_width (GTK_CONTAINER (table), 5);
	gtk_container_add(GTK_CONTAINER(frame),table);

	button = gtk_button_new_with_label("Start Reading RT Vars");
	g_object_set_data(G_OBJECT(button),"handler",GINT_TO_POINTER(START_REALTIME));
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (std_button_handler),
			NULL);
	gtk_table_attach (GTK_TABLE (table), button, 0, 1, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	button = gtk_button_new_with_label("Stop Reading RT vars");
	g_object_set_data(G_OBJECT(button),"handler",GINT_TO_POINTER(STOP_REALTIME));
	g_signal_connect(G_OBJECT (button), "clicked",
			G_CALLBACK (std_button_handler),
			NULL);
	gtk_table_attach (GTK_TABLE (table), button, 1, 2, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);

	button = gtk_button_new_with_label("Get Data from ECU");
	gtk_tooltips_set_tip(tip,button,
			"Reads in the Constants and VEtable from the MegaSquirt ECU and populates the GUI",NULL);

	gtk_table_attach (GTK_TABLE (table), button, 2, 3, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);
	g_object_set_data(G_OBJECT(button),"handler",GINT_TO_POINTER(READ_VE_CONST));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(std_button_handler),
			NULL);

	button = gtk_button_new_with_label("Burn to ECU");
	store_controls = g_list_append(store_controls,(gpointer)button);
	gtk_tooltips_set_tip(tip,button,
			"Even though MegaTunix writes data to the MS as soon as its changed, it has only written it to the MegaSquirt's RAM, thus you need to select this to burn all variables to flash so on next power up things are as you set them.  We don't want to burn to flash with every variable change as there is the possibility of exceeding the max number of write cycles to the flash memory.", NULL);
	gtk_table_attach (GTK_TABLE (table), button, 3, 4, 0, 1,
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL),
			(GtkAttachOptions) (GTK_EXPAND|GTK_FILL), 0, 0);
	g_object_set_data(G_OBJECT(button),"handler",GINT_TO_POINTER(BURN_MS_FLASH));
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(std_button_handler),
			NULL);


	return;
}

void warmwizard_update_status(gfloat temp)
{
	extern gint temp_units;
	extern GdkColor red;
	extern GdkColor black;
	gboolean skipnext = FALSE;
	gint i = 0;
	gfloat F_temps[10] = 
	{-40.0,-20.0,0,20.0,40.0,60.0,80.0,100.0,130.0,160.0};
	gfloat C_temps[10] = 
	{-40,-28.8,-17.7,-6.6,4.4,15.5,26.6,37.7,54.4,71.1};
	gfloat *range;

	if (temp_units == FAHRENHEIT)	
		range = F_temps;
	else
		range = C_temps;

	for (i=0;i<10;i++)
	{
		if (skipnext == FALSE)
		{
			gtk_widget_modify_fg(labels.warmwizard_lab[i],
					GTK_STATE_NORMAL,&black);
			gtk_widget_modify_text(spinners.warmwizard[i],
					GTK_STATE_NORMAL,&black);
			gtk_widget_modify_fg(spinners.warmwizard[i],
					GTK_STATE_NORMAL,&black);
		}
		else
			skipnext = FALSE;
		if ((temp > range[i]) && (temp < range[i+1]))
		{
			skipnext = TRUE;
			gtk_widget_modify_fg(labels.warmwizard_lab[i],
					GTK_STATE_NORMAL,&red);
			gtk_widget_modify_text(spinners.warmwizard[i],
					GTK_STATE_NORMAL,&red);
			gtk_widget_modify_fg(spinners.warmwizard[i],
					GTK_STATE_NORMAL,&red);
			gtk_widget_modify_fg(labels.warmwizard_lab[i+1],
					GTK_STATE_NORMAL,&red);
			gtk_widget_modify_text(spinners.warmwizard[i+1],
					GTK_STATE_NORMAL,&red);
			gtk_widget_modify_fg(spinners.warmwizard[i+1],
					GTK_STATE_NORMAL,&red);
		}
	}

}

