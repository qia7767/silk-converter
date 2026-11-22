#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
int silk_decoder_main(int argc, char **argv);
static gboolean run(char **args) {
    gint status = 0; GError *err = NULL;
    gboolean ok = g_spawn_sync(NULL, args, NULL, G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL, NULL, NULL, NULL, NULL, &status, &err);
    if (err) g_error_free(err);
    return ok && g_spawn_check_wait_status(status, NULL);
}
static gboolean convert(const char *input, const char *format, char **message) {
    gchar *dir = g_path_get_dirname(input), *base = g_path_get_basename(input), *dot = strrchr(base, '.');
    if (dot && dot != base) *dot = 0;
    gchar *name = g_strdup_printf("%s.%s", base, format), *out = g_build_filename(dir, name, NULL);
    GError *error = NULL; gchar *tmp = g_dir_make_tmp("silk-converter-XXXXXX", &error);
    gboolean ok = FALSE;
    if (!tmp) { *message = g_strdup(error->message); g_error_free(error); goto done; }
    gchar *pcm = g_build_filename(tmp, "audio.pcm", NULL), *result = g_strdup_printf("%s/output.%s", tmp, format);
    gchar self[PATH_MAX + 1]; ssize_t n = readlink("/proc/self/exe", self, PATH_MAX);
    if (n < 0) { *message = g_strdup("无法定位程序"); goto cleanup; } self[n] = 0;
    char *decoder[] = {self, "--decode", (char *)input, pcm, NULL};
    if (!run(decoder) || !g_file_test(pcm, G_FILE_TEST_IS_REGULAR)) { *message = g_strdup("SILK 解码失败"); goto cleanup; }
    GStatBuf st; if (g_stat(pcm, &st) || st.st_size == 0) { *message = g_strdup("没有解码出音频"); goto cleanup; }
    char *ffmpeg[] = {"ffmpeg", "-nostdin", "-v", "error", "-y", "-f", "s16le", "-ar", "24000", "-ac", "1", "-i", pcm, result, NULL};
    if (!run(ffmpeg)) { *message = g_strdup("FFmpeg 转换失败；请确认已安装 ffmpeg"); goto cleanup; }
    GFile *src = g_file_new_for_path(result), *dst = g_file_new_for_path(out);
    ok = g_file_copy(src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);
    if (!ok) { *message = g_strdup(error->message); g_error_free(error); }
    else *message = g_strdup(out);
    g_object_unref(src); g_object_unref(dst);
cleanup:
    g_unlink(pcm); g_unlink(result); g_rmdir(tmp); g_free(pcm); g_free(result); g_free(tmp);
done:
    g_free(dir); g_free(base); g_free(name); g_free(out); return ok;
}
static GtkWidget *list, *format;
static void add_path(const char *path) {
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        GError *error = NULL; GDir *dir = g_dir_open(path, 0, &error);
        if (!dir) { if (error) g_error_free(error); return; }
        const char *entry;
        while ((entry = g_dir_read_name(dir))) { gchar *child = g_build_filename(path, entry, NULL); if (g_file_test(child, G_FILE_TEST_IS_REGULAR)) gtk_list_box_insert(GTK_LIST_BOX(list), gtk_label_new(child), -1); g_free(child); }
        g_dir_close(dir);
    } else if (g_file_test(path, G_FILE_TEST_IS_REGULAR)) gtk_list_box_insert(GTK_LIST_BOX(list), gtk_label_new(path), -1);
    gtk_widget_show_all(list);
}
static void add_clicked(GtkButton *b, gpointer window) {
    gboolean folder = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "folder"));
    GtkWidget *dialog = gtk_file_chooser_dialog_new(folder ? "选择文件夹" : "选择 SILK 文件", GTK_WINDOW(window), folder ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN, "取消", GTK_RESPONSE_CANCEL, "添加", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), !folder);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        GSList *paths = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        for (GSList *p = paths; p; p = p->next) { add_path(p->data); g_free(p->data); }
        g_slist_free(paths);
    }
    gtk_widget_destroy(dialog);
}
static void drop_received(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data) {
    gchar **uris = gtk_selection_data_get_uris(data); gboolean any = FALSE;
    if (uris) { for (int i=0; uris[i]; i++) { gchar *path=g_filename_from_uri(uris[i],NULL,NULL); if(path) {add_path(path); g_free(path); any=TRUE;} } g_strfreev(uris); }
    gtk_drag_finish(context,any,FALSE,time);
}
static void clear_clicked(GtkButton *button, gpointer unused) {
    GList *rows=gtk_container_get_children(GTK_CONTAINER(list));
    for (GList *p=rows;p;p=p->next) gtk_widget_destroy(GTK_WIDGET(p->data));
    g_list_free(rows);
}
static void start_clicked(GtkButton *button, gpointer window) {
    gtk_widget_set_sensitive(GTK_WIDGET(button), FALSE); const char *fmt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(format));
    GList *rows = gtk_container_get_children(GTK_CONTAINER(list)); GString *summary = g_string_new(""); int ok=0, failed=0;
    for (GList *p=rows; p; p=p->next) {
        GtkWidget *label=gtk_bin_get_child(GTK_BIN(p->data)); const char *path=gtk_label_get_text(GTK_LABEL(label)); char *msg=NULL;
        if (convert(path,fmt,&msg)) ok++; else { failed++; g_string_append_printf(summary,"%s: %s\n",path,msg); }
        g_free(msg);
    }
    g_list_free(rows); gchar *headline=g_strdup_printf("成功 %d，失败 %d\n%s",ok,failed,summary->str);
    GtkWidget *dialog=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"%s",headline);
    gtk_dialog_run(GTK_DIALOG(dialog)); gtk_widget_destroy(dialog); g_free(headline); g_string_free(summary,TRUE); g_free((gpointer)fmt); gtk_widget_set_sensitive(GTK_WIDGET(button),TRUE);
}
int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1],"--decode") == 0) return silk_decoder_main(argc-1,argv+1);
    if (argc == 3) { if (strcmp(argv[2],"mp3") && strcmp(argv[2],"wav") && strcmp(argv[2],"ogg") && strcmp(argv[2],"flac")) { fprintf(stderr,"Unsupported format\n"); return 2; } char *msg=NULL; gboolean ok=convert(argv[1],argv[2],&msg); fprintf(ok?stdout:stderr,"%s\n",msg); g_free(msg); return ok?0:1; }
    gtk_init(&argc,&argv); GtkWidget *win=gtk_window_new(GTK_WINDOW_TOPLEVEL); gtk_window_set_title(GTK_WINDOW(win),"SILK 音频转换器"); gtk_window_set_default_size(GTK_WINDOW(win),650,400); g_signal_connect(win,"destroy",G_CALLBACK(gtk_main_quit),NULL);
    GtkWidget *box=gtk_box_new(GTK_ORIENTATION_VERTICAL,8), *bar=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8), *add=gtk_button_new_with_label("添加文件"), *folder=gtk_button_new_with_label("添加文件夹"), *clear=gtk_button_new_with_label("清空"), *start=gtk_button_new_with_label("开始转换"), *scroll=gtk_scrolled_window_new(NULL,NULL);
    format=gtk_combo_box_text_new(); const char *formats[]={"mp3","wav","ogg","flac"}; for (int i=0;i<4;i++) gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(format),formats[i]); gtk_combo_box_set_active(GTK_COMBO_BOX(format),0);
    list=gtk_list_box_new(); gtk_container_add(GTK_CONTAINER(scroll),list); gtk_box_pack_start(GTK_BOX(bar),add,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(bar),folder,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(bar),clear,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(bar),format,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(bar),start,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(box),bar,FALSE,FALSE,0); gtk_box_pack_start(GTK_BOX(box),scroll,TRUE,TRUE,0); gtk_container_add(GTK_CONTAINER(win),box);
    g_object_set_data(G_OBJECT(folder),"folder",GINT_TO_POINTER(1)); g_signal_connect(add,"clicked",G_CALLBACK(add_clicked),win); g_signal_connect(folder,"clicked",G_CALLBACK(add_clicked),win); g_signal_connect(clear,"clicked",G_CALLBACK(clear_clicked),NULL); GtkTargetEntry targets[] = {{"text/uri-list",0,0}}; gtk_drag_dest_set(win,GTK_DEST_DEFAULT_ALL,targets,1,GDK_ACTION_COPY); g_signal_connect(win,"drag-data-received",G_CALLBACK(drop_received),NULL); g_signal_connect(start,"clicked",G_CALLBACK(start_clicked),win); gtk_widget_show_all(win); gtk_main(); return 0;
}
