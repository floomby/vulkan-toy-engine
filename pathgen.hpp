#pragma once

#include "instance.hpp"
#include "utilities.hpp"

namespace Pathgen {
    enum class Kind {
        OMNI,
        COUNT
    };

    const float arrivalDeltaContinuing = 1.0f;
    const float arrivalDeltaStopping = 0.05f;
    const float steeringHeadingUpdateThreshhold = 0.00000000001f;

    void applySteering(Instance& inst, const glm::vec3& steering) {
        auto steeringAcceleration = clampLength(steering, inst.entity->dv);
        // acceleration = steering_force / mass
        inst.dP = clampLength(inst.dP + steeringAcceleration, inst.entity->v_m);
        inst.position += inst.dP;
        if (length(steeringAcceleration) < steeringHeadingUpdateThreshhold) return;

        auto rot = rotationVector({ 1.0f, 0.0f, 0.0f}, steeringAcceleration);
        // auto wantedAngular = log(rot * inverse(inst.heading));
        inst.heading = slerp(inst.heading, rot, 0.05f);

        // inst.heading = normalize(glm::quat({1.0, steeringAcceleration.x, steeringAcceleration.y, steeringAcceleration.z }));
        // inst.heading = normalize(glm::quat({1.0f, 0.f, 0.f, 0.0f }));
    }

    float seek(Instance& inst, const glm::vec3& dest) {
        auto offset = dest - inst.position;
        auto dist = length(offset);
        auto desiredVelocity = offset / dist * inst.entity->v_m;
        applySteering(inst, desiredVelocity - inst.dP);
        return dist;
    }

    float arrive(Instance& inst, const glm::vec3& dest) {
        auto offset = dest - inst.position;
        auto dist = length(offset);
        // I need to figure out the braking distance, clearly doing the integration to find the straigh approach braking distance was not the right thing
        auto ramped = inst.entity->maxSpeed * (dist / 100.0f);
        auto clipped = std::min(ramped, inst.entity->maxSpeed);
        auto desired = (clipped / dist) * offset;
        // std::cout << desired - inst.dP << std::endl;
        applySteering(inst, desired - inst.dP);
        return dist;
    }

    void stop(Instance& inst) {
        applySteering(inst, -inst.dP);
    }

    const float kick = 0.1f;

    void collide(Instance& a, Instance& b) {
        auto v = a.position - b.position;
        auto d2 = length2(v);
        if (d2 < 0.01) // avoid division by 0
            v = { 0.0, 0.0, 1.0 };
        auto db2 = sq(a.entity->boundingRadius + b.entity->boundingRadius);
        if (d2 < db2) {
            auto amount = db2 - d2;
            a.dP += amount * kick * normalize(v);
            b.dP -= amount * kick * normalize(v);
        }
    }

    // This function doesn't compute the right thing exactly (It should make normed vectors, but it does not, but I kind of like how it looks now)
    glm::vec3 computeBalisticTrajectory(const glm::vec3& o, const glm::vec3& p,
        const glm::vec3& dp, float r, float v) {
        auto d = p - o;
        // length2(d) - 2 * dot(dp, d) * t + sq(t) * (length2(dp) - sq(v)) = 0
        auto a = length2(dp) - sq(v);
        auto b = 2 * dot(dp, d);
        auto c = length2(d);
        auto disc = sqrtf(sq(b) - 4 * a * c);
        auto t = (disc - b) / (2 * a);
        auto x = (((o - p) / t) + dp) / - v;
        return -x;
    }
}