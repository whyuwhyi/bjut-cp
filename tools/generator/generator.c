#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  char *text;
  char *expected;
} TokenSample;

static char *xstrdup(const char *s) {
  char *d = malloc(strlen(s) + 1);
  if (!d) {
    perror("malloc");
    exit(1);
  }
  strcpy(d, s);
  return d;
}

static void append(char **buf, const char *s) {
  size_t old_len = *buf ? strlen(*buf) : 0;
  size_t add_len = strlen(s);
  *buf = realloc(*buf, old_len + add_len + 2);
  strcpy(*buf + old_len, s);
}

static TokenSample make_simple(const char *text, const char *type) {
  TokenSample t;
  t.text = xstrdup(text);
  char *out = malloc(64);
  sprintf(out, "%-10s -\n", type);
  t.expected = out;
  return t;
}

static TokenSample make_value(const char *text, const char *type, int value) {
  TokenSample t;
  t.text = xstrdup(text);
  char *out = malloc(64);
  sprintf(out, "%-10s %d\n", type, value);
  t.expected = out;
  return t;
}

static TokenSample make_idn(const char *text) {
  TokenSample t;
  t.text = xstrdup(text);
  char *out = malloc(64);
  sprintf(out, "IDN        %s\n", text);
  t.expected = out;
  return t;
}

static TokenSample gen_keyword() {
  const char *kw[] = {"if", "then", "else", "while", "do", "begin", "end"};
  const char *ty[] = {"IF", "THEN", "ELSE", "WHILE", "DO", "BEGIN", "END"};
  int i = rand() % 7;
  return make_simple(kw[i], ty[i]);
}

static TokenSample gen_operator() {
  const char *op[] = {"+",  "-",  "*",  "/", "<", ">", "=",
                      "<=", ">=", "<>", "(", ")", ";"};
  const char *ty[] = {"ADD", "SUB", "MUL", "DIV", "LT",  "GT",  "EQ",
                      "LE",  "GE",  "NEQ", "SLP", "SRP", "SEMI"};
  int i = rand() % 13;
  return make_simple(op[i], ty[i]);
}

static TokenSample gen_decimal() {
  int val = rand() % 100000;
  char buf[16];
  sprintf(buf, "%d", val);
  return make_value(buf, "DEC", val);
}

static TokenSample gen_octal() {
  int val = rand() % 040000;
  char buf[16];
  sprintf(buf, "0%o", val);
  return make_value(buf, "OCT", val);
}

static TokenSample gen_hex() {
  int val = rand() % 0x10000;
  char buf[16];
  sprintf(buf, "0x%x", val);
  return make_value(buf, "HEX", val);
}

static TokenSample gen_invalid_octal() {
  char buf[8];
  sprintf(buf, "09%d", rand() % 10);
  return make_simple(buf, "ILOCT");
}

static TokenSample gen_invalid_hex() {
  char buf[8];
  sprintf(buf, "0xz%d", rand() % 10);
  return make_simple(buf, "ILHEX");
}

static TokenSample gen_identifier() {
  int len = rand() % 6 + 1;
  char *buf = malloc(len + 2);
  buf[0] = 'a' + rand() % 26;
  for (int i = 1; i < len; i++) {
    int r = rand() % 36;
    buf[i] = r < 26 ? ('a' + r) : ('0' + r - 26);
  }
  buf[len] = 0;
  TokenSample t = make_idn(buf);
  free(buf);
  return t;
}

static TokenSample generate_random_token() {
  switch (rand() % 9) {
  case 0:
    return gen_keyword();
  case 1:
    return gen_operator();
  case 2:
    return gen_decimal();
  case 3:
    return gen_octal();
  case 4:
    return gen_hex();
  case 5:
    return gen_invalid_octal();
  case 6:
    return gen_invalid_hex();
  default:
    return gen_identifier();
  }
}

void generate_sample(FILE *out) {
  char *src = strdup("");
  char *exp = strdup("");

  int terms = rand() % 100 + 1;
  for (int i = 0; i < terms; i++) {
    TokenSample t = generate_random_token();
    append(&src, t.text);
    append(&src, " ");
    append(&exp, t.expected);
    free(t.text);
    free(t.expected);
  }
  append(&src, ";\n");
  append(&exp, "SEMI       -\n");
  append(&exp, "EOF        -\n");

  fprintf(out, "sample:\n%s", src);
  fprintf(out, "expected output:\n%s\n", exp);
  free(src);
  free(exp);
}

int main(int argc, char **argv) {
  int count = 10;
  const char *outfile = NULL;
  unsigned int seed = (unsigned int)time(NULL);
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-n") && i + 1 < argc)
      count = atoi(argv[++i]);
    else if (!strcmp(argv[i], "-o") && i + 1 < argc)
      outfile = argv[++i];
    else if (!strcmp(argv[i], "-s") && i + 1 < argc)
      seed = atoi(argv[++i]);
    else {
      fprintf(stderr, "Usage: %s [-n count] [-o outfile] [-s seed]\n", argv[0]);
      return 1;
    }
  }

  srand(seed);
  FILE *out = outfile ? fopen(outfile, "w") : stdout;
  if (!out) {
    perror("fopen");
    return 1;
  }
  for (int i = 0; i < count; i++)
    generate_sample(out);
  if (out != stdout)
    fclose(out);
  return 0;
}
