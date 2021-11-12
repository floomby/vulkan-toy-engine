#pragma once

#include "instance.hpp"
#include "utilities.hpp"

namespace Pathgen {
    enum class Kind {
        OMNI,
        COUNT
    };

    void applySteering(Instance& inst, const glm::vec3& steering) {
        auto steeringAcceleration = clampLength(steering, inst.entity->dv);
        // acceleration = steering_force / mass
        inst.dP = clampLength(inst.dP + steeringAcceleration, inst.entity->v_m);
        inst.position += inst.dP;

        auto rot = rotationVector({ 1.0f, 0.0f, 0.0f}, steeringAcceleration);
        auto wantedAngular = log(rot * inverse(inst.heading));
        inst.heading = slerp(inst.heading, rot, 0.05f);

        // inst.heading = normalize(glm::quat({1.0, steeringAcceleration.x, steeringAcceleration.y, steeringAcceleration.z }));
        // inst.heading = normalize(glm::quat({1.0f, 0.f, 0.f, 0.0f }));
    }

    void seek(Instance& inst, const glm::vec3& dest) {
        auto desiredVelocity = normalize(dest - inst.position) * inst.entity->v_m;
        applySteering(inst, desiredVelocity - inst.dP);
    }

    void arrive(Instance& inst, const glm::vec3& dest) {
        auto offset = dest - inst.position;
        auto dist = length(offset);
        auto ramped = inst.entity->maxSpeed * (dist / *inst.entity->brakingCurve.rend() * 2.f);
        auto clipped = std::min(ramped, inst.entity->maxSpeed);
        auto desired = (clipped / dist) * offset;
        applySteering(inst, desired - inst.dP);
    }
}

