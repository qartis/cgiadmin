#define _GNU_SOURCE
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "form.h"
#include "digest.h"

__attribute__((format(printf,1,2))) __attribute__((noreturn)) void fatal(const char *fmt, ...){
    va_list ap;
    char buf[MAX];
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("HTTP/1.1 200 OK\r\n");
    printf("\r\n");
    printf("fatal error: %s\n", buf);
    exit(69);
}

char *shellescape(const char *input){
    char *buf = malloc(strlen(input) * 2);
    char *cur = buf;
    while (*input){
        if (*input == '\''){
            strcpy(cur, "'\\''");
            cur += strlen("'\\''");
        } else {
            *cur = *input;
            cur++;
        }
        input++;
    }
    *cur = '\0';
    return buf;
}


char *escape(const char *input){
    char *buf = malloc(strlen(input) * 8);
    char *cur = buf;
    while(*input){
        const char *replace = NULL;
        switch (*input){
        case '&':
            replace = "&amp;";
            break;
        case '<':
            replace = "&lt;";
            break;
        case '>':
            replace = "&gt;";
            break;
        case '\'':
            replace = "&#039;";
            break;
        case '"':
            replace = "&quot;";
            break;
        default:
            break;
        }
        if (replace){
            strcpy(cur, replace);
            cur += strlen(replace);
            input++;
        } else {
            *cur++ = *input++;
        }
    }
    *cur = '\0';
    return buf;
}

int startswith(const char *a, const char *b){
    return strncmp(a, b, strlen(b)) == 0;
}

size_t fread_loop(void *ptr, size_t size, size_t nmemb, FILE *stream){
    size_t total = 0;
    size_t readb;
    do {
        readb = fread((char *)ptr + total,
        size - total,
        nmemb,
        stream);
        total += readb;
    } while (readb > 0 && total < size);
    return total;
}

char *grabline(char **data){
    char *nl = strstr(*data, "\r\n");
    if (!nl){
        return NULL;
    }
    *nl = '\0';
    char *line = *data;
    *data = nl + strlen("\r\n");
    return line;
}

int startswithcase(const char *a, const char *b){
    return strncasecmp(a, b, strlen(b)) == 0;
}

int save_file(const char *filename, const char *data, size_t len){
    char buf[MAX];
    
    sprintf(buf, TMP_UPLOAD_PATH "/%." XSTR(MAX) "s", filename);

    FILE *f = fopen(buf, "wb");
    if (!f){
        return -1;
    }

    size_t wrote;
    do {
        wrote = fwrite(data, 1, len, f);
        len -= wrote;
        data += wrote;
    } while (wrote > 0 && len > 0);

    fclose(f);

    return 0;
}

void extract_content_disposition(char *line, struct mimeheader_t *header){
    if (!startswith(line, "form-data;")){
        return;
    }
    char *name = extract_value(line, "name=");
    if (!name){
        return;
    }
    header->name = name;
    char *filename = extract_value(line, "filename=");
    if (filename){
        header->type = TYPE_FILE;
        header->filename = filename;
    } else {
        header->type = TYPE_VALUE;
    }
}

struct mimeheader_t parse_chunk_headers(char *chunk){
    struct mimeheader_t header = {0,0,0,0};
    while(*chunk){
        char *line = grabline(&chunk);
        if (!line){
            break;
        } else if (*line == '\0'){
            header.datap = line + strlen("\r\n");
            break;
        } else if (startswithcase(line, "content-disposition:")){
            line += strlen("content-disposition:");
            if (*line == ' '){
                line++;
            }
            extract_content_disposition(line, &header);
        }
    }
    return header;
}

size_t get_content_len(){
    char *content_len = getenv("HTTP_CONTENT_LENGTH");
    if (!content_len){
        return 0;
    }

    int len = atoi(content_len);
    if (len > 10*1000*1000){
        fatal("content too big (%d), bailing\n", len);
    }

    return (size_t)len;
}

void add_var_to_form(struct form_t *form, const char *name, const char *value){
    int i;
    for(i=0;i<form->num_fields;i++){
        if (strcmp(form->field_names[i], name) == 0){
            fatal("DUPLICATE variable '%s'\n", name);
        }
    }

    form->field_names[form->num_fields] = name;
    form->field_values[form->num_fields] = value;
    form->num_fields++;

    if (strcmp(name, "id") == 0 && value){
        int id = atoi(value);
        if (id < 0){
            id = -1;
        }
        form->id = id;
    }
}

int parse_chunk_data(struct mimeheader_t *header, struct form_t *form, size_t len){
    switch (header->type){
    case TYPE_FILE:
        if (save_file(header->filename, header->datap, len) == -1){
            fatal("error saving file: %s\n", strerror(errno));
        }
        form->file_names[form->num_files] = header->filename;
        form->file_sizes[form->num_files] = len;
        form->num_files++;
        break;
    case TYPE_VALUE:
        if (len > 10 * 1000){
            fatal("variable too big, aborting\nvarname: %s, size %zu bytes", header->name, len);
        }
        add_var_to_form(form, header->name, strndup(header->datap, len));
        break;
    default:
        break;
    }
    return 0;
}

char *urldecode(const char *input){
    char *nl = strchr(input, '\n');
    if (nl){
        *nl = '\0';
    }
    char *buf = malloc(strlen(input));
    char *cur = buf;
    while (*input){
        if (input[0] == '%' &&
                isxdigit(input[1]) &&
                isxdigit(input[2])){
            int digit;
            sscanf(input, "%%%02x", &digit);
            *cur = (char)digit;
            cur++;
            input += 3;
        } else {
            *cur = *input;
            cur++;
            input++;
        }
    }
    *cur = '\0';
    return buf;
}

int extract_query_string(struct form_t *form, const char *query_string){
    int count = 0;
    if (!query_string){
        return count;
    }
    query_string = urldecode(query_string);
    while (query_string && *query_string){
        const char *name;
        const char *value;
        char *eq = strchr(query_string, '=');
        if (eq){
            name = strndup(query_string, (size_t)(eq - query_string));
            query_string = eq + 1;
            char *amp = strchr(query_string, '&');
            if (amp){
                value = strndup(query_string, (size_t)(amp - query_string));
                query_string = amp + 1;
            } else {
                value = strdup(query_string);
                query_string = strchr(query_string, '\0');
            }
        } else {
            name = strdup(query_string);
            value = NULL;
            query_string = strchr(query_string, '\0');
        }
        if (strlen(name) > 0){
            add_var_to_form(form, name, value);
            count++;
        }
    }
    return count;
}

void parse_multipart_form(struct form_t *form, char *post, size_t content_len, const char *boundary){
    char *start = post;
    while (post && post[0] == '-' && post[1] == '-' && startswith(post+2, boundary)){
        post += strlen("--") + strlen(boundary) + strlen("\r\n");
        struct mimeheader_t header = parse_chunk_headers(post);
        char *endp = memmem(header.datap, content_len - (size_t)(header.datap - start), boundary, strlen(boundary));
        if (!endp){
            break;
        }
        size_t obj_len = (size_t)(endp - header.datap) - strlen("--") - strlen("\r\n");
        if (obj_len){
            if (parse_chunk_data(&header, form, obj_len) == -1){
                fatal("error parsing data for chunk\n");
            }
        }
        post = endp - strlen("--");
    }
}

struct form_t parse_form(){
    struct form_t form = {{0},{0},0,{0},{0},0,-1};

    const char *query_string = getenv("QUERY_STRING");
    extract_query_string(&form, query_string);

    size_t content_len = get_content_len();
    if (content_len < 1){
        return form;
    }
    char *buf = malloc(content_len);
    size_t readb = fread_loop(buf, 1, content_len, stdin);
    if (readb < content_len){
        fatal("problem parsing POST data (read %zu of %zu)\n", readb, content_len);
    } 
    char *content_type = getenv("CONTENT_TYPE");
    if (!content_type){
        free(buf);
        return form;
    }
    if (*content_type == ' '){
        content_type++;
    }
    if (!startswith(content_type, "multipart/form-data;")){
        fatal("form has unknown content type %s", content_type);
    }
    char *boundary = extract_value(content_type, "boundary=");
    parse_multipart_form(&form, buf, content_len, boundary);
    free(buf);
    return form;
}

const char *form_get_field(struct form_t *form, const char *name){
    if (!form){
        fatal("form doesn't have required field %s", name);
    }
    int i;
    for(i=0;i<form->num_fields;i++){
        if (strcmp(form->field_names[i], name) == 0){
            return form->field_values[i];
        }
    }
    fatal("form doesn't have required field %s", name);
}