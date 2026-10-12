-- Dash: instantly moves the caster forward along their last move direction,
-- scaled by a fixed-point distance constant.
function cast(caster_id, target_id)
  local dash_distance = fx_from_int(6)
  local mx, my = get_move_dir(caster_id)
  local cx, cy = get_pos(caster_id)
  local nx = cx + fx_mul(mx, dash_distance)
  local ny = cy + fx_mul(my, dash_distance)
  set_pos(caster_id, nx, ny)
end
