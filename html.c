#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "form.h"

void html_form_begin(const char *method, const char *action){
    printf("<form action='%s' method='%s' enctype='multipart/form-data'>\r\n", action, method);
}

__attribute__((format(printf,2,3))) void html_input_text(const char *name, const char *fmt, ...){
    char buf[MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<input type=text size=50 name='%s' value='%s' />\r\n", name, escape(buf));
}


void html_input_submit(const char *str){
    printf("<input type=submit value='%s' />\r\n", str);
}

__attribute__((format(printf,2,3))) void html_input_hidden(const char *name, const char *fmt, ...){
    char buf[MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<input type=hidden name='%s' value='%s' />\r\n", name, escape(buf));
}

void html_form_end(void){
    printf("</form>\r\n");
}

void html_h1(const char *str){
    printf("<h1>%s</h1>\r\n", escape(str));
}

void html_h2(const char *str){
    printf("<h2>%s</h2>\r\n", escape(str));
}

void html_ul_begin(void){
    printf("<ul>\r\n");
}

void html_ul_end(void){
    printf("</ul>\r\n");
}

void html_li(void){
    printf("<li>");
}

__attribute__((format(printf,1,2))) void html_a_begin(const char *fmt, ...){
    char buf[MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<a href='%s'>", escape(buf));
}

void html_a_end(void){
    printf("</a>\r\n");
}

__attribute__((format(printf,1,2))) void html_img(const char *fmt, ...){
    char buf[MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<img src='%s' />", escape(buf));
}

void html_input_file(const char *name){
    printf("<input type=file name='%s'>\n", name);
}

__attribute__((format(printf,1,2))) void html_img_thumb(const char *fmt, ...){
    char buf[MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<img width=100 height=100 src='%s' />", escape(buf));
}

__attribute__((format(printf,2,3))) void html_input_textarea(const char *name, const char *fmt, ...){
    char buf[LONGMAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("<textarea name=%s rows=10 cols=30>%s</textarea>\r\n", name, escape(buf));
}

void html_br(void){
    printf("<br/>\r\n");
}

void html_input_submit_confirm(const char *text, const char *msg){
    printf("<input type=submit value=%s onClick='return confirm(\"%s\");'>\r\n", text, escape(msg));
}
