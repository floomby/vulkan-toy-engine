Ship = {
    model = "models/spaceship.obj",
    texture = "textures/spaceship.png",
    icon = "textures/spaceship_icon.png",
    maxSpeed = 20,
    mass = 15,
    thrust = 13,
    maxOmega = 0.2,
    maxHealth = 500.0,
    weapons = {
        {
            name = "basic_beam",
            position = { 0.0, 0.0, 0.0 },
        }
        -- "basic_plasma",
        -- "basic_guided"
    },
    unitAIs = {
        "shooter_tester"
    },
    resources = 120
}