// 0x05014630
const GeoLayout tweester_geo[] = {
    GEO_CULLING_RADIUS(5000),
    GEO_OPEN_NODE(),
      GEO_DISPLAY_LIST(LAYER_TRANSPARENT, tornado_seg5_dl_050145C0),
    GEO_CLOSE_NODE(),
    GEO_END(),
};

const GeoLayout vr_twirl_tornado_geo[] = {
    GEO_CULLING_RADIUS(2000),
    GEO_OPEN_NODE(),
        GEO_ASM(0, geo_vr_twirl_tornado),
    GEO_CLOSE_NODE(),
    GEO_END(),
};
