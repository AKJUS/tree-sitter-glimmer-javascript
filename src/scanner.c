// ----------------------------------------------------------
//
//
//
// Scanners are not extensible, so we have to copy them,
//
// To Update:
//  - replace `tree-sitter-javascript/scanner.h` with `https://github.com/tree-sitter/tree-sitter-javascript/blob/master/src/scanner.c`
//
// ----------------------------------------------------------

#include "./tree-sitter-javascript/scanner.h"
#include <string.h>

enum GlimmerTokenType {
    RAW_TEXT = JSX_TEXT + 1,
};

static bool scan_raw_text(void *payload, TSLexer *lexer) {
    lexer->mark_end(lexer);

    const char *end_delimiter = "</TEMPLATE";

    unsigned delimiter_index = 0;
    bool is_valid = false;
    while (lexer->lookahead) {
        if ((char)towupper(lexer->lookahead) == end_delimiter[delimiter_index]) {
            delimiter_index++;
            if (delimiter_index == strlen(end_delimiter)) {
                break;
            }
            advance(lexer);
        } else {
            is_valid = true;
            delimiter_index = 0;
            advance(lexer);
            lexer->mark_end(lexer);
        }
    }
    if (is_valid) {
        lexer->result_symbol = RAW_TEXT;
    }
    return is_valid;
}

void *tree_sitter_glimmer_javascript_external_scanner_create() {
    return tree_sitter_javascript_external_scanner_create();
}

void tree_sitter_glimmer_javascript_external_scanner_destroy(void *payload) {
    tree_sitter_javascript_external_scanner_destroy(payload);
}

unsigned tree_sitter_glimmer_javascript_external_scanner_serialize(void *payload, char *buffer) {
    return tree_sitter_javascript_external_scanner_serialize(payload, buffer);
}

void tree_sitter_glimmer_javascript_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
    tree_sitter_javascript_external_scanner_deserialize(payload, buffer, length);
}

bool tree_sitter_glimmer_javascript_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
    if (valid_symbols[RAW_TEXT]) {
        return scan_raw_text(payload, lexer);
    }

    // The upstream JS scanner refuses to insert an automatic semicolon before `<`
    // because in standard JS it could be a less-than operator. In GJS/GTS, `<template>`
    // is a valid class body member, so we handle the AUTOMATIC_SEMICOLON case ourselves:
    // we call scan_automatic_semicolon directly (accessible from the included scanner.h),
    // and when it refuses due to `<`, we check whether `<template>` follows and insert
    // the semicolon if so.
    if (valid_symbols[AUTOMATIC_SEMICOLON]) {
        bool scanned_comment = false;
        bool result = scan_automatic_semicolon(lexer, !valid_symbols[LOGICAL_OR], &scanned_comment);
        if (!result && !scanned_comment) {
            if (lexer->lookahead == '<') {
                skip(lexer);
                const char *tag = "template>";
                for (unsigned i = 0; tag[i] != '\0'; i++) {
                    if ((char)lexer->lookahead != tag[i]) return false;
                    skip(lexer);
                }
                lexer->result_symbol = AUTOMATIC_SEMICOLON;
                return true;
            }
            if (valid_symbols[TERNARY_QMARK] && lexer->lookahead == '?') {
                return scan_ternary_qmark(lexer);
            }
        }
        return result;
    }

    return tree_sitter_javascript_external_scanner_scan(payload, lexer, valid_symbols);
}
