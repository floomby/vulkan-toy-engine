local inspect = require('libs/inspect')
-- require('lua/server_callbacks')

-- Idk where this belongs, but it isn't here (I should probably just lua_dostring it)
local audioDeviceList = eng_listAudioDevices()
eng_pickAudioDevice(audioDeviceList[1])

local function click()
    local r = eng_listAudioDevices()
    for _, v in ipairs(r) do
        print(v)
    end
end

local function text_updated()
    -- print("Text updated")
end

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

local build_options = {}
local function build_visibility()
    local units = eng_getSelectedInstances()
    local can_build = false
    build_options = {}
    for _, unit in ipairs(units) do
        local options = eng_getInstanceBuildOptions(unit)
        for _, option in ipairs(options) do
            can_build = true
            build_options[option] = true
        end
    end
    gui_setVisibility("build", can_build)
end

local function other(index)
    local r = eng_getSelectedInstances()
    for _, v in ipairs(r) do
        eng_setInstanceStateEngage(v, index)
    end
end

local function thing()
    eng_setInstanceHealth(101, 0.7)
    -- gui_setVisibility("broken", 0)
end

local function centerSelected()
    print("You need to make the api bindings")
end

local function testFire()
    pcall(test_fire)
end

local function dummyCallback(id)
    -- print("callback with id of " .. id)
    -- eng_setInstanceCustomState(id, "hold_position", 0)
    -- print(inspect(eng_getInstanceCustomState(id, "hold_position")))
    -- print(inspect(eng_getInstanceCustomState(id, "something_else")))
    print(inspect(eng_getInstancePosition(id)))
end

function Build_handler(mods, name)
    -- print("build handler: " .. mods .. " " .. name)
    cmd_createInstance(Split(name, " ")[1], { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, dummyCallback)
end

local function show_build_options()
    Build_menu.children = {}
    local i = 0
    for option, _ in pairs(build_options) do
        print("textures/" .. option .. "_icon.png")
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
    print("removing panel")
    gui_removePanel("Build_menu")
end

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
    children = {
        {
            x = -0.9,
            y = 0.75,
            width = 0.5,
            height = 0.1,
            onClick = click,
            kind = GuiLayoutKind__TEXT_BUTTON,
            text = "Click!!!",
            color = 0x101080ff,
            -- children = {}
            name = "button",
            tooltip = "This has a tooltip!"
        },
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
            onTextUpdated = text_updated
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

local function new_handler()
    print("I guess the gui can be dynamically loaded now")
end

local function new_handler2()
    print("There is an off by 1 error")
end

local function build_menu_creator()
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

-- cmd_createInstance("miner", { 0.0, 0.0, 3.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1)
-- gui_setLabelText("button", "Hello")
-- print("Created instance " .. id .. " which is a " .. eng_getInstanceEntityName(id))
-- eng_setInstanceHealth(id, 40)
-- print(eng_getInstanceHealth(id))
-- net_declareTeam(1, "josh")

local function dummyCallback2(id)
    print("callback with id of " .. id * 2)
end

-- cmd_createInstance("asteroid", { -95.0, 0.0, 0.0 }, { -0.798, 0.420, -0.104, 0.420 }, 1, dummyCallback)
-- cmd_createInstance("shipyard", { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, nil)
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

net_pause(false)

local teamID, display_name = state_getTeamIAm()
-- print(teamID .. " - " .. display_name)
net_declareTeam(teamID, display_name)
-- state_giveResources(1, 55.3)
-- print(state_getResources(1))

-- net_pause(false)
-- cmd_createInstance("miner", { 0.0, 3.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)


for i = 0,1,1 
do 
--    print(inspect(Server_callbacks[i]))
end


cmd_createInstance("miner", { -10.0, -6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, nil)
cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, nil)