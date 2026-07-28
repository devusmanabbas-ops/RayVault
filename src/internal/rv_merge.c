#include "rv_merge.h"
#include "rv_dict.h"

#include <string.h>

void rv_merge_options_default(rv_merge_options *o)
{
    if (!o)
        return;
    memset(o, 0, sizeof(*o));
    o->prefer_right_on_conflict = 1;
    o->remap_colliding_ids = 1;
    o->drop_unlinked_markers = 1;
    o->include_notes = 1;
}

static uint32_t rv_merge_map_name(rv_builder *out, const rv_snam_table *snam,
                                  uint32_t id, uint32_t *remapped)
{
    const rv_name_entry *e;
    uint32_t nid = 0;
    if (!id)
        return 0;
    e = rv_dict_by_id(snam, id);
    if (!e || !e->str)
        return 0;
    if (rv_builder_add_name(out, e->str, &nid) != 0)
        return 0;
    if (nid != id && remapped)
        (*remapped)++;
    return nid;
}

static int rv_merge_side(rv_builder *out, const rv_parsed *p,
                         const rv_index *idx, const rv_merge_options *opt,
                         rv_merge_stats *st, uint32_t id_bias)
{
    uint32_t i;
    (void)idx;
    for (i = 0; i < p->snam.count; i++) {
        uint32_t nid = 0;
        if (rv_builder_add_name(out, p->snam.entries[i].str, &nid) != 0)
            return -1;
        st->names_merged++;
    }
    for (i = 0; i < p->rout.count; i++) {
        rv_builder_route r;
        memset(&r, 0, sizeof(r));
        r.route_id = p->rout.items[i].route_id + id_bias;
        r.site_name_id = rv_merge_map_name(out, &p->snam,
                                           p->rout.items[i].site_name_id,
                                           &st->ids_remapped);
        r.far_name_id = rv_merge_map_name(out, &p->snam,
                                          p->rout.items[i].far_name_id,
                                          &st->ids_remapped);
        r.length_km = p->rout.items[i].length_km;
        r.fiber_count = p->rout.items[i].fiber_count;
        r.flags = p->rout.items[i].flags;
        if (rv_builder_add_route(out, &r) != 0)
            return -1;
        st->routes_merged++;
    }
    for (i = 0; i < p->inst.count; i++) {
        rv_builder_inst inst;
        memset(&inst, 0, sizeof(inst));
        inst.inst_id = p->inst.items[i].inst_id + id_bias;
        inst.model_name_id = rv_merge_map_name(out, &p->snam,
                                               p->inst.items[i].model_name_id,
                                               &st->ids_remapped);
        inst.serial_name_id = rv_merge_map_name(out, &p->snam,
                                                p->inst.items[i].serial_name_id,
                                                &st->ids_remapped);
        inst.wavelength_nm = p->inst.items[i].wavelength_nm;
        inst.pulse_default_ns = p->inst.items[i].pulse_default_ns;
        inst.dynamic_range_db = p->inst.items[i].dynamic_range_db;
        inst.flags = p->inst.items[i].flags;
        if (rv_builder_add_inst(out, &inst) != 0)
            return -1;
    }
    for (i = 0; i < p->clbr.count; i++) {
        rv_builder_calib c;
        memset(&c, 0, sizeof(c));
        c.calib_id = p->clbr.items[i].calib_id + id_bias;
        c.inst_id = p->clbr.items[i].inst_id + id_bias;
        c.calibrated_unix = p->clbr.items[i].calibrated_unix;
        c.refractive_index = p->clbr.items[i].refractive_index;
        c.backscatter_coef = p->clbr.items[i].backscatter_coef;
        c.splice_loss_db = p->clbr.items[i].splice_loss_db;
        c.flags = p->clbr.items[i].flags;
        if (rv_builder_add_calib(out, &c) != 0)
            return -1;
    }
    for (i = 0; i < p->wind.count; i++) {
        rv_builder_window w;
        memset(&w, 0, sizeof(w));
        w.window_id = p->wind.items[i].window_id + id_bias;
        w.route_id = p->wind.items[i].route_id + id_bias;
        w.inst_id = p->wind.items[i].inst_id + id_bias;
        w.calib_id = p->wind.items[i].calib_id + id_bias;
        w.wave_block_id = p->wind.items[i].wave_block_id + id_bias;
        w.pulse_ns = p->wind.items[i].pulse_ns;
        w.range_km = p->wind.items[i].range_km;
        w.resolution_m = p->wind.items[i].resolution_m;
        w.acquired_unix = p->wind.items[i].acquired_unix;
        if (rv_builder_add_window(out, &w) != 0)
            return -1;
        st->windows_merged++;
    }
    for (i = 0; i < p->wave.count; i++) {
        const rv_wave_block *wb = &p->wave.items[i];
        if (!wb->samples || wb->sample_count == 0)
            continue;
        if (rv_builder_add_wave_copy(out, wb->block_id + id_bias,
                                     wb->window_id + id_bias, wb->samples,
                                     wb->sample_count, wb->start_m,
                                     wb->step_m) != 0)
            return -1;
        st->waves_merged++;
    }
    for (i = 0; i < p->mark.count; i++) {
        rv_builder_marker m;
        int linked = 0;
        uint32_t j;
        memset(&m, 0, sizeof(m));
        m.marker_id = p->mark.items[i].marker_id + id_bias;
        m.window_id = p->mark.items[i].window_id + id_bias;
        m.label_name_id = rv_merge_map_name(out, &p->snam,
                                            p->mark.items[i].label_name_id,
                                            &st->ids_remapped);
        m.distance_m = p->mark.items[i].distance_m;
        m.loss_db = p->mark.items[i].loss_db;
        m.reflectance_db = p->mark.items[i].reflectance_db;
        m.kind = p->mark.items[i].kind;
        m.severity = p->mark.items[i].severity;
        m.flags = p->mark.items[i].flags;
        if (opt->drop_unlinked_markers) {
            for (j = 0; j < p->wind.count; j++) {
                if (p->wind.items[j].window_id == p->mark.items[i].window_id) {
                    linked = 1;
                    break;
                }
            }
            if (!linked) {
                st->markers_dropped++;
                continue;
            }
        }
        if (rv_builder_add_marker(out, &m) != 0)
            return -1;
        st->markers_merged++;
    }
    for (i = 0; i < p->link.count; i++) {
        rv_builder_link l;
        memset(&l, 0, sizeof(l));
        l.link_id = p->link.items[i].link_id + id_bias;
        l.from_kind = p->link.items[i].from_kind;
        l.from_id = p->link.items[i].from_id + id_bias;
        l.to_kind = p->link.items[i].to_kind;
        l.to_id = p->link.items[i].to_id + id_bias;
        l.flags = p->link.items[i].flags;
        if (rv_builder_add_link(out, &l) != 0)
            return -1;
    }
    if (opt->include_notes) {
        for (i = 0; i < p->note.count; i++) {
            rv_builder_note n;
            memset(&n, 0, sizeof(n));
            n.note_id = p->note.items[i].note_id + id_bias;
            n.target_kind = p->note.items[i].target_kind;
            n.target_id = p->note.items[i].target_id + id_bias;
            n.text_name_id = rv_merge_map_name(out, &p->snam,
                                               p->note.items[i].text_name_id,
                                               &st->ids_remapped);
            n.flags = p->note.items[i].flags;
            if (rv_builder_add_note(out, &n) != 0)
                return -1;
        }
    }
    return 0;
}

int rv_merge_into_builder(rv_builder *out, const rv_parsed *left,
                          const rv_index *lidx, const rv_parsed *right,
                          const rv_index *ridx, const rv_merge_options *opt,
                          rv_merge_stats *stats)
{
    rv_merge_options def;
    rv_merge_stats local;
    uint32_t bias = 0;
    if (!out || !left || !right || !lidx || !ridx)
        return -1;
    if (!opt) {
        rv_merge_options_default(&def);
        opt = &def;
    }
    memset(&local, 0, sizeof(local));
    if (rv_merge_side(out, left, lidx, opt, &local, 0) != 0)
        return -1;
    if (opt->remap_colliding_ids)
        bias = 100000u;
    if (rv_merge_side(out, right, ridx, opt, &local, bias) != 0)
        return -1;
    if (stats)
        *stats = local;
    (void)opt->prefer_right_on_conflict;
    return 0;
}
