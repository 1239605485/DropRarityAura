-- Drop Rarity Aura 0.1.0
-- Visual only: never mutates item, world-item, or loot data.
local PATCH_INTERVAL = 8 -- DrawItem callbacks per item (roughly 8 game frames)
local RING_RADIUS = 18
local enabled = true
local callback_count = {}
local failed_once = false
local hook_draw_item
local world_active_getter, world_inner, world_whoami, entity_position, item_rare
local new_dust, vector2_type, color_type
local dust_for_rarity = require("rarity_palette")

local function make_vector2(x, y)
    return mod.patch.new_instance(vector2_type, x, y)
end

local function draw_item_postfix(_, args)
    if not enabled or not args then return end
    local world_item = args[1]
    if not world_item then return end

    local ok, err = pcall(function()
        if not mod.patch.invoke(world_active_getter, world_item) then return end
        local item = mod.patch.get_field_value(world_inner, world_item, "object")
        if not item then return end
        local rarity = mod.patch.get_field_value(item_rare, item, "int32")
        local rgb = dust_for_rarity(rarity)
        if not rgb then return end

        local slot = mod.patch.get_field_value(world_whoami, world_item, "int32")
        local key = slot or 0
        local count = (callback_count[key] or 0) + 1
        callback_count[key] = count
        if count % PATCH_INTERVAL ~= 0 then return end

        local x, y = mod.patch.get_field_vec2(entity_position, world_item)
        local angle = ((count * 47 + key * 29) % 360) * math.pi / 180
        local px = x + RING_RADIUS + math.cos(angle) * RING_RADIUS
        local py = y + 8 + math.sin(angle) * (RING_RADIUS * 0.55)
        local point = make_vector2(px, py)
        local tint = mod.patch.new_instance(color_type, rgb[1], rgb[2], rgb[3], 255)
        -- DustID.WhiteTorch (63) is vanilla; rarity-specific tint is passed as Color.
        mod.patch.invoke(new_dust, point, 2, 2, 63, 0, 0, 0, tint, 1.0)
    end)
    if not ok and not failed_once then
        failed_once = true
        mod.warn("掉落物光环绘制回调异常：", tostring(err))
    end
end

local function setup()
    mod.meta = {
        pkg_id = "xinliu.droprarityaura",
        version = "0.1.0",
        version_code = 1,
        api_version = 1,
    }

    local main_type = mod.patch.get_type("Terraria", "Main")
    local world_type = mod.patch.get_type("Terraria", "WorldItem")
    local item_type = mod.patch.get_type("Terraria", "Item")
    local entity_type = mod.patch.get_type("Terraria", "Entity")
    local dust_type = mod.patch.get_type("Terraria", "Dust")
    vector2_type = mod.patch.get_type("Microsoft.Xna.Framework", "Vector2")
    color_type = mod.patch.get_type("Microsoft.Xna.Framework.Graphics", "Color")

    local active_property = mod.patch.get_property(world_type, "active")
    world_active_getter = mod.patch.property_get_method(active_property)
    world_inner = mod.patch.get_field(world_type, "inner")
    world_whoami = mod.patch.get_field(entity_type, "whoAmI")
    entity_position = mod.patch.get_field(entity_type, "position")
    item_rare = mod.patch.get_field(item_type, "rare")
    new_dust = mod.patch.get_method(dust_type, "NewDust", 9)

    -- Hook one visible item's draw call, avoiding a full world-item scan.
    local draw_method = mod.patch.get_method(main_type, "DrawItem", 2)
    if not draw_method then
        mod.warn("找不到 Main.DrawItem；光环未启用。")
        return
    end
    hook_draw_item = mod.patch.install_hook(draw_method, {
        postfix = draw_item_postfix,
        copy = true,
    })
    if hook_draw_item then
        mod.info("掉落物稀有度光环已加载。")
    else
        mod.warn("安装掉落物绘制钩子失败。")
    end
end

local function cleanup()
    if hook_draw_item then
        hook_draw_item:remove()
        hook_draw_item = nil
    end
    callback_count = {}
end

todo_list = { "setup" }
mod.cleanup = cleanup
