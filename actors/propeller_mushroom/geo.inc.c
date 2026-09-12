const GeoLayout vr_propeller_mushroom_geo[] = {
    GEO_CULLING_RADIUS(600),
    GEO_OPEN_NODE(),
        GEO_BILLBOARD(),
        GEO_OPEN_NODE(),
            GEO_DISPLAY_LIST(LAYER_ALPHA, vr_propeller_mushroom_dl),
        GEO_CLOSE_NODE(),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

// Dropped cosmetic helmets reuse the same head-space mesh at Mario's scale.
const GeoLayout vr_propeller_helmet_geo[] = {
    GEO_NODE_START(), GEO_OPEN_NODE(),
    GEO_SCALE(0, 16384), GEO_OPEN_NODE(),
    GEO_ROTATION_NODE(0, 0, 0, 90), GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE, vr_propeller_helmet_dl),
    GEO_TRANSLATE_NODE(0, 318, -12, 0), GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE, vr_propeller_rotor_dl),
    GEO_CLOSE_NODE(), GEO_CLOSE_NODE(), GEO_CLOSE_NODE(), GEO_CLOSE_NODE(), GEO_END(),
};
const GeoLayout vr_hammer_helmet_geo[] = {
    GEO_NODE_START(), GEO_OPEN_NODE(),
    GEO_SCALE(0, 16384), GEO_OPEN_NODE(),
    GEO_ROTATION_NODE(0, 0, 0, 90), GEO_OPEN_NODE(),
    GEO_DISPLAY_LIST(LAYER_OPAQUE, vr_hammer_helmet_dl),
    GEO_CLOSE_NODE(), GEO_CLOSE_NODE(), GEO_CLOSE_NODE(), GEO_END(),
};
