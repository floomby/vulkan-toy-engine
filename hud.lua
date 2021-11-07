local function click() 
    print("You clicked me")
end

local function centerSelected()
    print("You need to make the api bindings")
end

-- for text buttons it ignores the width field and just makes the width based on what the text says
Hud = {
    x = -1.0,
    y = 0.7,
    width = 2.0,
    height = 0.3,
    -- onClick = click,
    kind = GuiLayoutType__PANEL,
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
            kind = GuiLayoutType__TEXT_BUTTON,
            text = "ClickMe",
            color = 0x000000ff,
            -- children = {}
        },
        {
            x = -0.9,
            y = 0.875,
            width = 0.5,
            height = 0.1,
            onClick = click,
            kind = GuiLayoutType__TEXT_BUTTON,
            text = "ClickMe2",
            color = 0x000000ff,
            -- children = {}
        }
    }
}