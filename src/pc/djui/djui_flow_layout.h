#pragma once
#include "djui.h"

struct DjuiFlowLayout {
    struct DjuiBase base;
    enum DjuiFlowDirection flowDirection;
    struct DjuiScreenValue margin;
    f32 contentHeight;
    f32 scrollY;
    struct DjuiBase* lastAutoScrollSelection;
    bool orderedNavigation;
};

void djui_flow_layout_set_flow_direction(struct DjuiFlowLayout* layout, enum DjuiFlowDirection flowDirection);
void djui_flow_layout_set_margin(struct DjuiFlowLayout* layout, f32 margin);
void djui_flow_layout_set_margin_type(struct DjuiFlowLayout* layout, enum DjuiScreenValueType marginType);

struct DjuiFlowLayout* djui_flow_layout_create(struct DjuiBase* parent);
bool djui_flow_layout_navigate(struct DjuiBase* selected, s8 direction, struct DjuiBase** pick);
