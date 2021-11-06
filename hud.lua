local function click() 
    print("You clicked me")
end

-- for text buttons it ignores the width field and just makes the width based on what the text says
Hud = {
    x = -1.0,
    y = 0.7,
    width = 2.0,
    height = 0.3,
    onClick = click,
    kind = GuiLayoutType__PANEL,
    text = "",
    children = {
        {
            x = -0.8,
            y = 0.75,
            width = 0.5,
            height = 0.1,
            onClick = click,
            kind = GuiLayoutType__TEXT_BUTTON,
            text = "ClickMe",
            -- children = {}
        }
    }
}