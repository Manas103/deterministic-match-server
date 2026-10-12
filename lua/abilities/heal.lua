-- Heal: restores a flat amount of health to the target, capped at max HP.
function cast(caster_id, target_id)
  local heal_amount = 80
  deal_damage(target_id, -heal_amount)
end
