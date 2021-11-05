local function click() 
    print("You clicked me")
end

Hud = {
    x = -1.0,
    y = 0.7,
    width = 2.0,
    height = 0.3,
    onClick = click,
    kind = GuiLayoutType__PANEL,
    text = ""
}