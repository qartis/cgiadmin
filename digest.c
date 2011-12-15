#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#include <openssl/md5.h>

char *extract_value(const char *haystack, const char *needle){
    char *startp = strstr(haystack, needle);
    if (!startp){
        return NULL;
    }
    startp += strlen(needle);
    char *endp;
    if (*startp == '"'){
        startp++;
        endp = strchr(startp, '"');
        if (!endp){
            return NULL;
        }
    } else {
        endp = strpbrk(startp, ",; \r\n");
        if (!endp){
            endp = strchr(startp, '\0');
        }
    }
    size_t len = (size_t)(endp - startp);
    return strndup(startp, len);
}

char from_hex(char a){
    if (a >= 'A' && a <= 'F'){
        return (char)(a - 'A' + 10);
    } else if (a >= 'a' && a <= 'f'){
        return (char)(a - 'a' + 10);
    } else {
        return (char)(a - '0');
    }
}

int checkauth(const char *user, const char *pass){
    int i;
    unsigned char a1[MD5_DIGEST_LENGTH];
    unsigned char a2[MD5_DIGEST_LENGTH];
    unsigned char a3[MD5_DIGEST_LENGTH];
    char buf[256];
    const char *auth = getenv("HTTP_AUTHORIZATION");
    if (!auth){
        return -1;
    }
    const char *request_uri = getenv("REQUEST_URI");
    if (request_uri && strlen(request_uri) > sizeof(buf)/2){
        printf("URL too long\n");
        return -1;
    }
    const char *username = extract_value(auth, "username=");
    const char *nonce = extract_value(auth, "nonce=");
    const char *response = extract_value(auth, "response=");
    const char *uri = extract_value(auth, "uri=");
    const char *qop = extract_value(auth, "qop=");
    const char *nc = extract_value(auth, "nc=");
    const char *cnonce = extract_value(auth, "cnonce=");
    const char *realm = extract_value(auth, "realm=");
    const char *method = getenv("REQUEST_METHOD");
    const char *remote_addr = getenv("REMOTE_ADDR");
    if (!username || !nonce || !response || !uri || !qop || !nc || !cnonce || !realm){
        return -1;
    }
    snprintf(buf, sizeof(buf), "%s:%s:%s", user, realm, pass);
    MD5((unsigned char *)buf, strlen(buf), a1);
    snprintf(buf, sizeof(buf), "%s:%s", method, uri);
    MD5((unsigned char *)buf, strlen(buf), a2);
    buf[0] = '\0';
    for(i=0;i<MD5_DIGEST_LENGTH;i++){
        snprintf(buf+strlen(buf), sizeof(buf) - strlen(buf), "%02x", a1[i]);
    }
    snprintf(buf+strlen(buf), sizeof(buf), ":%s:%s:%s:%s:", nonce, nc, cnonce, qop);
    for(i=0;i<MD5_DIGEST_LENGTH;i++){
        snprintf(buf+strlen(buf), sizeof(buf) - strlen(buf), "%02x", a2[i]);
    }
    MD5((unsigned char *)buf, strlen(buf), a3);
    for(i=0;i<MD5_DIGEST_LENGTH;i++){
        unsigned char byte = (unsigned char )((from_hex(response[i*2]) << 4) |
                                              (from_hex(response[i*2+1])));
        if (byte != a3[i]){
            return -1;
        }
    }
    char *nonce_remote_addr = strchr(nonce, '-');
    if (!nonce_remote_addr || strcmp(nonce_remote_addr + 1, remote_addr)){
        return -1;
    }
    return 0;
}

void print_nonce(const char *remote_addr){
    uuid_t uuid;
    uuid_generate(uuid);
    int i;
    for(i=0;i<16;i++){
        printf("%02x", uuid[i]);
    }
    printf("-");
    printf("%s", remote_addr);
} 

void status(const char *str){
    printf("HTTP/1.1 %s\r\n", str);
    printf("Content-type: text/html; charset=utf-8\r\n");
}

void print_auth_header(){
    char *remote_addr = getenv("REMOTE_ADDR");
    status("401 Unauthorized");
    printf("WWW-Authenticate: Digest realm=\"nghdfilm.com\", algorithm=MD5, nonce=\"");
    print_nonce(remote_addr);
    printf("\", qop=auth\r\n");
}
