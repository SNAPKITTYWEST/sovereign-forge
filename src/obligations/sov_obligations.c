// Copyright © 2026 Sovereign Source Foundation. All rights reserved.
// Licensed under Sovereign Source License + Business Source License 1.1.
// Change Date: December 31, 2027 — after which, licensed under AGPL-3.0-only.
// See LICENSE for complete terms.

/*
 * sov_obligations.c -- Obligation Generator Implementation
 * FORGE Phase 2 (stub for compilation)
 */

#include "sov_obligations.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

ObligationSet *sov_obset_new(void)
{
    ObligationSet *obset = (ObligationSet *)malloc(sizeof(ObligationSet));
    if (obset) {
        memset(obset, 0, sizeof(ObligationSet));
        obset->capacity = 10;
        obset->items = (Obligation *)malloc(10 * sizeof(Obligation));
    }
    return obset;
}

void sov_obset_free(ObligationSet *obset)
{
    if (obset) {
        if (obset->items) {
            for (size_t i = 0; i < obset->count; i++) {
                free(obset->items[i].description);
            }
            free(obset->items);
        }
        free(obset);
    }
}

int32_t sov_obset_add_inv(ObligationSet *obset,
                          const int64_t *A __attribute__((unused)),
                          size_t n __attribute__((unused)),
                          uint32_t pc_start,
                          uint32_t pc_end)
{
    if (!obset) return -1;

    /* Expand capacity if needed */
    if (obset->count >= obset->capacity) {
        size_t new_cap = obset->capacity * 2;
        Obligation *new_items = (Obligation *)realloc(obset->items, new_cap * sizeof(Obligation));
        if (!new_items) return -1;
        obset->items = new_items;
        obset->capacity = new_cap;
    }

    /* Initialize new obligation */
    Obligation *obl = &obset->items[obset->count];
    memset(obl, 0, sizeof(Obligation));
    obl->id = obset->next_id;
    obl->kind = OBL_KIND_INV;
    obl->start_pc = pc_start;
    obl->end_pc = pc_end;
    obl->description = (char *)malloc(64);
    if (obl->description) {
        snprintf(obl->description, 64, "Invariant at PC %u", pc_start);
        obl->desc_len = strlen(obl->description);
    }

    obset->count++;
    return (int32_t)obset->next_id++;
}

int32_t sov_obset_add_solve(ObligationSet *obset,
                            const int64_t *A,
                            const int64_t *b,
                            size_t m,
                            size_t n,
                            uint32_t pc_start,
                            uint32_t pc_end)
{
    if (!obset || !A || !b) return -1;

    if (obset->count >= obset->capacity) {
        size_t new_cap = obset->capacity * 2;
        Obligation *new_items = (Obligation *)realloc(obset->items, new_cap * sizeof(Obligation));
        if (!new_items) return -1;
        obset->items = new_items;
        obset->capacity = new_cap;
    }

    Obligation *obl = &obset->items[obset->count];
    memset(obl, 0, sizeof(Obligation));
    obl->id = obset->next_id;
    obl->kind = OBL_KIND_SOLVE;
    obl->start_pc = pc_start;
    obl->end_pc = pc_end;
    obl->description = (char *)malloc(128);
    if (obl->description) {
        snprintf(obl->description, 128, "SOLVE_OK: verify A*x=b for %zu x %zu system at PC %u",
                 m, n, pc_start);
        obl->desc_len = strlen(obl->description);
    }

    obl->params = (void **)malloc(2 * sizeof(void *));
    if (obl->params) {
        obl->params[0] = (void *)(intptr_t)m;
        obl->params[1] = (void *)(intptr_t)n;
        obl->num_params = 2;
    }

    obset->count++;
    return (int32_t)obset->next_id++;
}

int32_t sov_obset_add_lstsq(ObligationSet *obset,
                            const int64_t *A,
                            const int64_t *b,
                            size_t m,
                            size_t n,
                            uint32_t pc_start,
                            uint32_t pc_end)
{
    if (!obset || !A || !b) return -1;

    if (obset->count >= obset->capacity) {
        size_t new_cap = obset->capacity * 2;
        Obligation *new_items = (Obligation *)realloc(obset->items, new_cap * sizeof(Obligation));
        if (!new_items) return -1;
        obset->items = new_items;
        obset->capacity = new_cap;
    }

    Obligation *obl = &obset->items[obset->count];
    memset(obl, 0, sizeof(Obligation));
    obl->id = obset->next_id;
    obl->kind = OBL_KIND_LSTSQ;
    obl->start_pc = pc_start;
    obl->end_pc = pc_end;
    obl->description = (char *)malloc(128);
    if (obl->description) {
        snprintf(obl->description, 128, "LSTSQ_OK: verify A^T(Ax-b)=0 for %zu x %zu system at PC %u",
                 m, n, pc_start);
        obl->desc_len = strlen(obl->description);
    }

    obl->params = (void **)malloc(2 * sizeof(void *));
    if (obl->params) {
        obl->params[0] = (void *)(intptr_t)m;
        obl->params[1] = (void *)(intptr_t)n;
        obl->num_params = 2;
    }

    obset->count++;
    return (int32_t)obset->next_id++;
}

int32_t sov_obset_add_type(ObligationSet *obset,
                           const char *description,
                           uint32_t pc_start,
                           uint32_t pc_end)
{
    if (!obset || !description) return -1;

    /* Expand capacity if needed */
    if (obset->count >= obset->capacity) {
        size_t new_cap = obset->capacity * 2;
        Obligation *new_items = (Obligation *)realloc(obset->items, new_cap * sizeof(Obligation));
        if (!new_items) return -1;
        obset->items = new_items;
        obset->capacity = new_cap;
    }

    /* Initialize new obligation */
    Obligation *obl = &obset->items[obset->count];
    memset(obl, 0, sizeof(Obligation));
    obl->id = obset->next_id;
    obl->kind = OBL_KIND_TYPE;
    obl->start_pc = pc_start;
    obl->end_pc = pc_end;
    obl->description = (char *)malloc(256);
    if (obl->description) {
        snprintf(obl->description, 256, "%s", description);
        obl->desc_len = strlen(obl->description);
    }

    obset->count++;
    return (int32_t)obset->next_id++;
}

int32_t sov_obset_add_prop(ObligationSet *obset,
                           const char *property,
                           uint32_t pc_start,
                           uint32_t pc_end)
{
    if (!obset || !property) return -1;

    if (obset->count >= obset->capacity) {
        size_t new_cap = obset->capacity * 2;
        Obligation *new_items = (Obligation *)realloc(obset->items, new_cap * sizeof(Obligation));
        if (!new_items) return -1;
        obset->items = new_items;
        obset->capacity = new_cap;
    }

    Obligation *obl = &obset->items[obset->count];
    memset(obl, 0, sizeof(Obligation));
    obl->id = obset->next_id;
    obl->kind = OBL_KIND_PROP;
    obl->start_pc = pc_start;
    obl->end_pc = pc_end;
    obl->description = (char *)malloc(256);
    if (obl->description) {
        snprintf(obl->description, 256, "Property: %s", property);
        obl->desc_len = strlen(obl->description);
    }

    obset->count++;
    return (int32_t)obset->next_id++;
}

Obligation *sov_obset_get(ObligationSet *obset, uint32_t id)
{
    if (!obset) return NULL;
    for (size_t i = 0; i < obset->count; i++) {
        if (obset->items[i].id == id)
            return &obset->items[i];
    }
    return NULL;
}

Obligation *sov_obset_at(ObligationSet *obset, size_t index)
{
    if (!obset || index >= obset->count) return NULL;
    return &obset->items[index];
}

int sov_obset_set_witness(ObligationSet *obset,
                          uint32_t id,
                          const void *witness,
                          size_t witness_len)
{
    if (!obset) return -1;
    Obligation *obl = sov_obset_get(obset, id);
    if (!obl) return -1;

    obl->params = (void **)realloc(obl->params, (obl->num_params + 2) * sizeof(void *));
    if (!obl->params) return -1;

    obl->params[obl->num_params] = (void *)witness;
    obl->params[obl->num_params + 1] = (void *)(intptr_t)witness_len;
    obl->num_params += 2;

    return 0;
}

const void *sov_obset_get_witness(ObligationSet *obset,
                                   uint32_t id,
                                   size_t *out_len)
{
    if (!obset || !out_len) return NULL;
    Obligation *obl = sov_obset_get(obset, id);
    if (!obl || obl->num_params < 2) {
        *out_len = 0;
        return NULL;
    }

    *out_len = (size_t)(intptr_t)obl->params[obl->num_params - 1];
    return obl->params[obl->num_params - 2];
}

int sov_obset_to_json(ObligationSet *obset,
                      uint8_t **out_json,
                      size_t *out_len)
{
    if (!obset || !out_json) return -1;

    size_t buf_size = 4096 + obset->count * 256;
    uint8_t *buf = (uint8_t *)malloc(buf_size);
    if (!buf) return -1;

    size_t pos = 0;
    pos += snprintf((char *)buf + pos, buf_size - pos, "{\"version\":1,\"obligations\":[");

    for (size_t i = 0; i < obset->count; i++) {
        Obligation *obl = &obset->items[i];
        if (i > 0) pos += snprintf((char *)buf + pos, buf_size - pos, ",");

        pos += snprintf((char *)buf + pos, buf_size - pos,
                       "{\"id\":%u,\"kind\":%d,\"start_pc\":%u,\"end_pc\":%u,\"description\":\"",
                       obl->id, (int)obl->kind, obl->start_pc, obl->end_pc);

        for (size_t j = 0; j < obl->desc_len && pos < buf_size - 1; j++) {
            uint8_t c = (uint8_t)obl->description[j];
            if (c == '"' || c == '\\') {
                buf[pos++] = '\\';
            }
            buf[pos++] = c;
        }

        pos += snprintf((char *)buf + pos, buf_size - pos, "\"}");
    }

    pos += snprintf((char *)buf + pos, buf_size - pos, "]}");

    *out_json = buf;
    *out_len = pos;
    return 0;
}

ObligationSet *sov_obset_from_json(const uint8_t *json_bytes,
                                    size_t len)
{
    return NULL;
}
