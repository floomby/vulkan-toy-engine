Shipyard = {
    model = "models/shipyard.obj",
    texture = "textures/shipyard.png",
    icon = "textures/shipyard_icon.png",
    maxSpeed = 0,
    mass = 100,
    thrust = 0,
    maxOmega = 0.0,
    maxHealth = 5000.0,
    weapons = {
        {
            name = "basic_plasma",
            position = { -1.2, 0.0, 0.0 }
        }
    },
    unitAIs = {
    },
    resources = 300,
    buildOptions = {
        "ship",
        "miner",
        "constructor"
    },
    buildPower = 10,
    stationary = true,
}