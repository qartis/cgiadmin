#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <sqlite3.h>
#include <openssl/md5.h>

#include "form.h"
#include "digest.h"
#include "html.h"

void segv();

enum page_type {
    GET,
    POST
};

const char *user = "sup";
const char *pass = "bro";

#define BASE_URL "http://admin.nghdfilm.com"

sqlite3 *db;

struct page_t {
    const char *uri;
    void (*func)(struct form_t *);
    enum page_type type;
};

void main_menu(struct form_t *form){
    (void)form;
    html_h1("Main menu");
    html_ul_begin();
        html_li();
            html_a_begin("/browse-talent");
            printf("Browse talent");
            html_a_end();
        html_li();
            html_a_begin("/browse-videos");
            printf("Browse videos");
            html_a_end();
        html_li();
            html_a_begin("/vars");
            printf("Vars");
            html_a_end();
    html_ul_end();
}

__attribute__((format(printf,1,2))) void redirect(const char *fmt, ...){
    va_list ap;
    char buf[MAX];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("HTTP/1.1 303 See Other\r\n");
    printf("Location: " BASE_URL "%s\r\n", buf);
    printf("\r\n");
}

void show_environ(struct form_t *form){
    html_h1("Environment Variables");
    char **envp = __environ;
    while(*envp){
        printf("%s<br>", *envp);
        envp++;
    }
    html_h1("Form variables");
    int i; 
    for(i=0;i<form->num_fields;i++){
        printf("%d: %s = '%s'<br>\r\n", i, form->field_names[i], form->field_values[i]);
    }
    for(i=0;i<form->num_files;i++){
        printf("File %d: '%s' (%zu bytes)<br>\r\n", i, form->file_names[i], form->file_sizes[i]);
    }
    printf("Done");
}

void dispatch(struct page_t *pages, int num_pages){
    char *request_uri = getenv("REQUEST_URI");
    char *endp = strchr(request_uri, '?');
    if (endp){
        *endp = '\0';
    }

    struct form_t form = parse_form();

    int i;
    for(i=0;i<num_pages;i++){
        if (strcmp(request_uri, pages[i].uri) == 0){
            if (pages[i].type == GET){
                status("200 OK");
                printf("\r\n");
                main_menu(&form);
            }
            pages[i].func(&form);
            return;
        }
    }

    status("200 OK");
    printf("\r\n");
    printf("No such handler '%s'<br>\n", request_uri);
    show_environ(&form);
}

void debug_sqlite_table(int nrows, int ncols, char **result){
    printf("nrows %d ncols %d\n", nrows, ncols);
    int i;
    for(i=ncols;i<(nrows+1)*ncols;i++){
        printf("result[%d] = '%s'\n", i, result[i]);
    }
}

int sqlite_table(const char *query, char ***result, int *nrows, int *ncols){
    int rc = sqlite3_get_table(db, query, result, nrows, ncols, NULL);
    if (rc){
        fatal("database error: %s\n", sqlite3_errmsg(db));
    }
    return rc;
}

int talent_num_photos(int id){
    char **result;
    int nrows, ncols;
    char *query = sqlite3_mprintf("select count(*) from talent_photos where id = %d", id);
    sqlite_table(query, &result, &nrows, &ncols);
    int num_photos = atoi(result[1]);
    sqlite3_free_table(result);
    sqlite3_free(query);
    return num_photos;
}

void talent_photos(struct form_t *form){
    int i;
    char **result;
    int nrows, ncols;
    int id = form->id;

    if (id == INVALID_ID){
        printf("invalid id\n");
        return;
    }
    char *query = sqlite3_mprintf("select filename,rowid from talent_photos where id = %d", id);
    sqlite_table(query, &result, &nrows, &ncols);
    html_ul_begin();
    for(i=ncols; i < (nrows + 1) * ncols; i += ncols){
        html_li();
        printf("%s", result[i]);
        html_a_begin("/photos/%s", result[i]);
        html_img_thumb("/photos/%s", result[i]);
        html_a_end();
        html_form_begin("POST", "/delete-photo");
        html_input_hidden("id", "%s", result[i+1]);
        html_input_hidden("talent_id", "%d", id);
        html_input_submit_confirm("Delete", "Are you sure you want to delete this photo?");
        html_form_end();
    }

    html_form_begin("POST", "/add-photo");
        html_input_hidden("id", "%d", form->id);
        html_input_file("new_photo");
        html_input_submit("Add Photo");
    html_form_end();

    sqlite3_free_table(result);
    sqlite3_free(query);
}

void browse_talent(struct form_t *form){
    (void)form;
    html_h1("Talent list");
    char **result;
    int nrows, ncols;

    sqlite_table("select name,rowid from talent", &result, &nrows, &ncols);
    html_a_begin("/new-talent");
    printf("Create new talent");
    html_a_end();
    int i;
    html_ul_begin();
    for(i=ncols;i<(nrows+1)*ncols;i+=ncols){
        html_li();
        html_a_begin("/edit-talent?id=%s", result[i+1]);
        printf("%s", result[i]);
        html_a_end();
        int id = atoi(result[i+1]);
        html_a_begin("/talent-photos?id=%s", result[i+1]);
        printf("(%d photos)", talent_num_photos(id));
        html_a_end();
    }

    sqlite3_free_table(result);
}

void browse_videos(struct form_t *form){
    (void)form;
    html_h1("Video list");
    char **result;
    int nrows, ncols;
    
    html_form_begin("POST", "/save-video");
    html_h2("New Video");
    html_input_text("url", "http://www.tudou.com/programs/view/6HFJRsg5FS8/");
    html_input_submit("Upload");
    html_form_end();

    sqlite_table("select name,url,img,rowid from videos order by rowid desc", &result, &nrows, &ncols);
    int i;
    html_ul_begin();
    for(i=ncols;i<(nrows+1)*ncols;i+=ncols){
        html_li();
        html_a_begin("%s", result[i+1]);
            printf("%s<br>", escape(result[i]));
            html_img("%s", result[i+2]);
        html_a_end();

        html_form_begin("POST", "/delete-video");
        html_input_hidden("id", "%s", result[i+3]);
        html_input_submit_confirm("Delete", "Are you sure you want to delete this video?");
        html_form_end();
    }

    sqlite3_free_table(result);
}

size_t curl_write(char *ptr, size_t size, size_t nmemb, void *userdata){
    if (userdata == NULL){
        return size * nmemb;
    } else {
        if (strncmp(ptr, "Location: ", strlen("Location: ")) == 0){
            char *buf = userdata;
            memcpy(buf, ptr, size * nmemb);
            buf[size * nmemb] = '\0';
        }
        return size * nmemb;
    }
}

char *tudou_swf_id(const char *url){
    const char *prefix = "programs/view/";
    char *start = strstr(url, prefix);
    if (!start){
        return NULL;
    }
    start += strlen(prefix);
    char *end = strchr(start, '/');
    if (!end){
        return NULL;
    }
    return strndup(start, (size_t)(end-start));
}

char *get_location_header(const char *url){
    char *buf = malloc(CURL_MAX_HTTP_HEADER);
    buf[0] = '\0';
    int retries = 0;
    CURLcode rc;
    CURL *easy = curl_easy_init();
    do {
        curl_easy_setopt(easy, CURLOPT_URL, url);
        curl_easy_setopt(easy, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, curl_write);
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curl_write);
        curl_easy_setopt(easy, CURLOPT_HEADERDATA, buf);
        curl_easy_setopt(easy, CURLOPT_WRITEDATA, NULL);
        curl_easy_setopt(easy, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(easy, CURLOPT_STDERR, stdout);
        curl_easy_setopt(easy, CURLOPT_USERAGENT, "Mozilla/5.0 (iPad; CPU OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3");

        rc = curl_easy_perform(easy);
    } while (rc == CURLE_RECV_ERROR && retries++ < 5);

    if (rc != 0){
        fatal("curl error: %d\n", rc);
    }

    return buf;
}

void save_video(struct form_t *form){
    char **result;
    int nrows, ncols;

    const char *url = form_get_field(form, "url");

    char *id = tudou_swf_id(url);
    if (!id){
        printf("invalid url format\n");
        return;
    }

    char proper_url[MAX];
    snprintf(proper_url, sizeof(proper_url), "http://www.tudou.com/v/%s/v.swf", id);

    char *location = get_location_header(proper_url);

    char *query_string = strchr(location, '?');
    if (!query_string){
        fatal("didn't get any redirectfrom tudou\n");
    }
    query_string++;

    extract_query_string(form, query_string);
    const char *title = form_get_field(form, "title");
    const char *snap_pic = form_get_field(form, "snap_pic");

    char *query = sqlite3_mprintf("insert into videos values (%Q,%Q,%Q)", title, id, snap_pic);
    sqlite_table(query, &result, &nrows, &ncols);

    redirect("/browse-videos");

    sqlite3_free_table(result);
    sqlite3_free(query);
    free(location);
}

void edit_talent(struct form_t *form){
    char **result;
    int nrows, ncols;
    char *name = strdup("");
    char *info = strdup("");

    html_form_begin("POST", "/save-talent");
    
    int id = form->id;

    if (id == INVALID_ID){
        html_h1("New talent");
    } else {
        html_h1("Edit talent");
        html_input_hidden("id", "%d", id);
        char *query = sqlite3_mprintf("select * from talent where rowid = '%d'", id);
        sqlite_table(query, &result, &nrows, &ncols);
        if (nrows == 0){
            printf("no such id\n");
            return;
        }
        sqlite3_free_table(result);
        sqlite3_free(query);
        name = escape(result[2]);
        info = escape(result[3]);
    }

    printf("Name: ");
        html_input_text("name", "%s", name);
    html_br();
    printf("Info: ");
        html_input_textarea("info", "%s", info);
    html_input_submit("Save");
    html_form_end();

    html_form_begin("POST", "/delete-talent");
    html_input_hidden("id", "%d", id);
    if (id != INVALID_ID){
        html_input_submit_confirm("Delete", "Are you sure you want to delete this talent?");
    }
    html_form_end();

    free(info);
    free(name);
    sqlite3_free_table(result);
}

void save_talent(struct form_t *form){
    char **result;
    int nrows, ncols;
    
    const char *name = form_get_field(form, "name");
    const char *info = form_get_field(form, "info");

    char *query;
    if (form->id == INVALID_ID){
        query = sqlite3_mprintf("insert into talent values (%Q,%Q)", name, info);
    } else {
        query = sqlite3_mprintf("update talent set name = %Q, info = %Q where rowid = '%u'", name, info, form->id);
    }

    sqlite_table(query, &result, &nrows, &ncols);

    redirect("/browse-talent");
    sqlite3_free_table(result);
    sqlite3_free(query);
}

void delete_photo(struct form_t *form){
    char **result;
    int nrows, ncols;

    if (form->id == INVALID_ID){
        printf("invalid id\n");
        return;
    }

    char *query = sqlite3_mprintf("delete from talent_photos where rowid = %d", form->id);
    sqlite_table(query, &result, &nrows, &ncols);

    const char *talent_id = form_get_field(form, "talent_id");

    redirect("/talent-photos?id=%s", talent_id);

    sqlite3_free_table(result);
    sqlite3_free(query);
}

void delete_video(struct form_t *form){
    char **result;
    int nrows, ncols;

    if (form->id == INVALID_ID){
        printf("invalid id\n");
        return;
    }

    char *query = sqlite3_mprintf("delete from videos where rowid = %d", form->id);
    sqlite_table(query, &result, &nrows, &ncols);

    redirect("/browse-videos");

    sqlite3_free_table(result);
    sqlite3_free(query);
}

void delete_talent(struct form_t *form){
    char **result;
    int nrows, ncols;

    if (form->id == INVALID_ID){
        printf("invalid id\n");
        return;
    }

    char *query = sqlite3_mprintf("delete from talent where rowid = %d", form->id);
    sqlite_table(query, &result, &nrows, &ncols);

    redirect("/browse-talent");

    sqlite3_free_table(result);
    sqlite3_free(query);
}

const char *file_ext(const char *filename){
    char *dot = strrchr(filename, '.');
    if (!dot){
        return NULL;
    }
    return dot + 1;
}

void add_photo(struct form_t *form){
    char **result;
    int nrows, ncols;

    if (form->id == INVALID_ID){
        printf("invalid id\n");
        return;
    }

    if (form->num_files != 1){
        fatal("invalid number of files: %d\n", form->num_files);
    }

    const char *ext = file_ext(form->file_names[0]);
    if (!ext || (strcasecmp(ext, "jpg") && strcasecmp(ext, "jpeg"))){
        fatal("invalid file type '%s'\n", ext);
    }

    char old[MAX];
    char new[MAX];
    snprintf(old, sizeof(old), TMP_UPLOAD_PATH "/%s", form->file_names[0]);
    snprintf(new, sizeof(new), "photos/%s", form->file_names[0]);
    int rc = rename(old, new);
    if (rc){
        fatal("error renaming: %s\n", strerror(errno));
    }
    char *query = sqlite3_mprintf("insert into talent_photos values (%d, %Q)", form->id, form->file_names[0]);
    sqlite_table(query, &result, &nrows, &ncols);

    redirect("/talent-photos?id=%d", form->id);

    sqlite3_free_table(result);
    sqlite3_free(query);
}

int main(){
    setbuf(stdout, NULL);
    signal(SIGSEGV, segv);
    struct page_t pages[] = {
        {"/",                 main_menu,          GET},
        {"/vars",             show_environ,       GET},
        {"/browse-talent",    browse_talent,      GET},
        {"/edit-talent",      edit_talent,        GET},
        {"/new-talent",       edit_talent,        GET},
        {"/save-talent",      save_talent,        POST},
        {"/delete-talent",    delete_talent,      POST},
        {"/talent-photos",    talent_photos,      GET},
        {"/delete-photo",     delete_photo,       POST},
        {"/add-photo",        add_photo,          POST},
        {"/browse-videos",    browse_videos,      GET},
        {"/save-video",       save_video,         POST},
        {"/delete-video",     delete_video,       POST},
    };
//add, new, save, why 3 different words?

    int rc = sqlite3_open(".db", &db);
    if (rc){
        fatal("db error: %s\n", sqlite3_errmsg(db));
    }

    if (checkauth(user, pass)){
        sleep(1);
        print_auth_header();
        printf("\r\n");
        html_h1("Password required");
    } else {
        dispatch(pages, sizeof(pages) / sizeof(pages[0]));
    }

    sqlite3_close(db);
    return 0;
}

