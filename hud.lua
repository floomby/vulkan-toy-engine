local inspect = require('libs/inspect')
require('bit')

local keys = require('lua/keys')
local mods = require('lua/mods')

local data_bindings = {}
local data_bindings_cache = {}

local function update_bound_data()
    for name, wrapper in pairs(data_bindings) do
        local value = wrapper()
        if (value ~= data_bindings_cache[name]) then
            data_bindings_cache[name] = value
            gui_setLabelText(name, value)
        end
    end
end

local function bind_data(name, wrapper)
    data_bindings[name] = wrapper
end

-- Idk where this belongs, but it isn't here (I should probably just lua_dostring it)
local audioDeviceList = eng_listAudioDevices()
eng_pickAudioDevice(audioDeviceList[1])

local function Split(s, delimiter)
    local result = {};
    for match in (s..delimiter):gmatch("(.-)"..delimiter) do
        table.insert(result, match);
    end
    return result;
end

local function engret_visibility()
    gui_setVisibility("engret", #eng_getSelectedInstances() > 0)
end

local function mods_to_mode(mds)
    local mode = InsertionMode__OVERWRITE;
    if bit.band(mods.MOD_SHIFT, mds) ~= 0 then
        mode = InsertionMode__BACK
    end
    if bit.band(mods.MOD_ALT, mds) ~= 0 then
        mode = InsertionMode__FRONT
    end
    return mode
end

local build_options = {}
local build_units = {}
local function build_visibility()
    local units = eng_getSelectedInstances()
    local can_build = false
    build_options = {}
    for _, unit in ipairs(units) do
        local options = eng_getInstanceBuildOptions(unit)
        for _, option in ipairs(options) do
            can_build = true
            build_options[option] = true
            if build_units[option] == nil then
                build_units[option] = {}
            end
            build_units[option][unit] = true
        end
    end
    gui_setVisibility("build", can_build)
end

function Build_handler(mods, name)
    local true_name = Split(name, " ")[1]
    -- print(inspect(build_units))
    for unit, _ in pairs(build_units[true_name]) do
        cmd_build(unit, true_name, mods_to_mode(mods))
    end
end

local function show_build_options()
    Build_menu.children = {}
    local i = 0
    for option, _ in pairs(build_options) do
        -- print("textures/" .. option .. "_icon.png")
        table.insert(Build_menu.children, {
            x = 0.1 + i * 0.1,
            y = 0.8,
            height = 0.1,
            kind = GuiLayoutKind__IMAGE_BUTTON,
            images = {
                "textures/" .. option .. "_icon.png"
            },
            onClick = Build_handler,
            color = 0x000000ff,
            name = option .. " build button"
        })
        i = i + 1
    end
    gui_addPanel("build menu", "Build_menu")
end

local function remove_build_menu()
    gui_removePanel("Build_menu")
end

-- allow for context dependent keybinding overrides
local keybinding_callback_stacks = {}

local function add_keybinding(key, func)
    if keybinding_callback_stacks[key] == nil then
        keybinding_callback_stacks[key] = {}
        eng_declareKeyBinding(key);
    end
    keybinding_callback_stacks[key][#keybinding_callback_stacks[key] + 1] = func
    Keybinding_callbacks[key] = func
end

local function pop_keybinding(key)
    if keybinding_callback_stacks[key] == nil then
        return
    end
    keybinding_callback_stacks[key][#keybinding_callback_stacks[key]] = nil
    if #keybinding_callback_stacks[key] == 0 then
        eng_undeclareKeyBinding(key);
    else
        Keybinding_callbacks[key] = keybinding_callback_stacks[key][#keybinding_callback_stacks[key]]
    end
end

-- local function tester()
--     print("Yay!")
-- end

-- local function tester2()
--     print("Yay2!")
-- end

-- add_keybinding(keys.KEY_G, tester2)
-- add_keybinding(keys.KEY_G, tester)
-- pop_keybinding(keys.KEY_G)

-- for text buttons it ignores the width field and just makes the width based on what the text says
Hud = {
    x = -1.0,
    y = 0.7,
    width = 2.0,
    height = 0.3,
    -- onClick = click,
    kind = GuiLayoutKind__PANEL,
    text = "",
    color = 0x6050cc60,
    onPeriodicUpdate = update_bound_data,
    children = {
        -- {
        --     x = -0.9,
        --     y = 0.75,
        --     width = 0.5,
        --     height = 0.1,
        --     -- onClick = click,
        --     kind = GuiLayoutKind__TEXT_BUTTON,
        --     text = "Click!!!",
        --     color = 0x101080ff,
        --     -- children = {}
        --     name = "button",
        --     tooltip = "This has a tooltip!"
        -- },
        {
            x = -0.2,
            y = 0.75,
            width = 0.5,
            height = 0.1,
            -- onClick = editing,
            kind = GuiLayoutKind__TEXT_EDITABLE,
            text = "Edit me",
            color = 0x101080ff,
            -- children = {}
            tooltip = "This has a tooltip!\nIsn't that nice",
            -- onTextUpdated = text_updated
        },
        {
            x = -0.9,
            y = 0.875,
            width = 0.5,
            height = 0.1,
            -- onClick = testFire,
            -- onToggle = ,
            onSelectionChanged = engret_visibility,
            kind = GuiLayoutKind__IMAGE_BUTTON,
            images = {
                "engage.png",
                "retreat.png"
            },
            color = 0x000000ff,
            -- children = {}
            name = "engret",
        },
        -- {
        --     x = 0.3,
        --     y = -1.0,
        --     width = 0.2,
        --     height = 0.1,
        --     onSelectionChanged = engret_visibility,
        --     kind = GuiLayoutKind__PANEL,
        --     color = 0x6050cc60,
        --     children = {
        --         {
        --             x = 0.3,
        --             y = -1.0,
        --             width = 0.5,
        --             height = 0.1,
        --             onSelectionChanged = engret_visibility,
        --             kind = GuiLayoutKind__TEXT_BUTTON,
        --             color = 0x000000ff,
        --             name = "rus",
        --             text = "",
        --         },
        --     }
        -- },
        {
            x = 0.3,
            y = -1.0,
            width = 0.5,
            height = 0.1,
            onSelectionChanged = engret_visibility,
            kind = GuiLayoutKind__TEXT_BUTTON,
            color = 0x000000ff,
            secondaryColor = 0x6050cc60,
            name = "rus",
            text = "",
        },
        {
            x = -0.7,
            y = 0.875,
            width = 0.5,
            height = 0.1,
            onClick = show_build_options,
            -- onToggle = engret_visibility,
            onSelectionChanged = build_visibility,
            kind = GuiLayoutKind__IMAGE_BUTTON,
            images = {
                "textures/build.png",
            },
            color = 0x000000ff,
            -- children = {}
            name = "build",
        },
        {
            x = 0.05,
            y = 0.75,
            width = 0.9,
            height = 0.2,
            -- onClick = click,
            kind = GuiLayoutKind__PANEL,
            color = 0x303030bb,
            children = {
            },
            name = "build menu"
        }
    }
}

local teamID, display_name = state_getTeamIAm()
net_declareTeam(teamID, display_name)

local function resource_wrapper()
    return math.floor(state_getResources(teamID)) .. " RUs"
end

bind_data("rus", resource_wrapper)

-- I am probably testing something if I am team 0
if teamID == 0 then
    net_declareNullTeam(1, "Team 1")
    net_declareNullTeam(2, "Team 2")
end

Build_menu = {
    x = 0.05,
    y = 0.75,
    width = 0.9,
    height = 0.2,
    -- onClick = click,
    kind = GuiLayoutKind__PANEL,
    color = 0x00000000,
    onSelectionChanged = remove_build_menu,
    children = {
    }
}

gui_setVisibility("build menu", true)
build_visibility()
engret_visibility()

state_giveResources(2, 50000)

-- eng_declareKeyBinding(keys.KEY_G);

-- cmd_createInstance("miner", { 0.0, 0.0, 3.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1)
-- gui_setLabelText("button", "Hello")
-- print("Created instance " .. id .. " which is a " .. eng_getInstanceEntityName(id))
-- eng_setInstanceHealth(id, 40)
-- print(eng_getInstanceHealth(id))
-- net_declareTeam(1, "josh")

-- local function dummyCallback2(id)
--     cmd_destroyInstance(id)
-- end

-- cmd_createInstance("miner", { -10.0, -6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback2)

-- state_giveResources(2, 5000)

-- cmd_createInstance("asteroid", { -20.0, 0.0, 0.0 }, { -0.798, 0.420, -0.104, 0.420 }, 1, dummyCallback)
-- cmd_createInstance("shipyard", { 6.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, nil)
cmd_createInstance("shipyard", { -6.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, nil)
-- cmd_createInstance("ship", { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, nil)
cmd_createInstance("ship", { 26.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, nil)
cmd_createInstance("miner", { 6.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, nil)

-- cmd_createInstance("miner", { -10.0, -6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -100.0, 3.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, dummyCallback)

-- net_pause(false)



-- state_giveResources(1, 55.3)
-- print(state_getResources(1))

-- net_pause(false)
-- cmd_createInstance("miner", { 0.0, 3.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)

-- cmd_createInstance("ship", { -10.0, -6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, nil)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, nil)

-- cmd_createInstance("shipyard", { 0.0, 0.0, -5.0 }, { -0.798, 0.420, -0.104, 0.420 }, 2, nil)

-- cmd_createInstance("ship", { 110.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)

-- state_giveResources(1, 50.76)

-- cmd_destroyInstance(100)

-- net_pause(false)
-- print(eng_fps())


-- eng_playSound("test.ogg")