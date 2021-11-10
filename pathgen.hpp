#pragma once

#include "instance.hpp"
#include "utilities.hpp"

namespace Pathgen {
    enum class Kind {
        OMNI,
        COUNT
    };
    // yaw, pitch, roll
    // template<Kind K> std::array<glm::vec3, 2> move(const Entity *entity, const glm::vec3& heading,
    //     const glm::vec3& position, const glm::vec3& dP, const glm::vec3& dH, const glm::vec3& destination);
    // std::array<glm::vec3, 2> move<OMNI>(const Entity *entity, const glm::vec3& heading,
    //     const glm::vec3& position, const glm::vec3& dP, const glm::vec3& dH, const glm::vec3& destination) {
        

        
        // For the simplest case (move to a single point without following commands queued) I have formalized the continuous version of the problem
/*

Find $dP(t)$ such that $P(\tau)$ is the destination where \\
$ P(\tau) = \int_{0}^{\tau}dP(t)dt + P_0 $\\
which minimizes $\tau$ and $dP(\tau) = 0$ and $|dP(a)| < a_m \: \and \: |P(b)| < b_m \: \forall \: a,b \in [0,\tau]$ for arbitrary $a_m,b_m > 0$ \\

*/
        // tbh for moving through a point to another command I haven't yet formalized it

        // Here is an * very * approximate solution to the 1 point problem
        // check if in the braking destination braking region
    // }
    glm::vec3 clampLength(const glm::vec3& v, float max) {
        auto len = length(v);
        if (len > max) {
            return v * (max / len);
        }
        return v;
    }

    void applySteering(Instance& inst, const glm::vec3& steering) {
        auto steeringAcceleration = clampLength(steering, inst.entity->dv);
        // acceleration = steering_force / mass
        inst.dP = clampLength(inst.dP + steeringAcceleration, inst.entity->v_m);
        inst.position += inst.dP;
        // inst.heading = normalize(glm::quat({1.0, steeringAcceleration.y, steeringAcceleration.x, steeringAcceleration.z }));
        inst.heading = normalize(glm::quat({1.0f, 0.f, 0.f, 1.0f }));
    }

    glm::vec3 seek(Instance& inst, const glm::vec3& dest) {
        auto desiredVelocity = normalize(dest - inst.position) * inst.entity->v_m;
        // std::cout << desiredVelocity << std::endl;
        return desiredVelocity - inst.dP;
    }
};