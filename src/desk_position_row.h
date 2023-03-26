#pragma once
#include <adwaita.h>

G_BEGIN_DECLS

#define DESK_TYPE_POSITION_ROW (desk_position_row_get_type())

G_DECLARE_FINAL_TYPE(DeskPositionRow, desk_position_row, DESK, POSITION_ROW, AdwActionRow)

DeskPositionRow *desk_position_row_new(void);

G_END_DECLS
