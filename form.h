#define MAX 128
#define LONGMAX 2048

#define STR(x) #x
#define XSTR(x) STR(x)

#define INVALID_ID (-1)

#define TMP_UPLOAD_PATH "/tmp/cgi-uploads"

enum form_var_type {
    TYPE_FILE,
    TYPE_VALUE,
};

struct mimeheader_t {
    enum form_var_type type;
    const char *name;
    const char *filename;
    char *datap;
};

struct form_t {
    const char *field_names[MAX];
    const char *field_values[MAX];
    int num_fields;

    const char *file_names[MAX];
    size_t file_sizes[MAX];
    int num_files;

    int id;
};

struct error_t {
    const char *error_str;
    const char *error_field;
};

void fatal(const char *fmt, ...) __attribute__((format(printf,1,2))) __attribute((noreturn));
char *escape(const char *input);
char *shellescape(const char *input);
struct form_t parse_form(void);
const char *form_get_field(struct form_t *form, const char *name);
struct error_t *new_error(const char *field, const char *str);
int extract_query_string(struct form_t *form, const char *query_string);
