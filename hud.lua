local inspect = require('libs/inspect')

local function click()
    print("You clicked me")
    local r = eng_getSelectedInstances()
    -- local r = eng_getTeamID(100)
    for _, v in ipairs(r) do
        print(inspect(eng_getTeamID(v)))
    end
end

local function thing()
    gui_setVisibility("broken", 0)
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
            onClick = thing,
            kind = GuiLayoutKind__TEXT_BUTTON,
            text = "Click!!!",
            color = 0x101080ff,
            -- children = {}
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
            onClick = testFire,
            kind = GuiLayoutKind__IMAGE_BUTTON,
            text = "test.png",
            color = 0x000000ff,
            -- children = {}
            name = "broken",
        }
    }
}