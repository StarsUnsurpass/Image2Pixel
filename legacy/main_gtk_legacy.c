#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

GtkWidget *window;
GtkWidget *image_display;
GtkWidget *status_label;
GdkPixbuf *original_pixbuf = NULL;
GdkPixbuf *processed_pixbuf = NULL;
int pixel_block_size = 10;

// Function to limit values
unsigned char clamp_val(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

// Core pixelation logic
void apply_pixelation(GdkPixbuf *pixbuf, int p_size) {
    if (p_size <= 1) return;

    int width = gdk_pixbuf_get_width(pixbuf);
    int height = gdk_pixbuf_get_height(pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels(pixbuf);
    int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);

    for (int y = 0; y < height; y += p_size) {
        for (int x = 0; x < width; x += p_size) {
            
            long sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0;
            int count = 0;

            // Calculate average
            for (int by = 0; by < p_size; ++by) {
                if (y + by >= height) break;
                guchar *row_ptr = pixels + (y + by) * rowstride;
                
                for (int bx = 0; bx < p_size; ++bx) {
                    if (x + bx >= width) break;
                    
                    guchar *p = row_ptr + (x + bx) * n_channels;
                    sum_r += p[0];
                    sum_g += p[1];
                    sum_b += p[2];
                    if (n_channels == 4) sum_a += p[3];
                    count++;
                }
            }

            if (count == 0) continue;

            unsigned char avg_r = sum_r / count;
            unsigned char avg_g = sum_g / count;
            unsigned char avg_b = sum_b / count;
            unsigned char avg_a = (n_channels == 4) ? (sum_a / count) : 255;

            // Fill block
            for (int by = 0; by < p_size; ++by) {
                if (y + by >= height) break;
                guchar *row_ptr = pixels + (y + by) * rowstride;
                
                for (int bx = 0; bx < p_size; ++bx) {
                    if (x + bx >= width) break;
                    
                    guchar *p = row_ptr + (x + bx) * n_channels;
                    p[0] = avg_r;
                    p[1] = avg_g;
                    p[2] = avg_b;
                    if (n_channels == 4) p[3] = avg_a;
                }
            }
        }
    }
}

void update_preview() {
    if (!original_pixbuf) return;

    if (processed_pixbuf) {
        g_object_unref(processed_pixbuf);
    }
    
    // Copy original to processed
    processed_pixbuf = gdk_pixbuf_copy(original_pixbuf);
    
    // Apply effect
    apply_pixelation(processed_pixbuf, pixel_block_size);
    
    // Update UI
    gtk_image_set_from_pixbuf(GTK_IMAGE(image_display), processed_pixbuf);
}

void on_open_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Open Image",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Open", GTK_RESPONSE_ACCEPT,
                                         NULL);

    // Add filters
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Images");
    gtk_file_filter_add_mime_type(filter, "image/png");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_filter_add_mime_type(filter, "image/bmp");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        
        GError *err = NULL;
        if (original_pixbuf) g_object_unref(original_pixbuf);
        
        original_pixbuf = gdk_pixbuf_new_from_file(filename, &err);
        
        if (!original_pixbuf) {
            gtk_label_set_text(GTK_LABEL(status_label), "Error loading image.");
            g_error_free(err);
        } else {
            // Resize if too big for screen (optional, simplified for now)
            // But let's keep it 1:1 for pixel accuracy or let ScrolledWindow handle it.
            update_preview();
            gtk_label_set_text(GTK_LABEL(status_label), filename);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_save_clicked(GtkWidget *widget, gpointer data) {
    if (!processed_pixbuf) return;

    GtkWidget *dialog;
    dialog = gtk_file_chooser_dialog_new("Save Image",
                                         GTK_WINDOW(window),
                                         GTK_FILE_CHOOSER_ACTION_SAVE,
                                         "_Cancel", GTK_RESPONSE_CANCEL,
                                         "_Save", GTK_RESPONSE_ACCEPT,
                                         NULL);
                                         
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "pixelated_image.png");

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename = gtk_file_chooser_get_filename(chooser);
        
        GError *err = NULL;
        gboolean success = gdk_pixbuf_save(processed_pixbuf, filename, "png", &err, NULL);
        
        if (success) {
            gtk_label_set_text(GTK_LABEL(status_label), "Saved successfully.");
        } else {
            gtk_label_set_text(GTK_LABEL(status_label), "Error saving file.");
            g_error_free(err);
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

void on_scale_changed(GtkRange *range, gpointer data) {
    int val = (int)gtk_range_get_value(range);
    if (val != pixel_block_size) {
        pixel_block_size = val;
        update_preview();
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    // Main Window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "image2pixel");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Layout Containers
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Toolbar / Header area
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 10);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    // Buttons
    GtkWidget *btn_open = gtk_button_new_with_label("Open Image");
    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 0);

    GtkWidget *btn_save = gtk_button_new_with_label("Save Image");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_save, FALSE, FALSE, 0);

    // Scale (Slider)
    GtkWidget *label_scale = gtk_label_new("Pixel Size:");
    gtk_box_pack_start(GTK_BOX(toolbar), label_scale, FALSE, FALSE, 0);

    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 100, 1);
    gtk_range_set_value(GTK_RANGE(scale), 10);
    gtk_widget_set_size_request(scale, 200, -1);
    g_signal_connect(scale, "value-changed", G_CALLBACK(on_scale_changed), NULL);
    gtk_box_pack_start(GTK_BOX(toolbar), scale, FALSE, FALSE, 0);

    // Status Label
    status_label = gtk_label_new("Welcome! Open an image to start.");
    gtk_box_pack_end(GTK_BOX(toolbar), status_label, FALSE, FALSE, 0);

    // Image Area
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    image_display = gtk_image_new();
    gtk_container_add(GTK_CONTAINER(scrolled_window), image_display);

    // CSS Styling (Optional - make it look a bit nicer)
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window { background-color: #f0f0f0; }"
        "button { padding: 5px 10px; }"
        , -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}