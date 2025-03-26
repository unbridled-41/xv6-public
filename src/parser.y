/**
 * @file parser.y
 * @brief Grammar for HTTP
 */

%{
#include "parse.h"

/* Debug */
#define YACCDEBUG
#ifdef YACCDEBUG
#include <stdio.h>
#define YPRINTF(...) printf(__VA_ARGS__)
#else
#define YPRINTF(...)
#endif

/* yyparse() calls yyerror() on error */
void yyerror(const char *s);

void set_parsing_options(char *buf, size_t siz, Request *parsing_request);

/* Input from buffer */
char *parsing_buf;
int parsing_offset;
size_t parsing_buf_siz;

/* Current parsing_request Header Struct */
Request *parsing_request;

%}

/* Define YYSTYPE Union */
%union {
    char str[8192];
    int i;
}

%start request

/* === Token Definitions (Modified) === */
%token t_crlf
%token t_backslash
%token t_slash
%token t_digit
%token t_dot
%token t_token_char
%token t_lws
%token t_colon
%token t_separators
%token t_sp
%token t_ws
%token t_ctl
%token t_comma /* ADD: Comma for header value lists */
%token t_equal /* ADD: For equality sign */
%token t_semicolon /* ADD: For semicolon */
%token t_q /* ADD: For 'q' keyword */

/* Types for tokens */
%type<str> t_crlf
%type<i> t_backslash
%type<i> t_slash
%type<i> t_digit
%type<i> t_dot
%type<i> t_token_char
%type<str> t_lws
%type<i> t_colon
%type<i> t_separators
%type<i> t_sp
%type<str> t_ws
%type<i> t_ctl
%type<str> t_comma
%type<str> digits /* MODIFIED: For multiple digit matching */

/* === Intermediate Rule Definitions === */
%type<i> allowed_char_for_token
%type<i> allowed_char_for_text
%type<str> ows
%type<str> token
%type<str> text

/* === New Rules === */
%type<str> q_value /* NEW: For quality values */
%type<str> header_value_part /* NEW: Header value parts */
%type<str> request_headers /* NEW: Recursive headers definition */


%%

/* === Rule: Allowed characters in a token === */
allowed_char_for_token: t_token_char
                      | t_digit { $$ = '0' + $1; }
                      | t_dot;

/* === Rule: A token is a sequence of allowed token chars === */
token: allowed_char_for_token {
    YPRINTF("token: Matched rule 1.\n");
    snprintf($$, 8192, "%c", $1);
}
| token allowed_char_for_token {
    YPRINTF("token: Matched rule 2.\n");
    strcat($$, $2);
};

/* === Rule: Allowed characters in text === */
allowed_char_for_text: allowed_char_for_token
                     | t_separators { $$ = $1; }
                     | t_colon { $$ = $1; }
                     | t_slash { $$ = $1; };

/* === Rule: Multiple digit support === */
digits: t_digit { // Single digit
    $$ = malloc(2);
    snprintf($$, 2, "%d", $1);
}
| digits t_digit { // Recursive match for multiple digits
    char temp[2];
    snprintf(temp, 2, "%d", $2);
    strcat($$, temp);
};

/* === Rule: Header value parts === */
header_value_part: allowed_char_for_text { 
    $$[0] = $1;
    $$[1] = 0;
}
| header_value_part allowed_char_for_text {
    strcat($$, $2);
};

/* === Rule: Quality values === */
q_value: t_semicolon ows t_q ows t_equal ows digits '.' digits {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), ";q=%s.%s", $6, $8);
    $$ = strdup(buffer);
};

/* === Rule: Text concatenation === */
text: header_value_part { // Simple matching
    $$ = strdup($1);
}
| text ows t_comma ows header_value_part { // Comma-separated lists
    char *temp = (char *)malloc(strlen($1) + strlen($5) + 2);
    if (!temp) yyerror("Memory allocation failed");
    sprintf(temp, "%s,%s", $1, $5);
    free($1); // Avoid memory leak
    $$ = temp;
}
| text q_value { // Append quality value
    strcat($$, $2);
};

/* === Rule: Optional white spaces === */
ows: /* empty */ {
    $$[0] = '\0';
}
| t_sp {
    snprintf($$, 8192, "%c", $1);
}
| t_ws {
    strncpy($$, $1, 8192);
};

/* === Rule: Known headers === */
known_header: "Connection" { $$ = "Connection"; }
             | "Accept" { $$ = "Accept"; }
             | "User-Agent" { $$ = "User-Agent"; }
             | "Accept-Encoding" { $$ = "Accept-Encoding"; }
             | "Accept-Language" { $$ = "Accept-Language"; };

/* === Rule: Headers === */
request_header: known_header ows t_colon ows text ows t_crlf {
    YPRINTF("request_Header:\n%s\n%s\n", $1, $5);
    parsing_request->headers = (Request_header *)realloc(parsing_request->headers, sizeof(Request_header) * (parsing_request->header_count + 1));
    strcpy(parsing_request->headers[parsing_request->header_count].header_name, $1);
    strcpy(parsing_request->headers[parsing_request->header_count].header_value, $5);
    parsing_request->header_count++;
};

/* === Rule: Recursive headers === */
request_headers: /* empty */
                | request_headers request_header;

/* === Rule: Request line === */
request_line: token t_sp text t_sp text t_crlf {
    YPRINTF("request_Line:\n%s\n%s\n%s\n", $1, $3, $5);
    strcpy(parsing_request->http_method, $1);
    strcpy(parsing_request->http_uri, $3);
    strcpy(parsing_request->http_version, $5);
};

/* === Final rule: Complete request === */
request: request_line request_headers t_crlf {
    YPRINTF("parsing_request: Matched Success.\n");
    return SUCCESS;
};

%%

/* C code */
void set_parsing_options(char *buf, size_t siz, Request *request) {
    parsing_buf = buf;
    parsing_offset = 0;
    parsing_buf_siz = siz;
    parsing_request = request;
}

void yyerror(const char *s) { fprintf(stderr, "%s\n", s); }

