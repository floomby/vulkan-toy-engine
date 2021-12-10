local inspect = require('libs/inspect')
-- require('lua/server_callbacks')

-- Idk where this belongs, but it isn't here
local audioDeviceList = eng_listAudioDevices()
eng_pickAudioDevice(audioDeviceList[1])

local function click()
    -- print("You clicked me")
    -- local r = eng_getSelectedInstances()
    -- -- -- local r = eng_getTeamID(100)
    -- for _, v in ipairs(r) do
    --     print(inspect(eng_getTeamID(v)))
    -- end
    local r = eng_listAudioDevices()
    -- -- local r = eng_getTeamID(100)
    for _, v in ipairs(r) do
        print(v)
    end
end

local function text_updated()
    print("Text updated")
end

local function engret_visibility()
    print("Setting visibility")
    gui_setVisibility("engret", #eng_getSelectedInstances() > 0)
end

local function build_visibility()
    print("Selection changed")
    local r = eng_getSelectedInstances()
    local canBuild = false
    for _, v in ipairs(r) do
        if eng_instanceCanBuild(v) then
            canBuild = true
            break
        end
    end
    gui_setVisibility("build", canBuild)
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
        -- {
        --     -- panel for unit details
        --     x = -0.9,
        --     y = 0.75,
        --     width = 0.5,
        --     height = 0.1,
        --     onClick = centerSelected,
        --     kind = GuiLayoutType__PANEL,
        --     text = "",
        --     color = 0x000000ff,
        -- },
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
        -- {
        --     x = -0.9,
        --     y = 0.875,
        --     width = 0.5,
        --     height = 0.1,
        --     onClick = testFire,
        --     kind = GuiLayoutKind__TEXT_BUTTON,
        --     text = "Fire",
        --     color = 0x000000ff,
        --     -- children = {}
        --     name = "broken",
        -- }
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
            -- onClick = testFire,
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

gui_setVisibility("build menu", true)
build_visibility()
engret_visibility()

-- cmd_createInstance("miner", { 0.0, 0.0, 3.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1)
-- gui_setLabelText("button", "Hello")
-- print("Created instance " .. id .. " which is a " .. eng_getInstanceEntityName(id))
-- eng_setInstanceHealth(id, 40)
-- print(eng_getInstanceHealth(id))
-- net_declareTeam(1, "josh")


local function dummyCallback(id)
    print("callback with id of " .. id)
end

local function dummyCallback2(id)
    print("callback with id of " .. id * 2)
end

-- cmd_createInstance("asteroid", { -95.0, 0.0, 0.0 }, { -0.798, 0.420, -0.104, 0.420 }, 1, dummyCallback)
cmd_createInstance("shipyard", { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, dummyCallback)
-- cmd_createInstance("ship", { -10.0, -6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
-- cmd_createInstance("ship", { -10.0, 6.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)
cmd_createInstance("ship", { -100.0, 3.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 2, dummyCallback)


-- net_declareTeam(1, "josh")
-- state_giveResources(1, 55.3)
-- print(state_getResources(1))

net_pause(false)
-- cmd_createInstance("miner", { 0.0, 3.0, 0.0 }, { 1.0, 0.0, 0.0, 0.0 }, 1, dummyCallback)


for i = 0,1,1 
do 
--    print(inspect(Server_callbacks[i]))
end
